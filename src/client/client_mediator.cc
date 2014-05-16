/**
 * @file      client_mediator.cc
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.1
 * @brief     Contains implementations of client::mediator member functions.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF CLIENT_MEDIATOR.CC ]******************************************************************************** *
 * ****************************************************************************************************************** */


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <cassert>
#include <cstdio>

#include "client_mediator.hh"

/* ****************************************************************************************************************** *
 ~ ~~~[ MEMBER FUNCTIONS IMPLEMENTATIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

using command = ABC::user_interface::E_user_command;

namespace client {
  mediator::mediator(client::settings_tuple &settings) :
    interface_barrier_(2), connection_barrier_(2), settings_{settings} 
  {{{
    // Preparing TCP connection:
    p_tcp_connect_ = new client::tcp_connection(settings_, message_in_, action_req_, action_req_mutex_,
                                                new_message_flag_, connection_barrier_);
    
    // Preparing the interface:
    p_interface_ = new client::terminal_interface(action_req_, action_req_mutex_, interface_barrier_, user_command_,
                                                  additional_data_);
    return;
  }}}

  /**
   * Making sure we terminate all properly.
   */
  mediator::~mediator()
  {{{
    p_tcp_connect_->disconnect();
    p_interface_->terminate();

    delete p_tcp_connect_;
    delete p_interface_;

    return;
  }}}

  // // // // // // // // // // // //

  /**
   * Starts the execution of the master logic of the mediator.
   *
   * @return  Exit code to be return to the terminal.
   */
  client::exit_codes mediator::run()
  {{{
    // Acquire initial lock on the action request mutex:
    boost::unique_lock<boost::mutex> action_lock(action_req_mutex_);

    p_interface_->initialize();
    interface_barrier_.wait();                // Synchronize with the interface.
    
    // Try to connect to server:
    if (p_tcp_connect_->connect() == false) {
      display_message_error();
      return exit_codes::E_CONNECT_FAILED;    // Connection fail, nothing more to do.
    }

    connection_barrier_.wait();               // Synchronize with established TCP connection thread.

    do {
      action_req_.wait(action_lock);          // Wait for asynchronous event occurrence.
      
      // Call the user command handler, if it's needed:
      if (user_command_ != command::NONE) {
        (this->*commands_handlers_[user_command_])();
      }
      
      // Categorize the incoming message and call appropriate handler:
      if (new_message_flag_ == true) {
        new_message_flag_ = false;            // Reseting the flag.

        switch (message_in_.type) {
          case protocol::E_type::CTRL :
            ctrl_message_handler();
            break;

          case protocol::E_type::INFO :
            if (message_in_.info_type >= 0 && message_in_.info_type < E_INFO_TYPE_SIZE) {
              (this->*info_message_handlers_[message_in_.info_type])();
            }
            else {
              // TODO: send message about wrong protocol
              display_error("Server is using wrong version of communication protocol, terminating...");
              retval_ = exit_codes::E_WRONG_PROTOCOL;
              run_ = false;
            }
            break;

          case protocol::E_type::ERROR :
            error_message_handler();
            break;
        }
      }

    } while (run_ == true);

    return retval_;
  }}}

  // // // // // // // // // // // //

  /**
   * Prepares the message to be sent [1st overload]. This is overload for CTRL type of message.
   *
   * @param[in]   type        Type of the message to be used. (Used for assertion checking for protocol correctness.)
   * @param[in]   ctrl_type   Specification of the CTRL message.
   * @param[in]   status      Status of the message.
   * @param[in]   data        Additional information of the message, empty by default.
   */
  inline void mediator::message_prepare(protocol::E_type type, protocol::E_ctrl_type ctrl_type,
                                        protocol::E_status status, std::vector<std::string> data)
  {{{
    assert(type == protocol::E_type::CTRL);       // Make sure we're sending the right type of message.

    message_out_.type = type;
    message_out_.ctrl_type = ctrl_type;
    message_out_.status = status;
    message_out_.data = data;

    return;
  }}}

