
#include <memory>
#include <string>
#include <sstream>

#include <boost/date_time.hpp>

#include "mazed_server.hh"

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
  {
    dt_format_ = std::locale(std::locale::classic(), new boost::posix_time::time_facet("%Y-%m-%d @ %H:%M:%s"));

    return;
  }

  server::~server()
  {
    log(mazed::log_level::INFO, "Server has STOPPED");
    log_file_.close();
    return;
  }
  
  void mazed::server::run()
  {
    chdir(std::get<mazed::LOG_FOLDER>(settings_).c_str());
    log_file_.open(std::get<mazed::SERVER_LOG_FILE>(settings_), std::ofstream::out | std::ofstream::app);

    log(mazed::log_level::ERROR, "----------------------------");
    log(mazed::log_level::INFO, "Server is RUNNING");

    start_signal_wait();
    start_accept();

    io_service_.run();
    
    return;
  }

  void mazed::server::start_signal_wait()
  {
    signal_.async_wait(boost::bind(&mazed::server::handle_signal_wait, this));
    return;
  }

  void mazed::server::handle_signal_wait()
  {
    if (acceptor_.is_open()) {
      int status = 0;

      while (waitpid(-1, &status, WNOHANG) > 0) {
        ;
      }

      start_signal_wait();
    }
  }

  void mazed::server::start_accept()
  {
    acceptor_.async_accept(socket_, boost::bind(&mazed::server::handle_accept, this, _1));
    
    return;
  }

  void mazed::server::handle_accept(const boost::system::error_code &error)
  {
    if (error == false) {
      log_connect_new();

      io_service_.notify_fork(boost::asio::io_service::fork_prepare);

      if (fork() == 0) {

        io_service_.notify_fork(boost::asio::io_service::fork_child);

        acceptor_.close();

        signal_.cancel();

        mazed::client_handler *p_handler = new mazed::client_handler(socket_, io_service_, settings_, connect_ID_);

        p_handler->run();

        delete p_handler;

        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        socket_.close();

        log_connect_close();

        exit(exit_codes::NO_ERROR);
      }
      else {
        io_service_.notify_fork(boost::asio::io_service::fork_parent);
        
        socket_.close();
        connect_ID_++;

        start_accept();
      }
    }
    else {
      std::string err = "Accept failed - ";
      log(mazed::log_level::ERROR, err.append(error.message()).c_str());
      start_accept();
    }

    return;
  }

  inline std::string mazed::server::date_time_str()
  {
    std::ostringstream stream;

    boost::posix_time::ptime date_time(boost::posix_time::microsec_clock::universal_time());

    stream.imbue(dt_format_);
    stream << date_time;
    return stream.str();
  }

  void mazed::server::log(mazed::log_level level, const char *str)
  {
    assert(level > mazed::log_level::NONE);     // Possible wrong usage of log function.

    if (level < std::get<mazed::LOGGING_LEVEL>(settings_)) {
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
          break;

        default:
          break;
      }

      log_file_ << str << std::endl;
    }
    log_mutex_.unlock();

    return;
  }

  void mazed::server::log_connect_new()
  {
    std::stringstream info;
    info << "Connection #" << connect_ID_ << " established";
    log(mazed::log_level::INFO, info.str().c_str());

    return;
  }

  void mazed::server::log_connect_close()
  {
    std::stringstream info;
    info << "Connection #" << connect_ID_ << " terminated";
    log(mazed::log_level::INFO, info.str().c_str());

    return;
  }
}
