/**
 * @file      client_connections.cc
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.5
 * @brief     Implementations of TCP and UDP connections used by client.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF CLIENT_CONNECTIONS.CC ]***************************************************************************** *
 * ****************************************************************************************************************** */


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <cassert>

#include "client_globals.hh"
#include "client_connections.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ TCP_CONNECTION IMPLEMENTATIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace client {

  /**
   * Constructor of client class.
   *
   * @param[in]   settings          Global client settings.
   * @param[out]  msg_storage       Place (memory) where to store the incoming and processed message.
   * @param[out]  action_cond       Condition variable to be used for notifying about new incoming message.
   * @param[out]  action_req_mutex  Mutex used for synchronisation for notification.
   * @param[out]  flag              Indication flag which should be set for a new message.
   * @param[out]  barrier           Barrier to be used for threads synchronisation.
   */
  tcp_connection::tcp_connection(client::settings_tuple &settings, protocol::message &msg_storage,
                                 boost::condition_variable &action_req, boost::mutex &action_req_mutex, bool &flag,
                                 boost::barrier &barrier) :
    connection(std::get<IPv4_ADDRESS>(settings), std::get<SERVER_PORT>(settings)),
    timeout_in_(io_service_), timeout_out_(io_service_),
    message_in_{msg_storage}, action_req_{action_req}, action_req_mutex_{action_req_mutex}, new_message_flag_{flag},
    init_barrier_{barrier}, settings_{settings}
  {{{
    messages_out_.resize(1);    // Avoiding segmentation fault.

    messages_out_[0].type = protocol::E_type::CTRL;
    messages_out_[0].ctrl_type = protocol::E_ctrl_type::SYN;
    messages_out_[0].status = protocol::E_status::QUERY;

    // Prepare HELLO packet for fast sending:
    HELLO_packet_.type = protocol::E_type::INFO;
    HELLO_packet_.info_type = protocol::E_info_type::HELLO;
    HELLO_packet_.status = protocol::E_status::UPDATE;

    FIN_packet_.type = protocol::E_type::CTRL;
    FIN_packet_.ctrl_type = protocol::E_ctrl_type::FIN;
    FIN_packet_.status = protocol::E_status::UPDATE;

    return;
  }}}


  /**
   * Establishes the connection to the server. If the establishing of the connection is successful, then it starts a new
   * thread to run the handshake phase. After successful handshake, the thread periodically sends HELLO packets to let
   * the server know we're still here.
   *
   * @note    Any informations about an error are provided via a message as if the connection would have send message to
   *          itself.
   * @note    This is a public function which should be used outside the running thread, not within inside.
   * @return  'false' upon any connection error, 'true' otherwise.
   */
  bool tcp_connection::connect()
  {{{
    if (socket_.is_open() == true) {
      message_in_.type = protocol::E_type::ERROR;
      message_in_.error_type = protocol::E_error_type::ALREADY_CONNECTED;
      message_in_.status = protocol::E_status::LOCAL;
      message_in_.data.push_back("Connection already established");

      return false;
    }

    boost::system::error_code error;

    it_endpoint_ = resolver_.resolve(query_, error);

    if (error) {
      message_in_.type = protocol::E_type::ERROR;
      message_in_.error_type = protocol::E_error_type::CONNECTION_FAILED;
      message_in_.status = protocol::E_status::LOCAL;
      message_in_.data.push_back(error.message());
      message_in_.data.back().append(" (NOTE: Are both the IP address and server port correct?)");

      return false;
    }

    endpoint_ = *it_endpoint_;
    socket_.connect(endpoint_, error);
    
    // Try other endpoints if they exist and error has occurred:
    while (error && ++it_endpoint_ != boost::asio::ip::tcp::resolver::iterator()) {
      socket_.close();
      endpoint_ = *it_endpoint_;
      socket_.connect(endpoint_, error);
    }
    
    if (error) {
      socket_.close();                  // Even upon error the socket remains open, free the resources.

      message_in_.type = protocol::E_type::ERROR;
      message_in_.error_type = protocol::E_error_type::CONNECTION_FAILED;
      message_in_.status = protocol::E_status::LOCAL;
      message_in_.data.push_back(error.message());
      message_in_.data.back().append(" (NOTE: Are both the IP address and server port correct?)");

      return false;
    }
    
    // Successful connection, starting & detaching a new thread for ASYNC operations:
    pu_asio_thread_ = std::unique_ptr<boost::thread>(new boost::thread(&tcp_connection::communication_start, this));
    
    return true;
  }}}


  /**
   * Disconnects the already connected connection by sending FIN packet, cancelling any pending asynchronous requests
   * and finishing the started thread. This function returns the connection to the state so the .connect() can be used
   * again.
   *
   * @note    This is a public function which should be used outside the running thread, not within inside.
   * @return  'true' upon success, 'false' in case there's no connection.
   */
  bool tcp_connection::disconnect()
  {{{
    if (socket_.is_open() == false) {
      message_in_.type = protocol::E_type::ERROR;
      message_in_.error_type = protocol::E_error_type::NO_CONNECTION;
      message_in_.status = protocol::E_status::LOCAL;
      message_in_.data.push_back("No established connection");

      return false;
    }

    async_send(FIN_packet_);

    // Giving time for message to be send:
    boost::asio::deadline_timer timer(io_service_, boost::posix_time::milliseconds(1000));
    timer.wait();
    
    // Cancelling any ASYNC operations from now on:
    timeout_out_.cancel();
    timeout_in_.cancel();

    // Closing the connection:
    boost::system::error_code ignored_error;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_error);
    socket_.cancel();
    socket_.close();
   
    io_service_.stop();                 // Making sure no more computation is done by thread.

    // We currently have a lock as a caller. We need to unlock to make sure the other thread doesn't get stuck waiting
    // for the lock, which it uses:
    action_req_mutex_.unlock();
    {
      // Join thread if it wasn't joined already:
      if (pu_asio_thread_ && (*pu_asio_thread_).joinable() == true) {
        (*pu_asio_thread_).join();
        pu_asio_thread_ = NULL;           // Make sure we don't call .join() multiple times.
      }
    }
    action_req_mutex_.lock();           // Acquire the lock back.

    io_service_.reset();                // Prepare the io_service for possible next run.

    return true;
  }}}

  
  /**
   * Sends the given message to the server.
   */
  void tcp_connection::async_send(const protocol::message &msg)
  {{{
    timeout_out_reset();                // Reseting the timer for outcoming messages.

    output_mutex_.lock();
    {
      messages_out_[0] = msg;
      pu_tcp_connect_->async_write(messages_out_, boost::bind(&tcp_connection::async_send_handler, this, 
                                                              boost::asio::placeholders::error));
    }
    output_mutex_.unlock();

    return;
  }}}

  // // // // // // // // // // // //

  /**
   * Registers the HANDSHAKE request and starts the io_service for upcoming ASYNC events.
   */
  void tcp_connection::communication_start()
  {{{
    pu_tcp_connect_->async_write(messages_out_, boost::bind(&tcp_connection::handshake_send_handler, this,
                                                            boost::asio::placeholders::error));
    start_timeout_in_timer();           // Timeout for server's answers.
    init_barrier_.wait();               // Wait for main thread to synchronize.
    io_service_.run();                  // Start the ASYNC events.
    return;
  }}}


  /**
   * In case of an error this function informs the running client. In case there wasn't any it registers the handler for
   * HANDSHAKE QUERY answer.
   */
  void tcp_connection::handshake_send_handler(const boost::system::error_code &error)
  {{{
    if (error) {
      action_req_mutex_.lock();
      {
        message_in_.type = protocol::E_type::ERROR;
        message_in_.error_type = protocol::E_error_type::HANDSHAKE;
        message_in_.status = protocol::E_status::LOCAL;
        message_in_.data.push_back("During HANDSHAKE init: ");
        message_in_.data.back().append(error.message());
        message_in_.data.push_back("The connection has been closed");


        new_message_flag_ = true;
        action_req_.notify_one();
      }
      action_req_mutex_.unlock();

      return;
    }

    timeout_in_set();                   // Make sure the server responds within timeout.

    pu_tcp_connect_->async_read(messages_in_, boost::bind(&tcp_connection::handshake_receive_handler, this,
                                                          boost::asio::placeholders::error));
    return;
  }}}


  /**
   * In case of an error this function informs the running client. In case there wasn't any it starts the ASYNC
   * send/receive loop.
   */
  void tcp_connection::handshake_receive_handler(const boost::system::error_code &error)
  {{{
    timeout_in_stop();                  // Answer arrived, stop the timeout.
    
    if (error) {
      action_req_mutex_.lock();
      {
        message_in_.type = protocol::E_type::ERROR;
        message_in_.error_type = protocol::E_error_type::HANDSHAKE;
        message_in_.status = protocol::E_status::LOCAL;
        message_in_.data.push_back("During HANDSHAKE confirm: ");
        message_in_.data.back().append(error.message());
        message_in_.data.push_back("The connection has been closed");

        new_message_flag_ = true;
        action_req_.notify_one();
      }
      action_req_mutex_.unlock();

      return;
    }
    // Testing the messages size - only 1 message is to be send by the protocol:
    else if (messages_in_.size() != 1) {
      action_req_mutex_.lock();
      {
        message_in_.type = protocol::E_type::ERROR;
        message_in_.status = protocol::E_status::LOCAL;

        if (messages_in_.size() == 0) {
          message_in_.error_type = protocol::E_error_type::EMPTY_MESSAGE;
          message_in_.data.push_back("Message with empty content received");
          message_in_.data.push_back("The connection has been closed");
        }
        else {
          message_in_.error_type = protocol::E_error_type::MULTIPLE_MESSAGES;
          message_in_.data.push_back("Wrong protocol - multiple messages received");
          message_in_.data.push_back("The connection has been closed");
        }

        new_message_flag_ = true;
        action_req_.notify_one();
      }
      action_req_mutex_.unlock();

      return;
    }
    // Testing the protocol again:
    else if (messages_in_[0].type != protocol::E_type::CTRL ||
             messages_in_[0].ctrl_type != protocol::E_ctrl_type::SYN) {
      action_req_mutex_.lock();
      {
        message_in_.type = protocol::E_type::ERROR;
        message_in_.error_type = protocol::E_error_type::WRONG_PROTOCOL;
        message_in_.status = protocol::E_status::LOCAL;
        message_in_.data.push_back("Server is using unknown protocol");
        message_in_.data.push_back("The connection has been closed");

        new_message_flag_ = true;
        action_req_.notify_one();
      }
      action_req_mutex_.unlock();

      return;
    }
    // Testing the server's answer to SYN request:
    else if (messages_in_[0].status != protocol::E_status::ACK) {
      action_req_mutex_.lock();
      {
        message_in_.type = protocol::E_type::ERROR;
        message_in_.error_type = protocol::E_error_type::REJECTED_CONNECTION;
        message_in_.status = protocol::E_status::LOCAL;
        message_in_.data.push_back("Server rejected the connection");
        message_in_.data.push_back("The connection has been closed");

        new_message_flag_ = true;
        action_req_.notify_one();
      }
      action_req_mutex_.unlock();

      return;
    }

    asio_loops_start();                 // Successful handshake, continue.

    return;
  }}}
  
  // // // // // // // // // // // //

  /**
   * Starts the timer for server's responses.
   */
  void tcp_connection::start_timeout_in_timer()
  {{{
    timeout_in_.expires_at(boost::posix_time::pos_infin);
    timeout_in_.async_wait(boost::bind(&tcp_connection::timeout_in_handler, this, boost::asio::placeholders::error));

    return;
  }}}


  /**
   * Sets the new timeout for server's response.
   */
  inline void tcp_connection::timeout_in_set()
  {{{
    timeout_in_.expires_from_now(boost::posix_time::milliseconds(std::get<MAX_PING>(settings_)));

    return;
  }}}


  /**
   * Stops the timeout for server's response.
   */
  inline void tcp_connection::timeout_in_stop()
  {{{
    timeout_in_.expires_from_now(boost::posix_time::pos_infin);

    return;
  }}}

  
  /**
   * Handler for the timeout of the server. In case of a timeout it informs the client, in case of timeout update it
   * plans the ASYNC handler calling again.
   */
  void tcp_connection::timeout_in_handler(const boost::system::error_code &error)
  {{{
    switch (error.value()) {
      // Actual TIMEOUT was cancelled and new one was set (TIMEOUT was UPDATED):
      case boost::system::errc::operation_canceled :
        timeout_in_.async_wait(boost::bind(&tcp_connection::timeout_in_handler, this,
                                           boost::asio::placeholders::error));
        return;
      
      // TIMEOUT has expired - inform the client:
      case boost::system::errc::success :
        action_req_mutex_.lock();
        {
          message_in_.type = protocol::E_type::ERROR;
          message_in_.error_type = protocol::E_error_type::TIMEOUT;
          message_in_.status = protocol::E_status::LOCAL;
          message_in_.data.push_back("Connection to server has timed out");
          message_in_.data.push_back("The connection has been closed");

          new_message_flag_ = true;
          action_req_.notify_one();
        }
        action_req_mutex_.unlock(); 

        return;
      
      // Different error - inform the client:
      default :
        action_req_mutex_.lock();
        {
          message_in_.type = protocol::E_type::ERROR;
          message_in_.error_type = protocol::E_error_type::UNKNOWN_ERROR;
          message_in_.status = protocol::E_status::LOCAL;
          message_in_.data.push_back(error.message());
          message_in_.data.push_back("The connection has been closed");

          new_message_flag_ = true;
          action_req_.notify_one();
        }
        action_req_mutex_.unlock(); 

        return;
    }
  }}}


  /**
   * Starts the timeout for outcoming messages. Client is responsible to send HELLO packets in case he doesn't send any
   * other messages. This timer provides such a functionality.
   */
  void tcp_connection::start_timeout_out_timer()
  {{{
    timeout_out_.expires_from_now(boost::posix_time::milliseconds(std::get<HELLO_INTERVAL>(settings_)));
    timeout_out_.async_wait(boost::bind(&tcp_connection::timeout_out_handler, this, boost::asio::placeholders::error));

    return;
  }}}


  /**
   * Resets the timeout for outcoming message.
   */
  inline void tcp_connection::timeout_out_reset()
  {{{
    timeout_out_.expires_from_now(boost::posix_time::milliseconds(std::get<HELLO_INTERVAL>(settings_)));

    return; 
  }}}

  
  /**
   * Handler for the outcoming messages. If the timeout has run out, it sends the HELLO packet, otherwise the timeout is
   * postponed.
   */
  void tcp_connection::timeout_out_handler(const boost::system::error_code &error)
  {{{
    switch (error.value()) {
      // Actual TIMEOUT was cancelled and new one was set (TIMEOUT was RESETED):
      case boost::system::errc::operation_canceled :
        timeout_out_.async_wait(boost::bind(&tcp_connection::timeout_out_handler, this,
                                            boost::asio::placeholders::error));
        return;
      
      // TIMEOUT has expired - send the HELLO packet:
      case boost::system::errc::success :
        async_send(HELLO_packet_);
        timeout_out_.async_wait(boost::bind(&tcp_connection::timeout_out_handler, this, 
                                            boost::asio::placeholders::error));
        return;
      
      // Different error - inform the client:
      default :
        action_req_mutex_.lock();
        {
          message_in_.type = protocol::E_type::ERROR;
          message_in_.error_type = protocol::E_error_type::UNKNOWN_ERROR;
          message_in_.status = protocol::E_status::LOCAL;
          message_in_.data.push_back(error.message());
          message_in_.data.push_back("The connection has been closed");

          new_message_flag_ = true;
          action_req_.notify_one();
        }
        action_req_mutex_.unlock(); 

        return;
    }
  }}}

  // // // // // // // // // // // //

  /**
   * Handler for asynchronous send. In case of an error it informs the client and closes the connection, otherwise it
   * resets the outcoming timer.
   */
  void tcp_connection::async_send_handler(const boost::system::error_code &error)
  {{{
    if (error) {
      action_req_mutex_.lock();
      {
        message_in_.type = protocol::E_type::ERROR;
        message_in_.status = protocol::E_status::LOCAL;

        switch (error.value()) {
          case boost::asio::error::eof :
            message_in_.error_type = protocol::E_error_type::CONNECTION_CLOSED;
            message_in_.data.push_back("Connection closed by server");
            break;

          case boost::asio::error::operation_aborted :
            message_in_.error_type = protocol::E_error_type::TIMEOUT;
            message_in_.data.push_back("Connection to server has timed out");
            message_in_.data.push_back("The connection has been closed");
            break;

          default :
            message_in_.error_type = protocol::E_error_type::UNKNOWN_ERROR;
            message_in_.data.push_back("While sending message: ");
            message_in_.data.back().append(error.message());
            message_in_.data.push_back("The connection has been closed");
            break;
        }

        new_message_flag_ = true;
        action_req_.notify_one();
      }
      action_req_mutex_.unlock();

      return;
    }

    timeout_in_set();                   // Everything was correct, message was send ->> reset the timer.

    return;
  }}}

  
  /**
   * Starts the new asynchronous receive of the message.
   */
  void tcp_connection::async_receive()
  {{{
    pu_tcp_connect_->async_read(messages_in_, boost::bind(&tcp_connection::async_receive_handler, this,
                                                          boost::asio::placeholders::error));
    return;
  }}}


  /**
   * Handler for new incoming message. Stops the timeout timer for server's response, processes the message and informs
   * the client about the message or the error, if any has occurred.
   */
  void tcp_connection::async_receive_handler(const boost::system::error_code &error)
  {{{
    timeout_in_stop();

    if (error) {
      action_req_mutex_.lock();
      {
        message_in_.type = protocol::E_type::ERROR;
        message_in_.status = protocol::E_status::LOCAL;

        switch (error.value()) {
          case boost::asio::error::eof :
            message_in_.error_type = protocol::E_error_type::CONNECTION_CLOSED;
            message_in_.data.push_back("Connection closed by server");
            break;

          case boost::asio::error::operation_aborted :
            message_in_.error_type = protocol::E_error_type::TIMEOUT;
            message_in_.data.push_back("Connection to server has timed out");
            message_in_.data.push_back("The connection has been closed");
            break;

          default :
            message_in_.error_type = protocol::E_error_type::UNKNOWN_ERROR;
            message_in_.data.push_back("While receiving message: ");
            message_in_.data.back().append(error.message());
            message_in_.data.push_back("The connection has been closed");
            break;
        }
        
        new_message_flag_ = true;
        action_req_.notify_one();
      }
      action_req_mutex_.unlock();
      
      return;
    }
    // Testing the message received - the protocol expects only one actual message from server:
    else if (messages_in_.size() != 1) {
      action_req_mutex_.lock();
      {
        message_in_.type = protocol::E_type::ERROR;
        message_in_.status = protocol::E_status::LOCAL;

        if (messages_in_.size() == 0) {
          message_in_.error_type = protocol::E_error_type::EMPTY_MESSAGE;
          message_in_.data.push_back("Message with empty content received");
          message_in_.data.push_back("The connection has been closed");
        }
        else {
          message_in_.error_type = protocol::E_error_type::MULTIPLE_MESSAGES;
          message_in_.data.push_back("Wrong protocol - multiple messages received");
          message_in_.data.push_back("The connection has been closed");
        }

        new_message_flag_ = true;
        action_req_.notify_one();
      }
      action_req_mutex_.unlock();

      return;
    }
    // Do not bother the client with HELLO packets - we can handle them ourselves:
    else if (messages_in_[0].type == protocol::E_type::INFO &&
             messages_in_[0].info_type == protocol::E_info_type::HELLO) {
            #ifndef NDEDUG
        //  Testing the protocol - for debugging purposes only:
        if (messages_in_[0].status != protocol::E_status::ACK) {
          action_req_mutex_.lock();
          {
            message_in_.type = protocol::E_type::ERROR;
            message_in_.status = protocol::E_status::LOCAL;
            message_in_.error_type = protocol::E_error_type::WRONG_PROTOCOL;
            message_in_.data.push_back("Server is using wrong protocol");
            message_in_.data.push_back("The connection has been closed");

            new_message_flag_ = true;
            action_req_.notify_one();
          }
          action_req_mutex_.unlock();
        }
      #endif

      async_receive();
      return;
    }
    else {
      //  Message has met all pre-conditions, prepare it for the client:
      action_req_mutex_.lock();
      {
        message_in_ = messages_in_[0];
        new_message_flag_ = true;
        action_req_.notify_one();
      }
      action_req_mutex_.unlock();
    
      async_receive();
      return;
    }
  }}}


  /**
   * Starts the outcoming timer for sending HELLO packets and starts the asynchronous receiving loop.
   */
  void tcp_connection::asio_loops_start()
  {{{
    start_timeout_out_timer();
    async_receive();

    return;
  }}}
}


