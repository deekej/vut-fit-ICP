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
#include <boost/thread/thread_guard.hpp>

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
    signals_(io_service, SIGINT, SIGTERM),
    settings_(settings)
  {{{
    // Create a formatting object for the date_time_str():
    dt_format_ = std::locale(std::locale::classic(), new boost::posix_time::time_facet("%Y-%m-%d @ %H:%M:%s"));

    ps_shared_res_ = std::shared_ptr<mazed::shared_resources>(new mazed::shared_resources(settings));

    return;
  }}}


  server::~server()
  {{{
    log(mazed::log_level::INFO, "Server has STOPPED");
    log_file_.close();
    return;
  }}}
  
  // // // // // // // // // // // // //

  /**
   * Runs the server instance itself.
   *
   * We're opening the log file here, not in constructor, to avoid unwanted too-early logging.
   */
  void server::run()
  {{{
    chdir(std::get<mazed::LOG_FOLDER>(settings_).c_str());
    log_file_.open(std::get<mazed::SERVER_LOG_FILE>(settings_), std::ofstream::out | std::ofstream::app);

    if (std::get<mazed::LOGGING_LEVEL>(settings_) != mazed::log_level::NONE) {
      log_file_ << "----------------------------" << std::endl;
    }

    log(mazed::log_level::INFO, "Server is RUNNING");
    
    signals_.async_wait(boost::bind(&server::signals_handler, this));

    boost::thread           thread_starter_loop(boost::bind(&server::thread_starter, this));
    boost::thread_guard<>   thread_starter_guard(thread_starter_loop);
    io_service_.run();
    
    return;
  }}}


  /**
   * Member function for starting of accepting new connection after the previous one was established.
   */
  void server::thread_starter()
  {{{
    boost::unique_lock<boost::mutex> lock(connection_mutex_);

    run_mutex_.lock();
    while (run_ == true) {
      run_mutex_.unlock();
      
      // Launch a new connection thread and detach it, it will start accept automatically:
      boost::thread connection_receive(boost::bind(&server::connection_thread, this));
      connection_receive.detach();
      
      // Wait for notification to create a new connection:
      new_connection_.wait(lock);
      connect_ID_++;

      run_mutex_.lock();
    }
    run_mutex_.unlock();

    return;
  }}}


  /**
   * Creates a new instance of server_connection which creates it's own io_service, socket and acceptor. It notifies the
   * thread_started() when it should create a new thread for new connection again.
   */
  void server::connection_thread()
  {{{
    std::unique_ptr<mazed::server_connection> pu_connect (new mazed::server_connection(settings_, this));
    pu_connect->run();
    return;
  }}}


  /**
   * Handles the SIGINT or SIGTERM signal, so the server daemon is properly shutdown.
   */
  void server::signals_handler()
  {{{
    run_mutex_.lock();
    {
      run_ = false;
      new_connection_.notify_one();
    }
    run_mutex_.unlock();

    io_service_.stop();
    return;
  }}}

  // // // // // // // // // // // // //

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
   *  Logging function. It's behaviour is controlled by the actual logging level setting.
   *
   *  @param[in]  level Logging level of the string to be logged.
   *  @param[in]  str String to be logged.
   */
  void server::log(mazed::log_level level, const char *str)
  {{{
    assert(level > mazed::log_level::NONE);     // Possible wrong usage of log function.

    if (level < std::get<mazed::LOGGING_LEVEL>(settings_)) {
      return;                                   // Not requested level of logging, nothing to do.
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
  void server::log_connect_new(unsigned connect_ID)
  {{{
    std::stringstream info;
    info << "Connection #" << connect_ID << " established";
    log(mazed::log_level::INFO, info.str().c_str());

    return;
  }}}


  /**
   * Wrapper function for logging finished connection.
   **/
  void server::log_connect_close(unsigned connect_ID)
  {{{
    std::stringstream info;
    info << "Connection #" << connect_ID << " terminated";
    log(mazed::log_level::INFO, info.str().c_str());

    return;
  }}}
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_SERVER.HH ]************************************************************************************* *
 * ****************************************************************************************************************** */

