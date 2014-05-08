
#include <sstream>
#include <string>

#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread_guard.hpp>

#include <cassert>

#include "mazed_cl_handler.hh"

namespace mazed {
  client_handler::client_handler(tcp::socket &socket, asio::io_service &io_service, mazed::settings_tuple &settings,
                                 unsigned connection_num) :
    socket_(socket),
    io_service_(io_service),
    timeout_(io_service),
    settings_(settings)
  {{{
    chdir(std::get<mazed::LOG_FOLDER>(settings_).c_str());

    std::stringstream filename;
    filename << "connection_" << connection_num << ".log";

    log_file_.open(filename.str(), std::ofstream::out | std::ofstream::trunc);
    log(mazed::log_level::INFO, "Client handler has STARTED (with TCP connection inherited)");

    pu_tcp_connect_ = std::unique_ptr<protocol::tcp_connection>(new protocol::tcp_connection(socket));
    log(mazed::log_level::INFO, "New serialization over TCP connection created");

    messages_out_.resize(1);

    return;
  }}}

  client_handler::~client_handler()
  {{{
    terminate();
    io_service_.stop();

    log(mazed::log_level::INFO, "Client handler is STOPPING");
    log_file_.close();

    return;
  }}}

  void client_handler::run()
  {{{
    boost::thread           asio_loop_thread(boost::bind(&client_handler::start_asio_loop, this));
    boost::thread_guard<>   asio_loop_guard(asio_loop_thread);

    boost::thread           timeout_thread(boost::bind(&client_handler::start_timeout, this));
    boost::thread_guard<>   timeout_guard(timeout_thread);

    run_processing();
    io_service_.stop();

    return;
  }}}

  // // // // // // // // // // // // // // // // // // // // // // // // // // // //

  void client_handler::run_processing()
  {{{
    if (handshake_success() == false) {
      return;
    }

    boost::unique_lock<boost::mutex> lock(action_req_mutex_);
    
    timeout_set();
    action_req_.wait(lock);         // Wait for next message.
    timeout_stop();

    run_mutex_.lock();
    while (run_ == true) {
      run_mutex_.unlock();

      switch (message_in_.type) {
        case CTRL :
          if (message_in_.ctrl_type >= 0 && message_in_.ctrl_type < E_CTRL_TYPE_SIZE) {
            (this->*ctrl_message_handlers[message_in_.ctrl_type])();
          }
          else {
            message_prepare(ERROR, WRONG_PROTOCOL, UPDATE);
            log(mazed::log_level::ERROR, "CTRL type value overflow");
          }
          break;
        
        case INFO :
          if (message_in_.info_type == HELLO) {
            message_prepare(INFO, HELLO, ACK);
          }
          else {
            message_prepare(ERROR, WRONG_PROTOCOL, UPDATE);
            log(mazed::log_level::ERROR, "Wrong INFO message received");
          }
          break;

        case ERROR :
          error_message_handler();
          break;

        default :
          message_prepare(ERROR, WRONG_PROTOCOL, UPDATE);
          log(mazed::log_level::ERROR, "Unknown message type received");
          break;
      }
          
      asio_continue_.notify_one();

      timeout_set();
      action_req_.wait(lock);
      timeout_stop();

      run_mutex_.lock();
    }
    run_mutex_.unlock();

    return;
  }}}

  void client_handler::terminate()
  {{{
    run_mutex_.lock();
    {
      run_ = false;
      action_req_.notify_one();
    }
    run_mutex_.unlock();

    return;
  }}}

  // // // // // // // // // // // // // // // // // // // // // // // // // // // //

