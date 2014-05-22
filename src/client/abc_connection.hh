/**
 * @file      abc_connection.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   1.0
 * @brief     Abstract Base Class (ABC) for creating a TCP or UDP connection to the server.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF ABC_CONNECTION.HH ]********************************************************************************* *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_ABC_CONNECTION_HH
#define H_GUARD_ABC_CONNECTION_HH


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

namespace ABC {

  /**
   * Abstract base class to be derived for creating a new specialized TCP and UDP connection classes.
   */
  class connection {
    protected:
      // Connection prerequisites:
      boost::asio::io_service                       io_service_;
      boost::asio::ip::tcp::socket                  socket_;
      boost::asio::ip::tcp::endpoint                endpoint_;
      boost::asio::ip::tcp::resolver                resolver_;
      boost::asio::ip::tcp::resolver::query         query_;
      boost::asio::ip::tcp::resolver::iterator      it_endpoint_;
      
      std::unique_ptr<protocol::tcp_serialization>  pu_tcp_connect_;

      // Thread for asynchronous receiving:
      std::unique_ptr<boost::thread>                pu_asio_thread_;

      // // // // // // // // // // //

    public:
      virtual bool connect() = 0;
      virtual bool disconnect() = 0;

      // // // // // // // // // // //

      connection(const std::string &IP_address, const std::string &port) :
        socket_(io_service_), resolver_(io_service_), query_(IP_address, port)
      {{{
        pu_tcp_connect_ = std::unique_ptr<protocol::tcp_serialization>(new protocol::tcp_serialization(socket_));
        return;
      }}}

      virtual ~connection()
      {{{
        resolver_.cancel();

        if (socket_.is_open() == true) {
          boost::system::error_code ignored_error;
          socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_error);
          socket_.cancel();
          socket_.close();
        }

        io_service_.stop();

        if (pu_asio_thread_ && (*pu_asio_thread_).joinable() == true) {
          (*pu_asio_thread_).join();
        }
        
        return;
      }}}
  };
}


/* ****************************************************************************************************************** *
 * ***[ END OF ABC_CONNECTION.HH ]*********************************************************************************** *
 * ****************************************************************************************************************** */

#endif

