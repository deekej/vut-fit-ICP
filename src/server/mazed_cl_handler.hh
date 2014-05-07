
#ifndef H_GUARD_CLIENT_HANDLER_HH
#define H_GUARD_CLIENT_HANDLER_HH

#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/thread_guard.hpp>

#include <cassert>

#include "../protocol.hh"
#include "mazed_globals.hh"


namespace asio = boost::asio;

using namespace protocol;

namespace mazed {
  class client_handler {
      using tcp = boost::asio::ip::tcp;
      // using pf_message_handler = void (client_handler::*)();

      // pf_message_handler array[1] = {
      //   &client_handler::empty_handler};

      tcp::socket                       &socket_;
      asio::io_service                  &io_service_;

      asio::deadline_timer              timeout_;

      boost::condition_variable         action_req_;
      boost::condition_variable         asio_continue_;
      boost::mutex                      action_req_mutex_;
      boost::mutex                      asio_mutex_;
      boost::mutex                      output_mutex_;
      boost::mutex                      run_mutex_;

      boost::mutex                      log_mutex_;
      std::ofstream                     log_file_;
      std::locale                       dt_format_;

      std::vector<protocol::message>    messages_in_;
      std::vector<protocol::message>    messages_out_;
      protocol::message                 message_in_;
      protocol::message                 message_out_;
      
      std::unique_ptr<protocol::tcp_connection> pu_tcp_connect_;

      mazed::settings_tuple             &settings_;
      
      bool run_                         {true};

    public:
      client_handler(tcp::socket &sckt, asio::io_service &io_serv, mazed::settings_tuple &settings, unsigned conn_num) :
        socket_(sckt), io_service_(io_serv), timeout_(io_serv), settings_(settings)
      {{{
        chdir(std::get<mazed::LOG_FOLDER>(settings_).c_str());

        std::stringstream filename;
        filename << "connection_" << conn_num << ".log";

        log_file_.open(filename.str(), std::ofstream::out | std::ofstream::trunc);
        log(mazed::log_level::INFO, "Client handler has STARTED (with TCP connection inherited)");
 
        pu_tcp_connect_ = std::unique_ptr<protocol::tcp_connection>(new protocol::tcp_connection(sckt));
        log(mazed::log_level::INFO, "New serialization over TCP connection created");

        messages_out_.resize(1);

        return;
      }}}

      ~client_handler()
      {{{
        terminate();

        log(mazed::log_level::INFO, "Client handler is STOPPING");
        log_file_.close();

        return;
      }}}

      void run()
      {{{
        boost::thread           asio_loop_thread(boost::bind(&client_handler::start_asio_loop, this));
        boost::thread_guard<>   asio_loop_guard(asio_loop_thread);

        boost::thread           timeout_thread(boost::bind(&client_handler::start_timeout, this));
        boost::thread_guard<>   timeout_guard(timeout_thread);

        run_processing();
        io_service_.stop();

        return;
      }}}

    private:
      void run_processing()
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
              ctrl_message_handler();
              break;
            
            case INFO :
              info_message_handler();
              break;

            case ERROR :
              error_message_handler();
              break;

            default :
              message_prepare(ERROR, WRONG_PROTOCOL, UPDATE);
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

      // // // // // // // // // // // // // // // // // // // // // // // // // // // //
      
      void ctrl_message_handler()
      {{{
        switch (message_in_.ctrl_type) {
          default :
            break;
        };

        return;
      }}}

      void info_message_handler()
      {{{
        switch (message_in_.info_type) {
          case HELLO :
            if (message_in_.status != UPDATE) {
              message_prepare(ERROR, WRONG_PROTOCOL, UPDATE);
              log(mazed::log_level::ERROR, "Wrong format of HELLO packet received");
              return;
            }
            message_prepare(INFO, HELLO, ACK);
            return;

          default :
            break;
        };

        return;
      }}}

      void error_message_handler()
      {{{
        switch (message_in_.error_type) {
          default :
            break;
        };

        return;
      }}}