  bool client_handler::handshake_success()
  {{{
    boost::unique_lock<boost::mutex> lock(action_req_mutex_);

    timeout_set();
    action_req_.wait(lock);

    if (run_ == false) {
      return false;                 // Timeout. Finish - the error message has been already sent.
    }
    
    timeout_stop();

    // Test the handshake, the asio_loop_receive() handler already verified there's only 1 message.
    if (message_in_.type != CTRL || message_in_.ctrl_type != SYN || message_in_.status != QUERY) {
      message_prepare(CTRL, SYN, NACK);
      async_send(message_out_);

      message_prepare(ERROR, WRONG_PROTOCOL, UPDATE);
      async_send(message_out_);

      log(mazed::log_level::ERROR, "Handshake was unsuccessful");
      return false;
    }

    message_prepare(CTRL, SYN, ACK);
    asio_continue_.notify_one();

    return true;
  }}}

  // // // // // // // // // // // // // // // // // // // // // // // // // // // //

  void client_handler::start_timeout()
  {{{
    timeout_.expires_at(boost::posix_time::pos_infin);
    check_timeout();
    io_service_.run();
    return;
  }}}

  void client_handler::check_timeout()
  {{{
    if (timeout_.expires_at() <= asio::deadline_timer::traits_type::now()) {

      message_prepare(ERROR, TIMEOUT, UPDATE);
      async_send(message_out_);
      log(mazed::log_level::INFO, "Client's connection has timed out");

      terminate();

      return;
    }

    timeout_.async_wait(boost::bind(&client_handler::check_timeout, this));
    return;
  }}}

  inline void client_handler::timeout_set()
  {{{
    timeout_.expires_from_now(boost::posix_time::milliseconds(std::get<MAX_PING>(settings_)));
    return;
  }}}

  inline void client_handler::timeout_stop()
  {{{
    timeout_.expires_at(boost::posix_time::pos_infin);
    return;
  }}}

  // // // // // // // // // // // // // // // // // // // // // // // // // // // //
  
  void client_handler::start_asio_loop()
  {{{
    asio_loop_receive();
    io_service_.run();

    return;
  }}}

  void client_handler::asio_loop_receive()
  {{{
    pu_tcp_connect_->async_read(messages_in_, boost::bind(&client_handler::asio_loop_receive_handler, this,
                                                          asio::placeholders::error));
    return;
  }}}

  void client_handler::asio_loop_receive_handler(const boost::system::error_code &error)
  {{{
    if (error) {
      if (error == boost::asio::error::eof) {
        log(mazed::log_level::INFO, "Connection has been closed by client");
      }
      else {
        log(mazed::log_level::ERROR, error.message().c_str());
      }

      terminate();
      return;
    }

    boost::unique_lock<boost::mutex> asio_lock(asio_mutex_);

    if (messages_in_.size() != 1) {
      if (messages_in_.size() == 0) {
        message_prepare(ERROR, EMPTY_MESSAGE, UPDATE);
        log(mazed::log_level::ERROR, "Message with no content received");
      }
      else {
        message_prepare(ERROR, MULTIPLE_MESSAGES, UPDATE);
        log(mazed::log_level::ERROR, "Multiple messages received");
      }
      
      async_send(message_out_);   // Inform the client.
      asio_loop_receive();        // We're not processing wrong messages, start receiving again!
      return;
    }
    
    action_req_mutex_.lock();
    {
      message_in_ = messages_in_[0];
    }
    action_req_mutex_.unlock();

    action_req_.notify_one();
    asio_continue_.wait(asio_lock);

    asio_loop_send();

    return;
  }}}

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

  void client_handler::asio_loop_send_handler(const boost::system::error_code &error)
  {{{
    if (error) {
      log(mazed::log_level::ERROR, error.message().c_str());
      terminate();
      return;
    }

    asio_loop_receive();
    return;
  }}}

  // // // // // // // // // // // // // // // // // // // // // // // // // // // //

  void client_handler::async_receive()
  {{{
    pu_tcp_connect_->async_read(messages_in_, boost::bind(&client_handler::async_receive_handler, this,
                                                          asio::placeholders::error));
    return;
  }}}

