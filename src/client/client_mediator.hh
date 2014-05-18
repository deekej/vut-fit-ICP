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

#include "abc_user_interface.hh"
#include "client_interface_terminal.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ MEDIATOR CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */
using command_t = ABC::user_interface::E_user_command;

namespace client {

  /**
   * Provides the master logic for controlling program's flow. It is implemented as a reactive process which upon
   * running starts everything necessary and then while sleeping waits for any asynchronous operations. These operations
   * are serviced by appropriate handlers and process is put back to sleep again. This class uses boost::thread(s), not
   * processes.
   */
  class mediator {
      client::tcp_connection                      *p_tcp_connect_ {NULL};
      // client::udp_connection                      *p_udp_connect_ {NULL};

      ABC::user_interface                         *p_interface_ {NULL};

      protocol::message                           message_in_;
      protocol::message                           message_out_;

      boost::condition_variable                   action_req_;
      boost::mutex                                action_req_mutex_;

      boost::barrier                              interface_barrier_;
      boost::barrier                              connection_barrier_;

      command_t                                   user_command_ {command_t::NONE};
      std::string                                 additional_data_;

      client::settings_tuple                      &settings_;
      client::exit_codes                          retval_ {exit_codes::NO_ERROR};

      bool                                        new_message_flag_ {false};
      bool                                        run_ {true};

      std::vector<std::string>                    available_mazes_;
      std::vector<std::string>                    available_saves_;

      // // // // // // // // // // //

      using pf_input_handler = void (mediator::*)();

      pf_input_handler                            commands_handlers_[ABC::user_interface::USER_COMMANDS_SIZE] {
        &mediator::CMD_NONE_handler,
        &mediator::CMD_LEFT_handler,
        &mediator::CMD_RIGHT_handler,
        &mediator::CMD_UP_handler,
        &mediator::CMD_DOWN_handler,
        &mediator::CMD_STOP_handler,
        &mediator::CMD_TAKE_OPEN_handler,
        &mediator::CMD_PAUSE_CONTINUE_handler,
        &mediator::CMD_LIST_MAZES_handler,
        &mediator::CMD_LIST_SAVES_handler,
        &mediator::CMD_LIST_RUNNING_handler,
        &mediator::CMD_GAME_START_handler,
        &mediator::CMD_GAME_RESTART_handler,
        &mediator::CMD_GAME_TERMINATE_handler,
        &mediator::CMD_GAME_JOIN_handler,
        &mediator::CMD_GAME_LEAVE_handler,
        &mediator::CMD_GAME_LOAD_LAST_handler,
        &mediator::CMD_GAME_LOAD_handler,
        &mediator::CMD_GAME_SAVE_handler,
        &mediator::CMD_GAME_SHOW_STATS_handler,
        &mediator::CMD_SET_NICK_handler,
        &mediator::CMD_NEW_IPv4_ADRESSS_handler,
        &mediator::CMD_NEW_SERVER_PORT_handler,
        &mediator::CMD_RECONNECT_handler,
        &mediator::CMD_DISCONNECT_handler,
        &mediator::CMD_HELP_handler,
        &mediator::CMD_EXIT_handler,
        &mediator::CMD_ERROR_INPUT_STREAM_handler,
      };

      pf_input_handler                            ctrl_message_handlers_[E_CTRL_TYPE_SIZE] {
        &mediator::CTRL_MSG_SYN_handler,
        &mediator::CTRL_MSG_FIN_handler,
        &mediator::CTRL_MSG_LOGIN_OR_CREATE_USER_handler,
        &mediator::CTRL_MSG_SET_NICK_handler,
        &mediator::CTRL_MSG_LIST_MAZES_handler,
        &mediator::CTRL_MSG_LIST_RUNNING_handler,
        &mediator::CTRL_MSG_LIST_SAVES_handler,
        &mediator::CTRL_MSG_CREATE_GAME_handler,
        &mediator::CTRL_MSG_LOAD_GAME_handler,
        &mediator::CTRL_MSG_SAVE_GAME_handler,
        &mediator::CTRL_MSG_JOIN_GAME_handler,
        &mediator::CTRL_MSG_LEAVE_GAME_handler,
        &mediator::CTRL_MSG_RESTART_GAME_handler,
        &mediator::CTRL_MSG_TERMINATE_GAME_handler,
      };

