/**
 * @file      mazed_server_connection.cc
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.3
 * @brief     Contains implementations of client_handler's class member functions.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_CL_HANDLER.CC ]******************************************************************************* *
 * ****************************************************************************************************************** */


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <sstream>
#include <string>

#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/thread/thread_guard.hpp>

#include <cassert>

#include "mazed_game_instance.hh"
#include "mazed_game_maze.hh"
#include "mazed_game_player.hh"
#include "mazed_cl_handler.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ MEMBER FUNCTIONS IMPLEMENTATIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace mazed {
  client_handler::client_handler(tcp::socket &socket, asio::io_service &io_service, mazed::settings_tuple &settings,
                                 const std::shared_ptr<mazed::shared_resources> ptr, unsigned connection_num) :
    socket_(socket),
    io_service_(io_service),
    ps_shared_res_{ptr},
    timeout_(timeout_io_service_),
    init_barrier_(3),
    settings_(settings)
  {{{
    chdir(std::get<mazed::LOG_FOLDER>(settings_).c_str());

    // Create the client's connection separate log:
    std::stringstream filename;
    filename << "connection_" << connection_num << ".log";

    log_file_.open(filename.str(), std::ofstream::out | std::ofstream::trunc);
    log(mazed::log_level::INFO, "Client handler has STARTED (with TCP connection inherited)");

    pu_tcp_connect_ = std::unique_ptr<protocol::tcp_serialization>(new protocol::tcp_serialization(socket));
    log(mazed::log_level::INFO, "New serialization over TCP connection created");

    messages_out_.resize(1);        // Avoiding segmentation fault because of using of operator = of the vector buffer.

    return;
  }}}


  client_handler::~client_handler()
  {{{
    log(mazed::log_level::INFO, "Client handler is STOPPING");
    log_file_.close();

    if (p_player_ != NULL) {
      p_player_->stop();
      delete p_player_;
      delete p_maze_;
    }

    return;
  }}}

  /**
   * Starts processing of the client's requests.
   */
  void client_handler::run()
  {{{
    // Starting other necessary threads - ASIO LOOP for ASYNC communication & TIMEOUT checker:
    boost::shared_ptr<boost::thread> asio_loop_thread(new boost::thread(&client_handler::start_asio_loop, this));
    boost::shared_ptr<boost::thread> timeout_thread(new boost::thread(&client_handler::start_timeout, this));

    run_processing();

    // Close the given socket properly and cancel any pending async operations on it:
    if (socket_.is_open() == true) {
      boost::system::error_code ignored_error;
      socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_error);
      socket_.cancel();
      socket_.close();
    }

    timeout_.cancel();              // Cancel any pending timeout checks.

    io_service_.stop();             // Make sure no more async operations will be planned.
    timeout_io_service_.stop();

    // Waiting for other threads to finish their job, we don't want any thread zombies:
    (*timeout_thread).join();
    (*asio_loop_thread).join();

    log(mazed::log_level::ALL, "All threads joined back successfully");
    return;
  }}}

  // // // // // // // // // // // // //

  /**
   * Runs the handshake procedure and starts the main processing loop of client's requests if the previous handshake was
   * successful.
   */
  void client_handler::run_processing()
  {{{
    if (handshake_success() == false) {
      return;                           // Handshake fail. Client has been already notified, bail out.
    }

    boost::unique_lock<boost::mutex> action_lock(action_req_mutex_);
    
    timeout_set();
    action_req_.wait(action_lock);      // Waiting for next message or timeout.
    timeout_stop();

    run_mutex_.lock();
    while (run_ == true) {
      run_mutex_.unlock();
      
      // Calling appropriate message handler of incoming message:
      switch (message_in_.type) {
        case CTRL :
          // NOTE: Making sure no one slips us the message that can cause STACK OVERFLOW:
          if (message_in_.ctrl_type >= 0 && message_in_.ctrl_type < E_CTRL_TYPE_SIZE) {
            (this->*ctrl_message_handlers_[message_in_.ctrl_type])();
          }
          else {
            message_prepare(ERROR, WRONG_PROTOCOL, UPDATE, data_t {"Wrong version of protocol"});
            log(mazed::log_level::ERROR, "CTRL type value overflow detected");
          }
          break;
        
        case INFO :
          if (message_in_.info_type == HELLO) {
            message_prepare(INFO, HELLO, ACK);
          }
          else {
            message_prepare(ERROR, WRONG_PROTOCOL, UPDATE, data_t {"Only HELLO packets are allowed to send on server"});
            log(mazed::log_level::ERROR, "Wrong INFO message received");
          }
          break;

        case ERROR :
          error_message_handler();
          break;

        default :
          message_prepare(ERROR, WRONG_PROTOCOL, UPDATE, data_t {"Unknown protocol message"});
          log(mazed::log_level::ERROR, "Unknown message type received");
          break;
      }
     
      // The timeout must be set before the ASIO loop continues and the ASIO mutex is unlocked: 
      asio_mutex_.lock();
      {
        timeout_set();
        asio_continue_.notify_one();    // Waking up the ASIO LOOP, this will effectively send any prepared message.
      }
      asio_mutex_.unlock();
      
      // Wait for next request for action - MESSAGE or TIMEOUT:
      action_req_.wait(action_lock);
      timeout_stop();

      run_mutex_.lock();
    }
    run_mutex_.unlock();

    return;
  }}}

  
  /**
   * Terminates the whole client's processing loop by notifying all involved threads that they should terminate. Be
   * careful when editing, risk of DEADLOCK are very high!
   */
  void client_handler::terminate()
  {{{
    run_mutex_.lock_upgrade();
    {
      run_ = false;
      action_req_.notify_one();
      asio_continue_.notify_one();
    }
    run_mutex_.unlock_upgrade();

    return;
  }}}

  // // // // // // // // // // // // //

  /**
   * Run the handshake procedure and informs the run_processing() member function of it's result.
   *
   * @return 'true' on success handshake | 'false' upon failure.
   */
  bool client_handler::handshake_success()
  {{{
    // Acquire lock on ACTION REQUEST mutex before the ASIO RECEIVE can (by using initial barrier):
    boost::unique_lock<boost::mutex> action_lock(action_req_mutex_);
    init_barrier_.wait();
  
    // Set the timeout and wait for message arrival/timeout:
    timeout_set();
    action_req_.wait(action_lock);
    timeout_stop();

    run_mutex_.lock();
    if(run_ == false) {
      run_mutex_.unlock();
      return false;                     // Timeout has passed, inform the caller.
    }
    run_mutex_.unlock();
    
    // Test the handshake message, the ASIO RECEIVE handler already verified there's only 1 message:
    if (message_in_.type != CTRL || message_in_.ctrl_type != SYN || message_in_.status != QUERY) {
      message_prepare(CTRL, SYN, NACK);
      async_send(message_out_);

      message_prepare(ERROR, WRONG_PROTOCOL, UPDATE, data_t {"Unknown HANDSHAKE protocol"});
      async_send(message_out_);

      log(mazed::log_level::ERROR, "Handshake was unsuccessful");
      return false;
    }

    // Prepare the response message and notify ASIO LOOP to send it:
    message_prepare(CTRL, SYN, ACK);

    asio_mutex_.lock();
    {
      asio_continue_.notify_one();
    }
    asio_mutex_.unlock();

    return true;
  }}}

  // // // // // // // // // // // // //

  /**
   * Starts the TIMEOUT check cycle. The run of the loop itself doesn't start until the all threads have reached the
   * initial barrier.
   */
  void client_handler::start_timeout()
  {{{
    timeout_.expires_at(boost::posix_time::pos_infin);
    timeout_.async_wait(boost::bind(&client_handler::check_timeout, this, asio::placeholders::error));

    init_barrier_.wait();
    timeout_io_service_.run();

    return;
  }}}

  
  /**
   * TIMEOUT check handler. Terminates the whole processing upon error or when the TIMEOUT has passed. Starts a new
   * asynchronous wait in case none of previous conditions are met.
   */
  void client_handler::check_timeout(const boost::system::error_code &error)
  {{{
    run_mutex_.lock();
    if (run_ == true) {
      run_mutex_.unlock();

      switch (error.value()) {
        // Actual TIMEOUT was cancelled and new one was set (TIMEOUT was UPDATED):
        case boost::system::errc::operation_canceled :
          timeout_.async_wait(boost::bind(&client_handler::check_timeout, this, asio::placeholders::error));
          return;
        
        // TIMEOUT has expired - inform the client:
        case boost::system::errc::success :
          message_prepare(ERROR, TIMEOUT, UPDATE, data_t {"Your connection has timed out"});
          async_send(message_out_);
          break;
        
        // Different error - log it & inform the client:
        default :
          log(mazed::log_level::ERROR, error.message().c_str());
          message_prepare(ERROR, SERVER_ERROR, UPDATE, data_t {"Unknown error occured"});
          async_send(message_out_);
          break;
      }
      
      terminate();                      // Terminating upon TIMEOUT or ERROR.
      return;
    }
    else {
      run_mutex_.unlock();              // The processing is finished, nothing more to do.
      return;
    }
  }}}
  

  /**
   * Inline function for setting new TIMEOUT expiration for new waiting upon client's message arrival.
   */
  inline void client_handler::timeout_set()
  {{{
    timeout_.expires_from_now(boost::posix_time::milliseconds(std::get<MAX_PING>(settings_)));
    return;
  }}}


  /**
   * Inline function for postponing the TIMEOUT to infinity so we doesn't timeout why processing the client's message.
   * There's possibility it can take some time.
   */
  inline void client_handler::timeout_stop()
  {{{
    timeout_.expires_from_now(boost::posix_time::pos_infin);
    return;
  }}}

  // // // // // // // // // // // // //
 
  /**
   * Start the asynchronous receive/send loop when the initial barrier is passed.
   */  
  void client_handler::start_asio_loop()
  {{{
    init_barrier_.wait();
    asio_loop_receive();
    io_service_.run();

    return;
  }}}


  /**
   * Starts the asynchronous receive with appropriate handler.
   */
  void client_handler::asio_loop_receive()
  {{{
    pu_tcp_connect_->async_read(messages_in_, boost::bind(&client_handler::asio_loop_receive_handler, this,
                                                          asio::placeholders::error));
    return;
  }}}


  /**
   * Handles the incoming message and prepares it for use by the main loop. Terminates the processing upon error.
   */
  void client_handler::asio_loop_receive_handler(const boost::system::error_code &error)
  {{{
    if (error) {
      switch (error.value()) {
        case boost::asio::error::eof :
          log(mazed::log_level::INFO, "Connection has been closed by client");
          break;

        case boost::asio::error::operation_aborted :
          log(mazed::log_level::INFO, "Client's connection has timed out");
          break;

        default :
          log(mazed::log_level::ERROR, error.message().c_str());
          break;
      }

      terminate();
      return;
    }
    
    // Acquiring ASIO mutex lock so we don't compete with other ASIO single run receive handler.
    boost::unique_lock<boost::mutex> asio_lock(asio_mutex_);

    // Testing the message received - the protocol expects only one actual message in client's request:
    if (messages_in_.size() != 1) {
      if (messages_in_.size() == 0) {
        message_prepare(ERROR, EMPTY_MESSAGE, UPDATE, data_t {"Empty message received"});
        log(mazed::log_level::ERROR, "Message with no content received");
      }
      else {
        message_prepare(ERROR, MULTIPLE_MESSAGES, UPDATE, data_t {"Multiple messages received"});
        log(mazed::log_level::ERROR, "Multiple messages received");
      }
      
      async_send(message_out_);         // Inform the client.
      asio_loop_receive();              // We're not processing wrong messages, start receiving again!
      return;
    }

    action_req_mutex_.lock();
    {
      message_in_ = messages_in_[0];    // Store the message into temporary storage so we don't block next message.
      action_req_.notify_one();         // Inform the main loop.
    }
    action_req_mutex_.unlock();

    // Check if we should continue so we don't deadlock:
    run_mutex_.lock();
    if (run_ == false) {
      run_mutex_.unlock();
      return;
    }
    run_mutex_.unlock();

    asio_continue_.wait(asio_lock);     // Wait until the messages to be sent isn't ready.

    // Check again to avoid sending on already closed socket:
    run_mutex_.lock();
    if (run_ == false) {
      run_mutex_.unlock();
      return;
    }
    run_mutex_.unlock();
    
    asio_loop_send();

    return;
  }}}


  /**
   * Send the prepared message to the client. This member function synchronizes with single use send function.
   */
  void client_handler::asio_loop_send()
  {{{
    output_mutex_.lock();
    {
      messages_out_[0] = message_out_;
      pu_tcp_connect_->async_write(messages_out_, boost::bind(&client_handler::asio_loop_send_handler, this,
                                                              asio::placeholders::error));
    }
    output_mutex_.unlock();

    return;
  }}}


  /**
   * Handler for sent message. By default only registers receive of next message. Terminates the processing upon error.
   */
  void client_handler::asio_loop_send_handler(const boost::system::error_code &error)
  {{{
    if (error) {
      message_prepare(ERROR, SERVER_ERROR, UPDATE, data_t {"Unknown error occured"});
      async_send(message_out_);

      log(mazed::log_level::ERROR, error.message().c_str());
      terminate();

      return;
    }

    asio_loop_receive();
    return;
  }}}

  // // // // // // // // // // // // //

  /**
   * Single use ASYNC receive from a client. Otherwise this is the same as asio_loop_receive() member function.
   */
  void client_handler::async_receive()
  {{{
    pu_tcp_connect_->async_read(messages_in_, boost::bind(&client_handler::async_receive_handler, this,
                                                          asio::placeholders::error));
    return;
  }}}


  /**
   * Same handler as async_loop_receive_handler(), except it doesn't start the wait for sending a response to the client
   * back.
   */
  void client_handler::async_receive_handler(const boost::system::error_code &error)
  {{{
    if (error) {
      switch (error.value()) {
        case boost::asio::error::eof :
          log(mazed::log_level::INFO, "Connection has been closed by client");
          break;

        case boost::asio::error::operation_aborted :
          log(mazed::log_level::INFO, "Client's connection has timed out");
          break;

        default :
          log(mazed::log_level::ERROR, error.message().c_str());
          break;
      }

      terminate();
      return;
    }
    
    // Acquiring ASIO mutex lock so we don't compete with other ASIO single run receive handler.
    boost::unique_lock<boost::mutex> asio_lock(asio_mutex_);

    // Testing the message received - the protocol expects only one actual message in client's request:
    if (messages_in_.size() != 1) {
      if (messages_in_.size() == 0) {
        message_prepare(ERROR, EMPTY_MESSAGE, UPDATE, data_t {"Empty message received"});
        log(mazed::log_level::ERROR, "Message with no content received");
      }
      else {
        message_prepare(ERROR, MULTIPLE_MESSAGES, UPDATE, data_t {"Multiple messages received"});
        log(mazed::log_level::ERROR, "Multiple messages received");
      }
      
      async_send(message_out_);         // Inform the client.
      asio_loop_receive();              // We're not processing wrong messages, start receiving again!
      return;
    }

    action_req_mutex_.lock();
    {
      message_in_ = messages_in_[0];    // Store the message into temporary storage so we don't block next message.
      action_req_.notify_one();         // Inform the main loop.
    }
    action_req_mutex_.unlock();

    return;
  }}}


  /**
   * Single use ASYNC send to a client. This function can be used by the game instance, therefore it needs a reference
   * to a message to be send to the client. Otherwise it is the same as async_loop_send(). It uses synchronization so
   * the message aren't break upon sending.
   *
   * @param[in]   msg Message to be sent.
   */
  void client_handler::async_send(protocol::message &msg)
  {{{
    output_mutex_.lock();
    {
      messages_out_[0] = msg;
      pu_tcp_connect_->async_write(messages_out_, boost::bind(&client_handler::empty_handler, this,
                                                              asio::placeholders::error));
    }
    output_mutex_.unlock();

    return;
  }}}


  /**
   * Handler for sent message. By default does nothing. Terminates the processing upon error.
   */
  void client_handler::empty_handler(const boost::system::error_code &error)
  {{{
    if (error) {
      message_prepare(ERROR, SERVER_ERROR, UPDATE, data_t {"Unknown error occured"});
      async_send(message_out_);

      log(mazed::log_level::ERROR, error.message().c_str());
      terminate();
    }

    return;   // Nothing to do - single message send.
  }}}

  // // // // // // // // // // // // //

  /**
   * Handler for SYN message. Because handshake can't be done twice, this member function informs the client about wrong
   * protocol and logs the event, nothing more.
   */
  void client_handler::SYN_handler()
  {{{
    message_prepare(ERROR, WRONG_PROTOCOL, UPDATE, data_t {"Wrong protocol usage - SYN received again"});
    log(mazed::log_level::ERROR, "SYN message received again");
    return;
  }}}


  void client_handler::FIN_handler()
  {{{
    return;
  }}}

  
  void client_handler::LOGIN_OR_CREATE_USER_handler()
  {{{
    return;
  }}}

  
  void client_handler::SET_NICK_handler()
  {{{
    return;
  }}}

  
  /**
   * Handles the client's request for displaying available mazes to play.
   */
  void client_handler::LIST_MAZES_handler()
  {{{
    std::vector<std::string> mazes = ps_shared_res_->p_mazes_manager->list_mazes();

    if (mazes.size() == 0) {
      log(mazed::log_level::ERROR, "No available mazes to load/play");
      message_prepare(ERROR, SERVER_ERROR_INFO, UPDATE, data_t {"Server is missing mazes for loading/playing"});
    }
    else {
      message_prepare(CTRL, LIST_MAZES, ACK, mazes);
    }

    return;
  }}}

  
  /**
   * Handles the client's request for displaying available saves to load.
   */
  void client_handler::LIST_SAVES_handler()
  {{{
    message_prepare(CTRL, LIST_SAVES, ACK, ps_shared_res_->p_mazes_manager->list_saves());

    return;
  }}}


  void client_handler::LIST_RUNNING_handler()
  {{{
    return;
  }}}


  void client_handler::CREATE_GAME_handler()
  {{{
    if (p_instance_ != NULL) {
      message_prepare(ERROR, ALREADY_IN_GAME, UPDATE, data_t {"Game already created, terminate it first"});
      return;
    }

    p_maze_ = ps_shared_res_->p_mazes_manager->load_maze(message_in_.data[0]);

    if (p_maze_ == NULL) {
      message_prepare(ERROR, MAZE_BROKEN, UPDATE, data_t {"The maze couldn't be loaded because it's not valid"});
      log(mazed::log_level::ERROR, "Failed to load broken maze");
      return;
    }

    p_instance_ = new game::instance(p_maze_, ps_shared_res_, this);
    p_player_ = new game::player(player_UID_, player_auth_key_, player_nick_, p_maze_, this);
    
    unsigned char player_num;

    p_maze_->access_mutex_.lock();
    {
      p_maze_->game_owner_ = player_UID_;
      
      p_maze_->players_.lock_upgrade();
      {
        player_num = p_maze_->players_.add_player(p_player);
      }
      p_maze_->players_.unlock_upgrade();
    }
    p_maze_->access_mutex_.unlock();

    p_player_->set_player_number(player_num);

    p_instance_->run();

    p_player_->run();
    message_prepare(CTRL, CREATE_GAME, ACK,
                    data_t {std::to_string(p_player_->port()), player_auth_key_, p_maze_->maze_scheme_});

    return;
  }}}


  void client_handler::LOAD_GAME_handler()
  {{{
    return;
  }}}


  void client_handler::SAVE_GAME_handler()
  {{{
    return;
  }}}


  void client_handler::JOIN_GAME_handler()
  {{{
    return;
  }}}


  void client_handler::LEAVE_GAME_handler()
  {{{
    if (p_instance_ == NULL) {
      if (p_player_ != NULL) {
        p_player_->stop();
        // removing player from maze
        delete p_player_;
        p_player_ = NULL;

        assert(p_maze_ == NULL);

        message_prepare(CTRL, LEAVE_GAME, ACK);
      }
      else {
        message_prepare(ERROR, NO_JOINED_GAME, UPDATE, data_t {"You have not joined any running game"});
      }
    }
    else {
      message_prepare(ERROR, USE_TERMINATE, UPDATE, data_t {"You cannot leave created game, use 'terminate' instead"});
    }
  }}}


  void client_handler::RESTART_GAME_handler()
  {{{
    return;
  }}}


  void client_handler::TERMINATE_GAME_handler()
  {{{
    if (p_instance_ != NULL) {
      p_player_->stop();
      // removing player from maze

      delete p_player_;
      p_player_ = NULL;

      p_instance_->stop();
      // Removing from shared resources
      delete p_instance_;
      p_instance_ = NULL;

      delete p_maze_;
      p_maze_ = NULL;

      message_prepare(CTRL, TERMINATE_GAME, ACK);
    }
    else {
      message_prepare(ERROR, NO_GAME_RUNNING, UPDATE, data_t {"There's no running game which could be terminated"});
    }

    return;
  }}}


  void client_handler::error_message_handler()
  {{{
    message_prepare(ERROR, message_in_.error_type, ACK);

    switch (message_in_.error_type) {
      case WRONG_PROTOCOL :
        log(mazed::log_level::ERROR, "From client: WRONG PROTOCOL message received");
        break;

      case EMPTY_MESSAGE :
        log(mazed::log_level::ERROR, "From client: EMPTY MESSAGE received");
        break;

      case MULTIPLE_MESSAGES :
        log(mazed::log_level::ERROR, "From client: MULTIPLE MESSAGES received");
        break;

      case TIMEOUT :
        message_prepare(ERROR, WRONG_PROTOCOL, UPDATE);
        log(mazed::log_level::ERROR, "From client: Connection TIMEOUT received");
        break;

      case ALREADY_PLAYED :
        message_prepare(ERROR, WRONG_PROTOCOL, UPDATE, data_t {"Wrong protocol usage - ALREADY_PLAYED received"});
        log(mazed::log_level::ERROR, "From client: ALREADY PLAYED received");
        break;

      case UNKNOWN_ERROR :
        log(mazed::log_level::ERROR, "From client: UNKNOWN ERROR received");
        break;

      default :
        message_prepare(ERROR, WRONG_PROTOCOL, UPDATE, data_t {"Unknown version of the protocol"});
        log(mazed::log_level::ERROR, "Error type value overflow");
        break;
    };

    return;
  }}}

  // // // // // // // // // // // // //
  
  /**
   * Prepares the message to be sent [1st overload]. This is overload for CTRL type of message.
   *
   * @param[in]   type        Type of the message to be used. (Used for assertion checking for protocol correctness.)
   * @param[in]   ctrl_type   Specification of the CTRL message.
   * @param[in]   status      Status of the message.
   * @param[in]   data        Additional information of the message, empty by default.
   */
  inline void client_handler::message_prepare(E_type type, E_ctrl_type ctrl_type, E_status status,
                                              std::vector<std::string> data)
  {{{
    assert(type == CTRL);       // Make sure we're sending the right type of message.

    message_out_.type = type;
    message_out_.ctrl_type = ctrl_type;
    message_out_.status = status;
    message_out_.data = data;

    return;
  }}}


  /**
   * Prepares the message to be sent [2nd overload]. This is overload for INFO type of message.
   *
   * @param[in]   type        Type of the message to be used. (Used for assertion checking for protocol correctness.)
   * @param[in]   info_type   Specification of the INFO message.
   * @param[in]   status      Status of the message.
   * @param[in]   data        Additional information of the message, empty by default.
   */
  inline void client_handler::message_prepare(E_type type, E_info_type info_type, E_status status,
                                              std::vector<std::string> data)
  {{{
    assert(type == INFO);       // Make sure we're sending the right type of message.

    message_out_.type = type;
    message_out_.info_type = info_type;
    message_out_.status = status;
    message_out_.data = data;

    return;
  }}}


  /**
   * Prepares the message to be sent [3rd overload]. This is overload for ERROR type of message.
   *
   * @param[in]   type        Type of the message to be used. (Used for assertion checking for protocol correctness.)
   * @param[in]   error_type  Specification of the ERROR message.
   * @param[in]   status      Status of the message.
   * @param[in]   data        Additional information of the message, empty by default.
   */
  inline void client_handler::message_prepare(E_type type, E_error_type error_type, E_status status,
                                              std::vector<std::string> data)
  {{{
    assert(type == ERROR);       // Make sure we're sending the right type of message.

    message_out_.type = type;
    message_out_.error_type = error_type;
    message_out_.status = status;
    message_out_.data = data;

    return;
  }}}


  /**
   * @return String representing actual date and time.
   */
  inline std::string client_handler::date_time_str()
  {{{
    std::ostringstream stream;

    boost::posix_time::ptime date_time(boost::posix_time::microsec_clock::universal_time());

    stream.imbue(dt_format_);
    stream << date_time;
    return stream.str();
  }}}


  /**
   *  Logging function. It's behaviour is controlled by the actual logging level setting.
   *
   *  @param[in]  level      Logging level of the string to be logged.
   *  @param[in]  str        String to be logged.
   */
  void client_handler::log(mazed::log_level level, const char *str)
  {{{
    assert(level > mazed::log_level::NONE);           // Possible wrong usage of log function.

    if (level <std::get<mazed::LOGGING_LEVEL>(settings_)) {
      return;
    }

    log_mutex_.lock();
    {
      log_file_ << date_time_str();
      
      switch (level) {
        case mazed::log_level::ALL :
          log_file_ << " - ALL: ";
          break;

        case mazed::log_level::INFO :
          log_file_ << " - INFO: ";
          break;

        case mazed::log_level::ERROR :
          log_file_ << " - ERROR: ";

        default:
          break;
      }

      log_file_ << str << std::endl;
    }
    log_mutex_.unlock();

    return;
  }}}
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_CL_HANDLER.CC ]********************************************************************************* *
 * ****************************************************************************************************************** */


