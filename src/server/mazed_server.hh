
#ifndef H_GUARD_SERVER_HH
#define H_GUARD_SERVER_HH

#include <fstream>
#include <iostream>
#include <locale>
#include <list>
#include <memory>
#include <string>
#include <tuple>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/date_time.hpp>

#include <sys/wait.h>

#include "mazed_cl_handler.hh"


namespace asio = boost::asio;
using     tcp = boost::asio::ip::tcp;

namespace mazed {
  class server {
      asio::io_service        &io_service_;

      asio::signal_set        signal_;
      tcp::acceptor           acceptor_;
      tcp::socket             socket_;

      boost::mutex            log_mutex_;
      std::ofstream           log_file_;
      std::locale             dt_format_;

      mazed::settings_tuple   &settings_;

      unsigned connect_ID_    {1};

    public:
       server(boost::asio::io_service &io_service, mazed::settings_tuple &settings);
      ~server();

      void run();

    private:
      void start_signal_wait();
      void handle_signal_wait();
      void start_accept();
      void handle_accept(const boost::system::error_code &error);

      inline std::string date_time_str();

      void log(mazed::log_level level, const char *str);
      void log_connect_new();
      void log_connect_close();
  };
}

#endif
