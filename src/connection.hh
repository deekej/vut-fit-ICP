/**
 * @file      connection.hh
 * @author    Christopher M. Kohlhoff (chris@kohlhoff.com)
 * @note      This is example from official Boost website. Slightly modified by Dee'Kej (David Kaspar - xkaspa34).
 * @version   1.0
 * @brief     Header file containing implementation of Boost's serialization over TCP/UDP protocol.
 *
 * @license   Boost Software License, Version 1.0 (http:://www.boost.org/LICENSE_1_0.txt)
 */

/* ****************************************************************************************************************** *
 * ***[ START OF CONNECTION.HH ]************************************************************************************* *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_CONNECTION_HH
#define H_GUARD_CONNECTION_HH

/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <boost/asio.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>


/* ****************************************************************************************************************** *
 ~ ~~~[ CONNECTION IMPLEMENTATION ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace protocol {
  /**
   *  Class for TCP connection. Each message sent using this connection consists of:
   *  @li An 8-byte header containing the length of the serialized data in hexadecimal.
   *  @li The serialized data. 
   */
  class tcp_connection {
      boost::asio::ip::tcp::socket &socket_;    // The socket to be used passed within constructor.
      enum { header_length = 8 };               // The size of a fixed length header.
      std::string outbound_header_;             // Holds an outbound header.
      std::string outbound_data_;               // Holds the outbound data.
      char inbound_header_[header_length];      // Holds an inbound header.
      std::vector<char> inbound_data_;          // Holds the inbound data.

    public:
      tcp_connection(boost::asio::ip::tcp::socket &s) : socket_(s) {}

      boost::asio::ip::tcp::socket &socket()
      {{{
        return socket_;
      }}}


      /**
       *  Asynchronously writes a data structure to the socket.
       *  Requires a handler which will be called upon finished successful data write.
       */
      template <typename T, typename Handler>
      void async_write(const T& t, Handler handler)
      {{{

        // Serializing the data to get their's size:.
        std::ostringstream archive_stream;
        boost::archive::text_oarchive archive(archive_stream);
        archive << t;
        outbound_data_ = archive_stream.str();

        // Header formatting:
        std::ostringstream header_stream;
        header_stream << std::setw(header_length) << std::hex << outbound_data_.size();

        if (!header_stream || header_stream.str().size() != header_length) {
          // Something went wrong, inform the caller:
          boost::system::error_code error(boost::asio::error::invalid_argument);
          socket_.get_io_service().post(boost::bind(handler, error));

          return;
        }

        outbound_header_ = header_stream.str();

        // Writing the serialized data to the socket in single operation via "gather-write":
        std::vector<boost::asio::const_buffer> buffers;
        buffers.push_back(boost::asio::buffer(outbound_header_));
        buffers.push_back(boost::asio::buffer(outbound_data_));
        boost::asio::async_write(socket_, buffers, handler);

        return;
      }}}


      /**
       *  Asynchronous read of data structure from the socket.
       *  Requires a handler which will be called upon data arriving.
       */
      template <typename T, typename Handler>
      void async_read(T& t, Handler handler)
      {{{
        // Read exactly the number of bytes in a header:
        void (tcp_connection::*f)(const boost::system::error_code&, T&, boost::tuple<Handler>)
          = &tcp_connection::handle_read_header<T, Handler>;

        boost::asio::async_read(socket_, boost::asio::buffer(inbound_header_),
                                boost::bind(f, this, boost::asio::placeholders::error, boost::ref(t),
                                            boost::make_tuple(handler)));

        return;
      }}}


      /**
       *  Handles a completed read of a message header.
       */
      template <typename T, typename Handler>
      void handle_read_header(const boost::system::error_code& e, T& t, boost::tuple<Handler> handler)
      {{{
        if (e) {
          boost::get<0>(handler)(e);
        }
        else {
          // Determine the length of the serialized data:
          std::istringstream is(std::string(inbound_header_, header_length));
          std::size_t inbound_data_size = 0;

          if (!(is >> std::hex >> inbound_data_size)) {
            // Header doesn't seem to be valid. Inform the caller:
            boost::system::error_code error(boost::asio::error::invalid_argument);
            boost::get<0>(handler)(error);
            return;
          }

          // Starting an asynchronous call to receive the data:
          inbound_data_.resize(inbound_data_size);

          void (tcp_connection::*f)(const boost::system::error_code&, T&, boost::tuple<Handler>)
            = &tcp_connection::handle_read_data<T, Handler>;

          boost::asio::async_read(socket_, boost::asio::buffer(inbound_data_),
                                  boost::bind(f, this, boost::asio::placeholders::error, boost::ref(t), handler));
        }

        return;
      }}}


      /**
       * Handles a completed read of message data.
       */
      template <typename T, typename Handler>
      void handle_read_data(const boost::system::error_code& e, T& t, boost::tuple<Handler> handler)
      {{{
        if (e) {
          boost::get<0>(handler)(e);
        }
        else {
          // Extract the data structure from the data just received:
          try {
            std::string archive_data(&inbound_data_[0], inbound_data_.size());
            std::istringstream archive_stream(archive_data);
            boost::archive::text_iarchive archive(archive_stream);
            archive >> t;
          }
          catch (std::exception& e) {
            // Unable to decode data:
            boost::system::error_code error(boost::asio::error::invalid_argument);
            boost::get<0>(handler)(error);
            return;
          }

          // Inform caller that data has been received correctly:
          boost::get<0>(handler)(e);
        }
      }}}
  };


  // TODO/FIXME: remove header since it's apparently not needed as said by Christopher.
  /**
   *  Class for UDP connection. Each message sent using this connection consists of:
   *  @li An 8-byte header containing the length of the serialized data in hexadecimal.
   *  @li The serialized data. 
   */
  class udp_connection {
      boost::asio::ip::udp::socket &socket_;    // The socket to be used passed within constructor.
      enum { header_length = 8 };               // The size of a fixed length header.
      std::string outbound_header_;             // Holds an outbound header.
      std::string outbound_data_;               // Holds the outbound data.
      char inbound_header_[header_length];      // Holds an inbound header.
      std::vector<char> inbound_data_;          // Holds the inbound data.

    public:
      udp_connection(boost::asio::ip::udp::socket &s) : socket_(s) {}

      boost::asio::ip::udp::socket &socket()
      {{{
        return socket_;
      }}}


      /**
       *  Asynchronously writes a data structure to the socket.
       *  Requires a handler which will be called upon finished successful data write.
       */
      template <typename T, typename Handler>
      void async_write(const T& t, Handler handler)
      {{{

        // Serializing the data to get their's size:.
        std::ostringstream archive_stream;
        boost::archive::text_oarchive archive(archive_stream);
        archive << t;
        outbound_data_ = archive_stream.str();

        // Header formatting:
        std::ostringstream header_stream;
        header_stream << std::setw(header_length) << std::hex << outbound_data_.size();

        if (!header_stream || header_stream.str().size() != header_length) {
          // Something went wrong, inform the caller:
          boost::system::error_code error(boost::asio::error::invalid_argument);
          socket_.get_io_service().post(boost::bind(handler, error));

          return;
        }

        outbound_header_ = header_stream.str();

        // Writing the serialized data to the socket in single operation via "gather-write":
        std::vector<boost::asio::const_buffer> buffers;
        buffers.push_back(boost::asio::buffer(outbound_header_));
        buffers.push_back(boost::asio::buffer(outbound_data_));
        boost::asio::ip::udp::socket::async_send(socket_, buffers, handler);

        return;
      }}}


      /**
       *  Asynchronous read of data structure from the socket.
       *  Requires a handler which will be called upon data arriving.
       */
      template <typename T, typename Handler>
      void async_read(T& t, Handler handler)
      {{{
        // Read exactly the number of bytes in a header:
        void (udp_connection::*f)(const boost::system::error_code&, T&, boost::tuple<Handler>)
          = &udp_connection::handle_read_header<T, Handler>;

        boost::asio::ip::udp::socket::async_receive(socket_, boost::asio::buffer(inbound_header_),
                                                    boost::bind(f, this, boost::asio::placeholders::error,
                                                    boost::ref(t), boost::make_tuple(handler)));

        return;
      }}}


      /**
       *  Handles a completed read of a message header.
       */
      template <typename T, typename Handler>
      void handle_read_header(const boost::system::error_code& e, T& t, boost::tuple<Handler> handler)
      {{{
        if (e) {
          boost::get<0>(handler)(e);
        }
        else {
          // Determine the length of the serialized data:
          std::istringstream is(std::string(inbound_header_, header_length));
          std::size_t inbound_data_size = 0;

          if (!(is >> std::hex >> inbound_data_size)) {
            // Header doesn't seem to be valid. Inform the caller:
            boost::system::error_code error(boost::asio::error::invalid_argument);
            boost::get<0>(handler)(error);
            return;
          }

          // Starting an asynchronous call to receive the data:
          inbound_data_.resize(inbound_data_size);

          void (udp_connection::*f)(const boost::system::error_code&, T&, boost::tuple<Handler>)
            = &udp_connection::handle_read_data<T, Handler>;

          boost::asio::ip::udp::socket::async_receive(socket_, boost::asio::buffer(inbound_data_),
                                                      boost::bind(f, this, boost::asio::placeholders::error,
                                                                  boost::ref(t), handler));
        }

        return;
      }}}


      /**
       * Handles a completed read of message data.
       */
      template <typename T, typename Handler>
      void handle_read_data(const boost::system::error_code& e, T& t, boost::tuple<Handler> handler)
      {{{
        if (e) {
          boost::get<0>(handler)(e);
        }
        else {
          // Extract the data structure from the data just received:
          try {
            std::string archive_data(&inbound_data_[0], inbound_data_.size());
            std::istringstream archive_stream(archive_data);
            boost::archive::text_iarchive archive(archive_stream);
            archive >> t;
          }
          catch (std::exception& e) {
            // Unable to decode data:
            boost::system::error_code error(boost::asio::error::invalid_argument);
            boost::get<0>(handler)(error);
            return;
          }

          // Inform caller that data has been received correctly:
          boost::get<0>(handler)(e);
        }
      }}}
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF CONNECTION.HH ]*************************************************************************************** *
 * ****************************************************************************************************************** */

#endif

