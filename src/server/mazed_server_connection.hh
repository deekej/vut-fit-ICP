/**
 * @file      mazed_server_connection.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   1.0
 * @brief     Class for encapsulation of each server connection.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_SERVER_CONNECTION.HH ]************************************************************************ *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_MAZED_SERVER_CONNECTION_HH
#define H_GUARD_MAZED_SERVER_CONNECTION_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "mazed_globals.hh"
#include "mazed_server.hh"
#include "mazed_cl_handler.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ SERVER_CONNECTION CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace asio = boost::asio;
using     tcp = boost::asio::ip::tcp;

namespace mazed {
  class server;                         // Declaration needed because of the cross-references.
  
  /**
   *  This is friend class of mazed::server class used for each client's connection.
   */
  class server_connection {
      asio::io_service                  io_service_;
      tcp::socket                       socket_;
      tcp::acceptor                     acceptor_;

      mazed::settings_tuple             &settings_;
      mazed::server                     *p_server_;

      mazed::client_handler             *p_handler_;

      unsigned connect_ID_;

    public:
       server_connection(mazed::settings_tuple &settings, mazed::server *server_instance);
      ~server_connection();

      void run();

    private:
      void start_accept();
      void handle_accept(const boost::system::error_code &error);
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_SERVER_CONNECTION.HH ]************************************************************************** *
 * ****************************************************************************************************************** */

#endif
