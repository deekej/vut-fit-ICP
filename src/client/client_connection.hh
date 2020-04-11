/**
 * @file      client_connection.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   1.0
 * @brief     Abstract Base Class (ABC) for creating a TCP or UDP connection to the server.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF CLIENT_CONNECTION.HH ]****************************************************************************** *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_CLIENT_CONNECTION_HH
#define H_GUARD_CLIENT_CONNECTION_HH

/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include <memory>
#include <vector>

#include "../protocol.hh"

/* ****************************************************************************************************************** *
 ~ ~~~[ CONNECTION CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace client {
  class connection {
    protected:
      // Connection prerequisites:
      boost::asio::io_service                   io_service_;
      boost::asio::ip::tcp::socket              socket_;
      boost::asio::ip::tcp::endpoint            endpoint_;
      boost::asio::ip::tcp::resolver            resolver_;
      boost::asio::ip::tcp::resolver::query     query_;
      boost::asio::ip::tcp::resolver::iterator  it_endpoint_;
      
      // Messages buffers:
      std::vector<protocol::message>            messages_in_;
      std::vector<protocol::message>            messages_out_;
      
      // Messages timeout timers:
      boost::asio::deadline_timer               timeout_in_;
      boost::asio::deadline_timer               timeout_out_;
      
      // Thread for asynchronous receiving:
      std::unique_ptr<boost::thread>            async_receive_thread_;

      // // // // // // // // // // //

    public:
      virtual bool connect() = 0;
      virtual void disconnect() = 0;
      virtual void async_send(const protocol::message &msg) = 0;
      virtual void async_receive_start() = 0;

      // // // // // // // // // // //

      connection(const std::string &IP_address, const std::string &port) :
        socket_(io_service_), resolver_(io_service_), query_(IP_address, port),
        timeout_in_(io_service_), timeout_out_(io_service_)
      {{{
        it_endpoint_ = resolver_.resolve(query_);
        endpoint_ = *it_endpoint_;
        return;
      }}}

      virtual ~connection()
      {{{
        resolver_.cancel();

        if (socket_.is_open() == true) {
          socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
          socket_.cancel();
          socket_.close();
        }

        io_service_.stop();

        if ((*async_receive_thread_).joinable() == true) {
          (*async_receive_thread_).join();
        }
        
        return;
      }}}
  };
}


/* ****************************************************************************************************************** *
 * ***[ END OF CLIENT_CONNECTION.HH ]******************************************************************************** *
 * ****************************************************************************************************************** */

#endif

