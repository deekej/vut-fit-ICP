/**
 * @file      mazed_server.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.4
 * @brief     Implementation of server daemon itself. Enables clients to connect and start/control the game.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_SERVER.HH ]*********************************************************************************** *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_MAZED_SERVER_HH
#define H_GUARD_MAZED_SERVER_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <fstream>
#include <list>
#include <locale>
#include <string>
#include <utility>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/date_time.hpp>

#include "mazed_globals.hh"
#include "mazed_server_connection.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ SERVER CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace asio = boost::asio;

namespace mazed {
  class server_connection;
  
  /**
   * Server class which upon running as daemon launches accepting of new connections.
   */
  class server {
      friend class mazed::server_connection;

      asio::io_service                  &io_service_;
      asio::signal_set                  signals_;
      
      boost::condition_variable         new_connection_;
      boost::mutex                      connection_mutex_;
      boost::mutex                      run_mutex_;
      boost::mutex                      log_mutex_;

      std::ofstream                     log_file_;
      std::locale                       dt_format_;

      mazed::settings_tuple             &settings_;

      unsigned connect_ID_              {1};
      bool run_                         {true};

    public:
       server(boost::asio::io_service &io_service, mazed::settings_tuple &settings);
      ~server();

      void run();

    private:
      void thread_starter();
      void connection_thread();

      void signals_handler();

      inline std::string date_time_str();

      void log(mazed::log_level level, const char *str);
      void log_connect_new(unsigned connect_ID);
      void log_connect_close(unsigned connect_ID);
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_SERVER.HH ]************************************************************************************* *
 * ****************************************************************************************************************** */

#endif
