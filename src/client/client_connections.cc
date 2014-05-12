/**
 * @file      client_connections.cc
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.4
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
 ~ ~~~[ CONNECTIONS CLASSES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace client {
  /**
   * Constructor of client class.
   *
   * @param[in]   IP_addres     IPv4 address of the server to be used.
   * @param[in]   port          Listening PORT of the server.
   * @param[out]  msg_storage   Place (memory) where to store the incoming and processed message.
   * @param[out]  action_cond   Condition variable to be used for notifying about new incoming message.
   * @param[out]  action_mutex  Mutex used for synchronisation for notification.
   * @param[out]  flag          Indication flag to be set for new message.   
   */
  tcp_connection::tcp_connection(const std::string &IP_address, const std::string &port, protocol::message msg_storage,
                                 boost::condition_variable &action_cond, boost::mutex &action_mutex, bool &flag,
                                 boost::barrier barrier, client::settings_tuple &settings) :
    connection(IP_address, port),
    message_in_{msg_storage}, action_req_{action_cond}, action_mutex_{action_mutex}, new_message_flag_{flag},
    init_barrier_{barrier}, settings_{settings}
  {{{
    // We're preparing the serialization and SYN message in advance so we can run handshake right after connecting:
    pu_tcp_connect_ = std::unique_ptr<protocol::tcp_serialization>(new protocol::tcp_serialization(socket_));

    messages_out_[0].type = protocol::E_type::CTRL;
    messages_out_[0].ctrl_type = protocol::E_ctrl_type::SYN;
    messages_out_[0].status = protocol::E_status::QUERY;

    // Make sure we have enough memory to create possible error message:
    message_in_.data.reserve(1);

    // Prepare HELLO packet for fast sending:
    hello_packet_.type = protocol::E_type::INFO;
    hello_packet_.info_type = protocol::E_info_type::HELLO;
    hello_packet_.status = protocol::E_status::UPDATE;

    return;
  }}}


  /**
   * Establishes the connection to the server and starts the handshake. If the handshake is successful, it also starts
   * new thread for asynchronous receiving of new messages.
   *
   * @return  'false' upon any connection error, 'true' otherwise.
   * @note    The information about HANDSHAKE error is provided by notifying as if the new message has arrived.
   */
  bool tcp_connection::connect()
  {{{
    assert(socket_.is_open() == false);

    boost::system::error_code error;

    socket_.connect(endpoint_, error);
    
    // Try other endpoints if they exist:
    while (error && it_endpoint_ != boost::asio::ip::tcp::resolver::iterator()) {
      socket_.close();
      it_endpoint_++;
      endpoint_ = *it_endpoint_;
      socket_.connect(endpoint_, error);
    }
    
    if (error) {
      std::cerr << "Error while connecting to server: " << error.message() << std::endl;
      return false;
    }
    
    // Successful connection, starting & detaching a new thread for ASYNC operations:
    pu_asio_thread_ = std::unique_ptr<boost::thread> (new boost::thread(&tcp_connection::communication_start, this));
    
    return true;
  }}}


  void tcp_connection::disconnect()
  {{{
    return;
  }}}


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
      action_mutex_.lock();
      {
        message_in_.type = protocol::E_type::ERROR;
        message_in_.error_type = protocol::E_error_type::HANDSHAKE;
        message_in_.status = protocol::E_status::LOCAL;
        message_in_.data[0] = "Error during HANDSHAKE initialization: ";
        message_in_.data[0].append(error.message());

        new_message_flag_ = true;
        action_req_.notify_one();
      }
      action_mutex_.unlock();

      return;
    }

    timeout_in_set();
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
    timeout_in_stop();                  // Answer arrived, stop timeout.

    if (error) {
      action_mutex_.lock();
      {
        message_in_.type = protocol::E_type::ERROR;
        message_in_.error_type = protocol::E_error_type::HANDSHAKE;
        message_in_.status = protocol::E_status::LOCAL;
        message_in_.data[0] = "Error during HANDSHAKE confirmation: ";
        message_in_.data[0].append(error.message());

        new_message_flag_ = true;
        action_req_.notify_one();
      }
      action_mutex_.unlock();

      return;
    }

    asio_loops_start();

    return;
  }}}
  

  void tcp_connection::start_timeout_in_timer()
  {{{
    timeout_in_.expires_at(boost::posix_time::pos_infin);
    timeout_in_.async_wait(boost::bind(&tcp_connection::timeout_in_handler, this, boost::asio::placeholders::error));
    return;
  }}}

  inline void tcp_connection::timeout_in_set()
  {{{
    timeout_in_.expires_from_now(boost::posix_time::milliseconds(std::get<MAX_PING>(settings_)));
    return;
  }}}


  inline void tcp_connection::timeout_in_stop()
  {{{
    timeout_in_.expires_from_now(boost::posix_time::pos_infin);
    return;
  }}}

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
        action_mutex_.lock();
        {
          message_in_.type = protocol::E_type::ERROR;
          message_in_.error_type = protocol::E_error_type::TIMEOUT;
          message_in_.status = protocol::E_status::LOCAL;
          message_in_.data[0] = "Error: Connection to server has timed out";

          new_message_flag_ = true;
          action_req_.notify_one();
        }
        action_mutex_.unlock(); 
        return;
      
      // Different error - inform the client:
      default :
        action_mutex_.lock();
        {
          message_in_.type = protocol::E_type::ERROR;
          message_in_.error_type = protocol::E_error_type::UNKNOWN_ERROR;
          message_in_.status = protocol::E_status::LOCAL;
          message_in_.data[0] = "Error: ";
          message_in_.data[0].append(error.message());

          new_message_flag_ = true;
          action_req_.notify_one();
        }
        action_mutex_.unlock(); 
        return;
    }
  }}}

  void tcp_connection::start_timeout_out_timer()
  {{{
    timeout_out_.expires_from_now(boost::posix_time::milliseconds(std::get<HELLO_INTERVAL>(settings_)));
    timeout_out_.async_wait(boost::bind(&tcp_connection::timeout_out_handler, this, boost::asio::placeholders::error));
    return;
  }}}

  inline void tcp_connection::timeout_out_reset()
  {{{
    timeout_out_.expires_from_now(boost::posix_time::milliseconds(std::get<HELLO_INTERVAL>(settings_)));
    return; 
  }}}

  void tcp_connection::timeout_out_handler(const boost::system::error_code &error)
  {{{
    switch (error.value()) {
      // Actual TIMEOUT was cancelled and new one was set (TIMEOUT was RESETED):
      case boost::system::errc::operation_canceled :
        timeout_in_.async_wait(boost::bind(&tcp_connection::timeout_in_handler, this,
                                           boost::asio::placeholders::error));
        return;
      
      // TIMEOUT has expired - send the HELLO packet:
      case boost::system::errc::success :
        async_send(hello_packet_);
        timeout_in_.async_wait(boost::bind(&tcp_connection::timeout_in_handler, this, 
                                           boost::asio::placeholders::error));
        return;
      
      // Different error - inform the client:
      default :
        action_mutex_.lock();
        {
          message_in_.type = protocol::E_type::ERROR;
          message_in_.error_type = protocol::E_error_type::UNKNOWN_ERROR;
          message_in_.status = protocol::E_status::LOCAL;
          message_in_.data[0] = "Error: ";
          message_in_.data[0].append(error.message());

          new_message_flag_ = true;
          action_req_.notify_one();
        }
        action_mutex_.unlock(); 
        return;
    }
  }}}

  void tcp_connection::async_send(const protocol::message &msg)
  {{{
    output_mutex_.lock();
    {
      messages_out_[0] = msg;
      pu_tcp_connect_->async_write(messages_out_, boost::bind(&tcp_connection::async_send_handler, this, 
                                                              boost::asio::placeholders::error));
    }
    output_mutex_.lock();

    timeout_out_reset();

    return;
  }}}

  void tcp_connection::async_send_handler(const boost::system::error_code &error)
  {{{
    if (error) {
      action_mutex_.lock();
      {
        message_in_.type = protocol::E_type::ERROR;
        message_in_.status = protocol::E_status::LOCAL;

        switch (error.value()) {
          case boost::asio::error::eof :
            message_in_.data[0] = "INFO: Connection closed by server";
            break;

          case boost::asio::error::operation_aborted :
            message_in_.data[0] = "INFO: Connection to server has timed out";
            break;

          default :
            message_in_.data[0] = "Error during ASYNC_SEND: ";
            message_in_.data[0].append(error.message());
            break;
        }

        new_message_flag_ = true;
        action_req_.notify_one();
      }
      action_mutex_.unlock();
      return;
    }

    timeout_in_set();
    return;
  }}}

  void tcp_connection::async_receive()
  {{{
    pu_tcp_connect_->async_read(messages_in_, boost::bind(&tcp_connection::async_receive_handler, this,
                                                          boost::asio::placeholders::error));
    return;
  }}}

  void tcp_connection::async_receive_handler(const boost::system::error_code &error)
  {{{
    action_mutex_.lock();
    {
      if (error) {
        message_in_.type = protocol::E_type::ERROR;
        message_in_.status = protocol::E_status::LOCAL;

        switch (error.value()) {
          case boost::asio::error::eof :
            message_in_.error_type = protocol::E_error_type::CLOSED_CONNECTION;
            message_in_.data[0] = "INFO: Connection closed by server";
            break;

          case boost::asio::error::operation_aborted :
            message_in_.error_type = protocol::E_error_type::TIMEOUT;
            message_in_.data[0] = "INFO: Connection to server has timed out";
            break;

          default :
            message_in_.error_type = protocol::E_error_type::UNKNOWN_ERROR;
            message_in_.data[0] = "Error during ASYNC_RECEIVE: ";
            message_in_.data[0].append(error.message());
            break;
        }
      }
      // Testing the message received - the protocol expects only one actual message from server:
      else if (messages_in_.size() != 1) {
        message_in_.type = protocol::E_type::ERROR;
        message_in_.status = protocol::E_status::LOCAL;

        if (messages_in_.size() == 0) {
          message_in_.error_type = protocol::E_error_type::EMPTY_MESSAGE;
          message_in_.data[0] = "ERROR: Message with now content received";
        }
        else {
          message_in_.error_type = protocol::E_error_type::MULTIPLE_MESSAGES;
          message_in_.data[0] = "ERROR: Multiple messages received";
        }
      }
      else {
        message_in_ = messages_in_[0];
      }
      
      new_message_flag_ = true;
      action_req_.notify_one();
    }
    action_mutex_.unlock();

    async_receive();
    return;
  }}}

  void tcp_connection::asio_loops_start()
  {{{
    start_timeout_out_timer();
    async_receive();
    return;
  }}}

}


/* ****************************************************************************************************************** *
 * ***[ END OF CLIENT_CONNECTIONS.CC ]******************************************************************************* *
 * ****************************************************************************************************************** */