  void client_handler::async_receive_handler(const boost::system::error_code &error)
  {{{
    if (error) {
      if (error == boost::asio::error::eof) {
        log(mazed::log_level::INFO, "Connection has been closed by client");
      }
      else {
        log(mazed::log_level::ERROR, error.message().c_str());
      }

      terminate();
      return;
    }

    boost::unique_lock<boost::mutex> asio_lock(asio_mutex_);

    if (messages_in_.size() != 1) {
      if (messages_in_.size() == 0) {
        message_prepare(ERROR, EMPTY_MESSAGE, UPDATE);
        log(mazed::log_level::ERROR, "Message with no content received");
      }
      else {
        message_prepare(ERROR, MULTIPLE_MESSAGES, UPDATE);
        log(mazed::log_level::ERROR, "Multiple messages received");
      }
      
      async_send(message_out_);   // Inform the client.
      asio_loop_receive();        // We're not processing wrong messages, start receiving again!
      return;
    }

    action_req_mutex_.lock();
    {
      message_in_ = messages_in_[0];
    }
    action_req_mutex_.unlock();

    action_req_.notify_one();

    return;
  }}}

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

  void client_handler::empty_handler(const boost::system::error_code &error)
  {{{
    if (error) {
      log(mazed::log_level::ERROR, error.message().c_str());
      terminate();
      return;
    }

    return;   // Nothing to do - single message send.
  }}}

  // // // // // // // // // // // // // // // // // // // // // // // // // // // //

  void client_handler::SYN_handler()
  {{{
    message_prepare(ERROR, WRONG_PROTOCOL, UPDATE);
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

  void client_handler::LIST_SAVES_handler()
  {{{
    return;
  }}}

  void client_handler::LIST_GAMES_handler()
  {{{
    return;
  }}}

  void client_handler::CREATE_GAME_handler()
  {{{
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
    return;
  }}}

  void client_handler::RESTART_GAME_handler()
  {{{
    return;
  }}}

  void client_handler::TERMINATE_GAME_handler()
  {{{
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
        message_prepare(ERROR, WRONG_PROTOCOL, UPDATE);
        log(mazed::log_level::ERROR, "From client: ALREADY PLAYED received");
        break;

      case UNKNOWN_ERROR :
        log(mazed::log_level::ERROR, "From client: UNKNOWN ERROR received");
        break;

      default :
        message_prepare(ERROR, WRONG_PROTOCOL, UPDATE);
        log(mazed::log_level::ERROR, "Error type value overflow");
        break;
    };

    return;
  }}}

  // // // // // // // // // // // // // // // // // // // // // // // // // // // //

  inline void client_handler::message_prepare(E_type type, E_ctrl_type ctrl_type, E_status status, std::string str)
  {{{
    assert(type == CTRL);       // Make sure we're sending the right type of message.

    message_out_.type = type;
    message_out_.ctrl_type = ctrl_type;
    message_out_.status = status;
    message_out_.data = str;

    return;
  }}}

  inline void client_handler::message_prepare(E_type type, E_info_type info_type, E_status status, std::string str)
  {{{
    assert(type == INFO);       // Make sure we're sending the right type of message.

    message_out_.type = type;
    message_out_.info_type = info_type;
    message_out_.status = status;
    message_out_.data = str;

    return;
  }}}

  inline void client_handler::message_prepare(E_type type, E_error_type error_type, E_status status, std::string str)
  {{{
    assert(type == ERROR);       // Make sure we're sending the right type of message.

    message_out_.type = type;
    message_out_.error_type = error_type;
    message_out_.status = status;
    message_out_.data = str;

    return;
  }}}

  inline std::string client_handler::date_time_str()
  {{{
    std::ostringstream stream;

    boost::posix_time::ptime date_time(boost::posix_time::microsec_clock::universal_time());

    stream.imbue(dt_format_);
    stream << date_time;
    return stream.str();
  }}}

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
