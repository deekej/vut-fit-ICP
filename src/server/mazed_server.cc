/**
 * @file      mazed_server.cc
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.4
 * @brief     Implementation of server daemon itself. Enables clients to connect and start/control the game.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_SERVER.CC ]*********************************************************************************** *
 * ****************************************************************************************************************** */


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <memory>
#include <string>
#include <sstream>

#include <boost/date_time.hpp>

#include <sys/wait.h>

#include "mazed_server.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ MEMBER FUNCTIONS IMPLEMENTATIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace asio = boost::asio;
namespace ip = boost::asio::ip;
using     tcp = boost::asio::ip::tcp;

namespace mazed {
  server::server(boost::asio::io_service &io_service, mazed::settings_tuple &settings) :
    io_service_(io_service),
    signal_(io_service, SIGCHLD),
    acceptor_(io_service, ip::tcp::endpoint(ip::tcp::v4(), std::get<SERVER_PORT>(settings))),
    socket_(io_service),
    settings_(settings)
  {{{
    // Create a formatting object for logging purposes:
    dt_format_ = std::locale(std::locale::classic(), new boost::posix_time::time_facet("%Y-%m-%d @ %H:%M:%s"));

    return;
  }}}

  server::~server()
  {{{
    log(mazed::log_level::INFO, "Server has STOPPED");
    log_file_.close();
    return;
  }}}
  
  /**
   *  Runs the server instance itself.
   *
   *  We're opening the log file here, not in constructor, to avoid unwanted too-early logging.
   */
  void server::run()
  {{{
    chdir(std::get<mazed::LOG_FOLDER>(settings_).c_str());
    log_file_.open(std::get<mazed::SERVER_LOG_FILE>(settings_), std::ofstream::out | std::ofstream::app);

    log(mazed::log_level::ERROR, "----------------------------");
    log(mazed::log_level::INFO, "Server is RUNNING");

    start_signal_wait();
    start_accept();

    io_service_.run();
    
    return;
  }}}


  /**
   *  Starts waiting on asynchronous SIGCHLD signal and bind a handler for invocation.
   */
  void server::start_signal_wait()
  {{{
    signal_.async_wait(boost::bind(&mazed::server::handle_signal_wait, this));
    return;
  }}}


  /**
   *  Handler for registered SIGCHLD signal.
   */
  void server::handle_signal_wait()
  {{{
    // Checking if the process is parent or not:
    if (acceptor_.is_open()) {
      int status = 0;

      // Reaping completed child processes to avoid zombies:
      while (waitpid(-1, &status, WNOHANG) > 0) {
        ;
      }

      start_signal_wait();
    }
  }}}


  
  /**
   *  Starts new accepting on given port. (Just like a listen() does.)
   */
  void server::start_accept()
  {{{
    acceptor_.async_accept(socket_, boost::bind(&mazed::server::handle_accept, this, _1));
    return;
  }}}


  /**
   *  Handler for new incoming connection. The process forks allowing another non-blocking connection to be received. 
   */
  void server::handle_accept(const boost::system::error_code &error)
  {{{
    if (error == false) {
      log_connect_new();

      // Inform io_service to prepare for forking:
      io_service_.notify_fork(boost::asio::io_service::fork_prepare);

      if (fork() == 0) {
        // Inform io_service fork is finished and that this is the child:
        io_service_.notify_fork(boost::asio::io_service::fork_child);

        // We don't need acceptor or signal handler in the child:
        acceptor_.close();
        signal_.cancel();

        // Create new client handler and pass the opened socket and io_service to that:
        mazed::client_handler *p_handler = new mazed::client_handler(socket_, io_service_, settings_, connect_ID_);

        p_handler->run();             // Run the handler.
        delete p_handler;             // Clean up.

        // Disconnect properly:
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        socket_.close();

        log_connect_close();

        exit(exit_codes::NO_ERROR);   // No more work to do.
      }
      else {
        // Inform io_service fork is finished and that this is the parent:
        io_service_.notify_fork(boost::asio::io_service::fork_parent);
        
        // Close the socket for reuse, child has it's own instance open:
        socket_.close();
        connect_ID_++;

        start_accept();               // Start again.
      }
    }
    else {
      std::string err = "Accept failed - ";
      log(mazed::log_level::ERROR, err.append(error.message()).c_str());
      start_accept();
    }

    return;
  }}}


  /**
   *  @return The string of actual date and time.
   */
  inline std::string server::date_time_str()
  {{{
    std::ostringstream stream;

    boost::posix_time::ptime date_time(boost::posix_time::microsec_clock::universal_time());

    stream.imbue(dt_format_);
    stream << date_time;
    return stream.str();
  }}}

  
  /**
   *  Logging function. It's behaviour is controlled by actual logging level settings.
   *
   *  @param[in]  level Logging level of the string to be logged.
   *  @param[in]  str String to be logged.
   */
  void server::log(mazed::log_level level, const char *str)
  {{{
    assert(level > mazed::log_level::NONE);     // Possible wrong usage of log function.

    if (level < std::get<mazed::LOGGING_LEVEL>(settings_)) {
      return;     // Not requested level of logging, nothing to do.
    }
    
    log_mutex_.lock();                          // Making sure the log message are not interleaved.
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
          break;

        default:
          break;
      }

      log_file_ << str << std::endl;
    }
    log_mutex_.unlock();

    return;
  }}}


  /**
   * Wrapper function for logging new incoming connection.
   **/
  void server::log_connect_new()
  {{{
    std::stringstream info;
    info << "Connection #" << connect_ID_ << " established";
    log(mazed::log_level::INFO, info.str().c_str());

    return;
  }}}

  /**
   * Wrapper function for logging finished connection.
   **/
  void server::log_connect_close()
  {{{
    std::stringstream info;
    info << "Connection #" << connect_ID_ << " terminated";
    log(mazed::log_level::INFO, info.str().c_str());

    return;
  }}}
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_SERVER.HH ]************************************************************************************* *
 * ****************************************************************************************************************** */