#if 0
  /**
   * Prepares the message to be sent [2nd overload]. This is overload for INFO type of message.
   *
   * @param[in]   type        Type of the message to be used. (Used for assertion checking for protocol correctness.)
   * @param[in]   info_type   Specification of the INFO message.
   * @param[in]   status      Status of the message.
   * @param[in]   data        Additional information of the message, empty by default.
   */
  inline void mediator::message_prepare(protocol::E_type type, protocol::E_info_type info_type,
                                        protocol::E_status status, std::vector<std::string> data)
  {{{
    assert(type == protocol::E_type::INFO);       // Make sure we're sending the right type of message.

    message_out_.type = type;
    message_out_.info_type = info_type;
    message_out_.status = status;
    message_out_.data = data;

    return;
  }}}
#endif

  /**
   * Prepares the message to be sent [3rd overload]. This is overload for ERROR type of message.
   *
   * @param[in]   type        Type of the message to be used. (Used for assertion checking for protocol correctness.)
   * @param[in]   error_type  Specification of the ERROR message.
   * @param[in]   status      Status of the message.
   * @param[in]   data        Additional information of the message, empty by default.
   */
  inline void mediator::message_prepare(protocol::E_type type, protocol::E_error_type error_type,
                                        protocol::E_status status, std::vector<std::string> data)
  {{{
    assert(type == protocol::E_type::ERROR);      // Make sure we're sending the right type of message.

    message_out_.type = type;
    message_out_.error_type = error_type;
    message_out_.status = status;
    message_out_.data = data;

    return;
  }}}

  // // // // // // // // // // // //
  
  /**
   * TODO: This is dummy for now.
   */
  void mediator::ctrl_message_handler()
  {{{
    return;
  }}}


  /**
   * TODO: This is dummy for now.
   */
  void mediator::error_message_handler()
  {{{
    return;
  }}}

  // // // // // // // // // // // //

  /**
   * Displays the error messages generated from server or locally during network communication.
   */
  void mediator::display_message_error()
  {{{
    assert(p_interface_ != NULL);
    
    // There might be more than one error message:
    for (auto error_msg : message_in_.data) {
      if (error_msg.length() == 0 || error_msg == "") {
        continue;
      }

      p_interface_->display_message("ERROR: " + error_msg);
    }

    return;
  }}}


  /**
   * Displays single string as an error message:
   */
  void mediator::display_error(const std::string &str)
  {{{
    assert(p_interface_ != NULL);

    p_interface_->display_message("ERROR: " + str);

    return;
  }}}

  // // // // // // // // // // // //

  void mediator::CMD_NONE_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_LEFT_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_RIGHT_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_UP_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_DOWN_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_STOP_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_TAKE_OPEN_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_PAUSE_CONTINUE_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_LIST_MAZES_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_LIST_SAVES_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_LIST_RUNNING_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_GAME_START_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_GAME_RESTART_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_GAME_TERMINATE_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_GAME_JOIN_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_GAME_LEAVE_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_GAME_LOAD_LAST_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_GAME_LOAD_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_GAME_SAVE_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_GAME_SHOW_STATS_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_SET_NICK_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_EXIT_handler()
  {{{
    // TODO: Terminate other necessary things.
    run_ = false;

    return;
  }}}


  void mediator::CMD_HELP_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_ERROR_INPUT_STREAM_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}

  // // // // // // // // // // // //

  void mediator::MSG_HELLO_handler()
  {{{
    return;
  }}}


  void mediator::MSG_LOAD_DATA_handler()
  {{{
    return;
  }}}


  void mediator::MSG_GAMEs_DATA_handler()
  {{{
    return;
  }}}


  void mediator::MSG_PLAYER_JOINED_handler()
  {{{
    return;
  }}}


  void mediator::MSG_PLAYER_LEFT_handler()
  {{{
    return;
  }}}


  void mediator::MSG_PLAYER_KILLED_handler()
  {{{
    return;
  }}}


  void mediator::MSG_PLAYER_GAME_OVER_handler()
  {{{
    return;
  }}}


  void mediator::MSG_PLAYER_WIN_handler()
  {{{
    return;
  }}}


  void mediator::MSG_GAME_RESTARTED_handler()
  {{{
    return;
  }}}


  void mediator::MSG_GAME_TERMINATED_handler()
  {{{
    return;
  }}}
}


/* ****************************************************************************************************************** *
 * ***[ END OF CLIENT_MEDIATOR.CC ]********************************************************************************** *
 * ****************************************************************************************************************** */