      // // // // // // // // // // // // // // // // // // // // // // // // // // // //

      bool handshake_success()
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

      void start_timeout()
      {{{
        timeout_.expires_at(boost::posix_time::pos_infin);
        check_timeout();
        io_service_.run();
        return;
      }}}

      void check_timeout()
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

      inline void timeout_set()
      {{{
        timeout_.expires_from_now(boost::posix_time::milliseconds(std::get<MAX_PING>(settings_)));
        return;
      }}}

      inline void timeout_stop()
      {{{
        timeout_.expires_at(boost::posix_time::pos_infin);
        return;
      }}}
      
      // // // // // // // // // // // // // // // // // // // // // // // // // // // //

      void start_asio_loop()
      {{{
        asio_loop_receive();
        io_service_.run();

        return;
      }}}

      void asio_loop_receive()
      {{{
        pu_tcp_connect_->async_read(messages_in_, boost::bind(&client_handler::asio_loop_receive_handler, this,
                                                              asio::placeholders::error));
        return;
      }}}

      void asio_loop_receive_handler(const boost::system::error_code &error)
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

      void asio_loop_send()
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

      void asio_loop_send_handler(const boost::system::error_code &error)
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

      void async_receive()
      {{{
        pu_tcp_connect_->async_read(messages_in_, boost::bind(&client_handler::async_receive_handler, this));
        return;
      }}}

      void async_receive_handler()
      {{{
        boost::unique_lock<boost::mutex> asio_lock(asio_mutex_);

        if (messages_in_.size() != 1) {
          if (messages_in_.size() == 0) {
            // connection  error - LOG + start reading again
          }
          else {
            // protocol error - LOG + send info about protocol error
          }
        }

        // TODO: FINNISH THIS!
        messages_in_[0] = message_in_;
      }}}

      void async_send(protocol::message &msg)
      {{{
        output_mutex_.lock();
        {
          messages_out_[0] = msg;
          pu_tcp_connect_->async_write(messages_out_, boost::bind(&client_handler::empty_handler, this));
        }
        output_mutex_.unlock();

        return;
      }}}

      void empty_handler()
      {{{
        return;   // Nothing to do - single message send.
      }}}

      // // // // // // // // // // // // // // // // // // // // // // // // // // // //

      void terminate()
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

      inline void message_prepare(E_type type, E_ctrl_type ctrl_type, E_status status, std::string str = "")
      {{{
        assert(type == CTRL);       // Make sure we're sending the right type of message.

        message_out_.type = type;
        message_out_.ctrl_type = ctrl_type;
        message_out_.status = status;
        message_out_.data = str;

        return;
      }}}

      inline void message_prepare(E_type type, E_info_type info_type, E_status status, std::string str = "")
      {{{
        assert(type == INFO);       // Make sure we're sending the right type of message.

        message_out_.type = type;
        message_out_.info_type = info_type;
        message_out_.status = status;
        message_out_.data = str;

        std::stringstream stream;
        stream << "type: " << message_out_.type << " | subtype: " << message_out_.info_type << " | status: ";
        stream << message_out_.status << " | data: " << message_out_.data << std::endl;

        log(mazed::log_level::INFO, stream.str().c_str());

        return;
      }}}

      inline void message_prepare(E_type type, E_error_type error_type, E_status status, std::string str = "")
      {{{
        assert(type == ERROR);       // Make sure we're sending the right type of message.

        message_out_.type = type;
        message_out_.error_type = error_type;
        message_out_.status = status;
        message_out_.data = str;

        return;
      }}}

      inline std::string date_time_str()
      {{{
        std::ostringstream stream;

        boost::posix_time::ptime date_time(boost::posix_time::microsec_clock::universal_time());

        stream.imbue(dt_format_);
        stream << date_time;
        return stream.str();
      }}}

      void log(mazed::log_level level, const char *str)
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

      // TODO: Another auxiliary functions for starting Game instance, saving it, loading it, etc. etc.
  };
}

#endif
