/**
 * @file      client_connections.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.5
 * @brief     Implementations of TCP and UDP connections used by client.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF CLIENT_CONNECTIONS.HH ]***************************************************************************** *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_CLIENT_CONNECTIONS_HH
#define H_GUARD_CLIENT_CONNECTIONS_HH

/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include "abc_connection.hh"
#include "client_globals.hh"

/* ****************************************************************************************************************** *
 ~ ~~~[ CONNECTIONS CLASSES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace client {
  class tcp_connection : public ABC::connection {
      std::unique_ptr<protocol::tcp_serialization>  pu_tcp_connect_;

      protocol::message                             &message_in_;
      boost::condition_variable                     &action_req_;
      boost::mutex                                  &action_req_mutex_;
      bool                                          &new_message_flag_;

      boost::barrier                                &init_barrier_;
      boost::mutex                                  output_mutex_;
      protocol::message                             HELLO_packet_;
      protocol::message                             FIN_packet_;

      client::settings_tuple                        &settings_;

      // // // // // // // // // // //
      
      void communication_start();
      void handshake_send_handler(const boost::system::error_code &error);
      void handshake_receive_handler(const boost::system::error_code &error);

      void start_timeout_in_timer();
      inline void timeout_in_set();
      inline void timeout_in_stop();
      void timeout_in_handler(const boost::system::error_code &error);

      void start_timeout_out_timer();
      inline void timeout_out_reset();
      void timeout_out_handler(const boost::system::error_code &error);

      void async_receive();
      void async_receive_handler(const boost::system::error_code &error);

      void async_send_handler(const boost::system::error_code &error);

      // // // // // // // // // // //

      void asio_loops_start() override;

    public:
      bool connect() override;
      bool disconnect() override;
      void async_send(const protocol::message &msg) override;
      tcp_connection(client::settings_tuple &settings, protocol::message &msg_storage,
                     boost::condition_variable &action_req, boost::mutex &action_req_mutex, bool &flag,
                     boost::barrier &barrier);
  };
}


/* ****************************************************************************************************************** *
 * ***[ END OF CLIENT_CONNECTIONS.HH ]******************************************************************************* *
 * ****************************************************************************************************************** */

#endif