      pf_input_handler                            info_message_handlers_[E_INFO_TYPE_SIZE]{
        &mediator::INFO_MSG_HELLO_handler,
        &mediator::INFO_MSG_LOAD_DATA_handler,
        &mediator::INFO_MSG_GAMEs_DATA_handler,
        &mediator::INFO_MSG_PLAYER_JOINED_handler,
        &mediator::INFO_MSG_PLAYER_LEFT_handler,
        &mediator::INFO_MSG_PLAYER_TIMEOUT_handler,
        &mediator::INFO_MSG_PLAYER_KILLED_handler,
        &mediator::INFO_MSG_PLAYER_GAME_OVER_handler,
        &mediator::INFO_MSG_PLAYER_WIN_handler,
        &mediator::INFO_MSG_GAME_RESTARTED_handler,
        &mediator::INFO_MSG_GAME_TERMINATED_handler,
      };

      // // // // // // // // // // //

      inline void message_send();
      inline void message_prepare(protocol::E_type type, protocol::E_ctrl_type ctrl_type, protocol::E_status status,
                                  std::vector<std::string> data = {""});
      // inline void message_prepare(protocol::E_type type, protocol::E_info_type info_type, protocol::E_status status,
      //                             std::vector<std::string> data = {""});
      inline void message_prepare(protocol::E_type type, protocol::E_error_type error_type, protocol::E_status status,
                                  std::vector<std::string> data = {""});

      // // // // // // // // // // //

      void error_message_handler();

      // // // // // // // // // // //
      
      void display_message_error();
      void display_error(const std::string &str);

      // // // // // // // // // // //

      void CMD_NONE_handler();
      void CMD_LEFT_handler();
      void CMD_RIGHT_handler();
      void CMD_UP_handler();
      void CMD_DOWN_handler();
      void CMD_STOP_handler();
      void CMD_TAKE_OPEN_handler();
      void CMD_PAUSE_CONTINUE_handler();
      void CMD_LIST_MAZES_handler();
      void CMD_LIST_SAVES_handler();
      void CMD_LIST_RUNNING_handler();
      void CMD_GAME_START_handler();
      void CMD_GAME_RESTART_handler();
      void CMD_GAME_TERMINATE_handler();
      void CMD_GAME_JOIN_handler();
      void CMD_GAME_LEAVE_handler();
      void CMD_GAME_LOAD_LAST_handler();
      void CMD_GAME_LOAD_handler();
      void CMD_GAME_SAVE_handler();
      void CMD_GAME_SHOW_STATS_handler();
      void CMD_SET_NICK_handler();
      void CMD_NEW_IPv4_ADRESSS_handler();
      void CMD_NEW_SERVER_PORT_handler();
      void CMD_RECONNECT_handler();
      void CMD_DISCONNECT_handler();
      void CMD_HELP_handler();
      void CMD_EXIT_handler();
      void CMD_ERROR_INPUT_STREAM_handler();

      // // // // // // // // // // //

      void CTRL_MSG_SYN_handler();
      void CTRL_MSG_FIN_handler();
      void CTRL_MSG_LOGIN_OR_CREATE_USER_handler();
      void CTRL_MSG_SET_NICK_handler();
      void CTRL_MSG_LIST_MAZES_handler();
      void CTRL_MSG_LIST_RUNNING_handler();
      void CTRL_MSG_LIST_SAVES_handler();
      void CTRL_MSG_CREATE_GAME_handler();
      void CTRL_MSG_LOAD_GAME_handler();
      void CTRL_MSG_SAVE_GAME_handler();
      void CTRL_MSG_JOIN_GAME_handler();
      void CTRL_MSG_LEAVE_GAME_handler();
      void CTRL_MSG_RESTART_GAME_handler();
      void CTRL_MSG_TERMINATE_GAME_handler();

      // // // // // // // // // // //

      void INFO_MSG_HELLO_handler();
      void INFO_MSG_LOAD_DATA_handler();
      void INFO_MSG_GAMEs_DATA_handler();
      void INFO_MSG_PLAYER_JOINED_handler();
      void INFO_MSG_PLAYER_LEFT_handler();
      void INFO_MSG_PLAYER_TIMEOUT_handler();
      void INFO_MSG_PLAYER_KILLED_handler();
      void INFO_MSG_PLAYER_GAME_OVER_handler();
      void INFO_MSG_PLAYER_WIN_handler();
      void INFO_MSG_GAME_RESTARTED_handler();
      void INFO_MSG_GAME_TERMINATED_handler();

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