/* ****************************************************************************************************************** *
 ~ ~~~[ GAME_CONNECTION IMPLEMENTATIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace client {
  game_connection::game_connection(const std::string &IP_address, const std::string &port, const std::string &auth_key,
                                   protocol::update &update_storage, boost::condition_variable &update_cond_var,
                                   boost::mutex &update_mutex, boost::condition_variable &error_cond_var,
                                   boost::mutex &error_mutex, protocol::message &error_msg_storage, bool &error_flag) :
    connection(IP_address, port),
    update_in_{update_storage}, update_in_received_{update_cond_var}, update_in_mutex_{update_mutex},
    error_occured_{error_cond_var}, error_mutex_{error_mutex}, error_message_{error_msg_storage},
    error_flag_{error_flag},
    auth_key_{auth_key}
  {{{
    commands_out_.resize(1);
    return;
  }}}


  game_connection::~game_connection()
  {{{
    if (socket_.is_open() == true) {
      // Closing the connection:
      boost::system::error_code ignored_error;

      socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_error);
      socket_.cancel();
      socket_.close();
    }
   
    io_service_.stop();                   // Making sure no more computation is done by thread.

    // Join thread if it wasn't joined already:
    if (pu_asio_thread_ && (*pu_asio_thread_).joinable() == true) {
      (*pu_asio_thread_).join();
      pu_asio_thread_.reset();            // Make sure we don't call .join() multiple times.
    }

    return;
  }}}


  bool game_connection::connect()
  {{{
    assert(socket_.is_open() == false);

    boost::system::error_code error;

    it_endpoint_ = resolver_.resolve(query_, error);

    if (error) {
      error_message_.type = protocol::E_type::ERROR;
      error_message_.error_type = protocol::E_error_type::CONNECTION_FAILED;
      error_message_.status = protocol::E_status::GAME_LOCAL;
      error_message_.data.push_back(error.message());

      return false;
    }

    endpoint_ = *it_endpoint_;
    socket_.connect(endpoint_, error);
    
    // Try other endpoints if they exist and error has occurred:
    while (error && ++it_endpoint_ != boost::asio::ip::tcp::resolver::iterator()) {
      socket_.close();
      endpoint_ = *it_endpoint_;
      socket_.connect(endpoint_, error);
    }
    
    if (error) {
      socket_.close();                  // Even upon error the socket remains open, free the resources.

      error_message_.type = protocol::E_type::ERROR;
      error_message_.error_type = protocol::E_error_type::CONNECTION_FAILED;
      error_message_.status = protocol::E_status::GAME_LOCAL;
      error_message_.data.push_back(error.message());

      return false;
    }
    
    // Successful connection, starting & detaching a new thread for ASYNC operations:
    pu_asio_thread_ = std::unique_ptr<boost::thread>(new boost::thread(&game_connection::communication_start, this));
    
    return true;
  }}}


  bool game_connection::disconnect()
  {{{
    if (socket_.is_open() == true) {
      // Closing the connection:
      boost::system::error_code ignored_error;

      socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_error);
      socket_.cancel();
      socket_.close();
    }
   
    io_service_.stop();                   // Making sure no more computation is done by thread.

#if 0
    // We currently have a lock as a caller. We need to unlock to make sure the other thread doesn't get stuck waiting
    // for the lock, which it uses:
    update_in_mutex_.unlock();
    {
#endif
      // Join thread if it wasn't joined already:
      if (pu_asio_thread_ && (*pu_asio_thread_).joinable() == true) {
        (*pu_asio_thread_).join();
        pu_asio_thread_.reset();          // Make sure we don't call .join() multiple times.
      }
#if 0
    }
    update_in_mutex_.lock();              // Acquire the lock back.
#endif

    io_service_.reset();

    return true;
  }}}

  
  void game_connection::communication_start()
  {{{
    authenticate();
    io_service_.run();
    return;
  }}}


  void game_connection::authenticate()
  {{{
    std::vector<protocol::message> message_vect;
    message_vect.resize(1);

    message_vect[0].type = protocol::E_type::CTRL;
    message_vect[0].ctrl_type = protocol::E_ctrl_type::SYN;
    message_vect[0].status = protocol::E_status::UPDATE;
    message_vect[0].data.push_back(auth_key_);

    pu_tcp_connect_->async_write(message_vect, boost::bind(&game_connection::authenticate_handler, this,
                                                           boost::asio::placeholders::error));
    return;
  }}}


  void game_connection::authenticate_handler(const boost::system::error_code &error)
  {{{
    if (error) {
      error_mutex_.lock();
      {
        error_message_.type = protocol::E_type::ERROR;
        error_message_.status = protocol::E_status::GAME_LOCAL;

        switch (error.value()) {
          case boost::asio::error::eof :
            error_message_.error_type = protocol::E_error_type::CONNECTION_CLOSED;
            error_message_.data.push_back("Game authentication failed");
            break;

          case boost::asio::error::operation_aborted :
            error_message_.error_type = protocol::E_error_type::TIMEOUT;
            error_message_.data.push_back("Game connection to server has timed out");
            error_message_.data.push_back("The connection has been closed");
            break;

          default :
            error_message_.error_type = protocol::E_error_type::UNKNOWN_ERROR;
            error_message_.data.push_back("While authenticating to game: ");
            error_message_.data.back().append(error.message());
            error_message_.data.push_back("The connection has been closed");
            break;
        }

        error_flag_ = true;
        error_occured_.notify_one();
      }
      error_mutex_.unlock();
    }

    async_receive();
    return;
  }}}


  void game_connection::async_send(const protocol::command &cmd)
  {{{
    commands_out_[0] = cmd;
    pu_tcp_connect_->async_write(commands_out_, boost::bind(&game_connection::async_send_handler, this,
                                                            boost::asio::placeholders::error));
    return;
  }}}
  

  void game_connection::async_send_handler(const boost::system::error_code &error)
  {{{
    if (error) {
      error_mutex_.lock();
      {
        error_message_.type = protocol::E_type::ERROR;
        error_message_.status = protocol::E_status::GAME_LOCAL;

        switch (error.value()) {
          case boost::asio::error::eof :
            error_message_.error_type = protocol::E_error_type::CONNECTION_CLOSED;
            error_message_.data.push_back("Connection closed by server");
            break;

          case boost::asio::error::operation_aborted :
            error_message_.error_type = protocol::E_error_type::TIMEOUT;
            error_message_.data.push_back("Connection to server has timed out");
            error_message_.data.push_back("The connection has been closed");
            break;

          default :
            error_message_.error_type = protocol::E_error_type::UNKNOWN_ERROR;
            error_message_.data.push_back("While sending command: ");
            error_message_.data.back().append(error.message());
            error_message_.data.push_back("The connection has been closed");
            break;
        }

        error_flag_ = true;
        error_occured_.notify_one();
      }
      error_mutex_.unlock();
    }

    return;
  }}}


  void game_connection::async_receive()
  {{{
    pu_tcp_connect_->async_read(updates_in_, boost::bind(&game_connection::async_receive_handler, this,
                                                         boost::asio::placeholders::error));
    return;
  }}}
  
  
  void game_connection::async_receive_handler(const boost::system::error_code &error)
  {{{
    if (error) {
      error_mutex_.lock();
      {
        error_message_.type = protocol::E_type::ERROR;
        error_message_.status = protocol::E_status::LOCAL;

        switch (error.value()) {
          case boost::asio::error::eof :
            error_message_.error_type = protocol::E_error_type::CONNECTION_CLOSED;
            error_message_.data.push_back("Connection closed by server");
            break;

          case boost::asio::error::operation_aborted :
            error_message_.error_type = protocol::E_error_type::TIMEOUT;
            error_message_.data.push_back("Connection to server has timed out");
            error_message_.data.push_back("The connection has been closed");
            break;

          default :
            error_message_.error_type = protocol::E_error_type::UNKNOWN_ERROR;
            error_message_.data.push_back("While receiving update: ");
            error_message_.data.back().append(error.message());
            error_message_.data.push_back("The connection has been closed");
            break;
        }
        
        error_flag_ = true;
        error_occured_.notify_one();
      }
      error_mutex_.unlock();
      return;
    }
    // Testing the message received - the protocol expects only one actual message from server:
    else if (updates_in_.size() != 1) {
      error_mutex_.lock();
      {
        error_message_.type = protocol::E_type::ERROR;
        error_message_.status = protocol::E_status::LOCAL;

        if (updates_in_.size() == 0) {
          error_message_.error_type = protocol::E_error_type::EMPTY_MESSAGE;
          error_message_.data.push_back("Message with empty content received");
          error_message_.data.push_back("The connection has been closed");
        }
        else {
          error_message_.error_type = protocol::E_error_type::MULTIPLE_MESSAGES;
          error_message_.data.push_back("Wrong protocol - multiple messages received");
          error_message_.data.push_back("The connection has been closed");
        }

        error_flag_ = true;
        error_occured_.notify_one();
      };
      error_mutex_.unlock();
      return;
    }
    else {
      //  Message has met all pre-conditions, prepare it for the game instance:
      update_in_mutex_.lock();
      {
        update_in_ = updates_in_[0];
        update_in_received_.notify_one();
      }
      update_in_mutex_.unlock();
    
      async_receive();
      return;
    }
  }}}
}


/* ****************************************************************************************************************** *
 * ***[ END OF CLIENT_CONNECTIONS.CC ]******************************************************************************* *
 * ****************************************************************************************************************** */

