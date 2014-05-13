/**
 * @file      client_mediator.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.1
 * @brief     Contains a class declarations which provides master functionality for the client.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF CLIENT_MEDIATOR.HH ]******************************************************************************** *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_CLIENT_MEDIATOR_HH
#define H_GUARD_CLIENT_MEDIATOR_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <boost/thread.hpp>

#include "client_globals.hh"
#include "client_connections.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ MEDIATOR CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace client {

  /**
   * Provides the master logic for controlling program's flow. It is implemented as a reactive process which upon
   * running starts everything necessary and then while sleeping waits for any asynchronous operations. These operations
   * are serviced by appropriate handlers and process is put back to sleep again. This class uses boost::thread(s), not
   * processes.
   */
  class mediator {
      client::tcp_connection            *p_tcp_connect_ {NULL};
      // client::udp_connection            *p_udp_connect_ {NULL};

      protocol::message                 message_in_;
      protocol::message                 message_out_;

      boost::condition_variable         action_req_;
      boost::mutex                      action_req_mutex_;

      bool                              new_message_flag_ {false};
      bool                              user_input_flag_ {false};

      boost::barrier                    terminal_barrier_;
      boost::barrier                    connection_barrier_;

      client::settings_tuple            &settings_;

    public:
      mediator(client::settings_tuple &settings);
     ~mediator();
      client::exit_codes run();
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF CLIENT_MEDIATOR.HH ]********************************************************************************** *
 * ****************************************************************************************************************** */

#endif

