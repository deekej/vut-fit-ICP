/**
 * @file      mazed_server_connection.cc
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.4
 * @brief     Implementations of member functions of server_connection class.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_SERVER_CONNECTION.CC ]************************************************************************ *
 * ****************************************************************************************************************** */


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include "mazed_server_connection.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ MEMBER FUNCTIONS IMPLEMENTATIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace asio = boost::asio;
namespace ip = boost::asio::ip;
using     tcp = boost::asio::ip::tcp;

namespace mazed {
  server_connection::server_connection(mazed::settings_tuple &settings, mazed::server *p_server_instance) :
    socket_(io_service_),
    acceptor_(io_service_, ip::tcp::endpoint(ip::tcp::v4(), std::get<SERVER_PORT>(settings))),
    settings_(settings),
    p_server_{p_server_instance},
    connect_ID_{p_server_instance->connect_ID_}
  {{{
    return;
  }}}


  server_connection::~server_connection()
  {{{
    io_service_.stop();                         // Making sure the io_service has been stopped in case of signal.
    delete p_handler_;                          // Make sure there's nothing left after we finish.
    p_server_->log_connect_close(connect_ID_);  // Log the connection close even when it is forced by a signal.
    return;
  }}}
  

  /**
   * Starts the waiting for incoming connection.
   */
  void server_connection::run()
  {{{
    start_accept();
    io_service_.run();
    
    return;
  }}}


  /**
   *  Starts accepting (listening) on given port of actual socket.
   */
  void server_connection::start_accept()
  {{{
    acceptor_.async_accept(socket_, boost::bind(&server_connection::handle_accept, this, _1));
    return;
  }}}


  /**
   *  Handler for new incoming connection. It closes the acceptor and notifies the server to start listen for new
   *  incoming connection.
   */
  void server_connection::handle_accept(const boost::system::error_code &error)
  {{{
    acceptor_.close();  // We don't want to accept more connections on current socket.

    if (error) {
      // Error occurred, log the problem and notify the server to start listening again:
      p_server_->log(mazed::log_level::ERROR, error.message().c_str());
      p_server_->new_connection_.notify_one();
      return;
    }

    // Log the connection and notify the server to start listening again:
    p_server_->log_connect_new(connect_ID_);
    p_server_->new_connection_.notify_one();

    // Create new client handler and pass all the requirements so the client can be serviced:
    p_handler_ = new mazed::client_handler(socket_, io_service_, settings_, connect_ID_);
    p_handler_->run();

    return;
  }}}
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_SERVER_CONNECTION.HH ]************************************************************************** *
 * ****************************************************************************************************************** */
