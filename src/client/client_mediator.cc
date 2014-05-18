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
using namespace protocol;

using command_t = ABC::user_interface::E_user_command;

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

    p_interface_->initialize();               // Start the UI.
    interface_barrier_.wait();                // Synchronize with the interface.

    // Try to connect to server:
    if (p_tcp_connect_->connect() == false) {
      error_message_handler();
      message_in_.data.clear();               // Clear any residual content.
    }
    else {
      connection_barrier_.wait();             // Synchronize with established TCP connection thread.
    }
    
    do {
      action_req_.wait(action_lock);          // Wait for asynchronous event occurrence.

      
      // Call the user command handler, if it's needed:
      if (user_command_ != command_t::NONE) {
        (this->*commands_handlers_[user_command_])();
        user_command_ = command_t::NONE;
      }
      
      // Categorize the incoming message and call appropriate handler:
      if (new_message_flag_ == true) {
        new_message_flag_ = false;            // Reseting the flag.

        switch (message_in_.type) {
          case CTRL :
            if (message_in_.ctrl_type >= 0 && message_in_.ctrl_type < E_CTRL_TYPE_SIZE) {
              (this->*ctrl_message_handlers_[message_in_.ctrl_type])();
            }
            else {
              // TODO: send message about wrong protocol
              display_error("Server is using wrong version of protocol, disconnecting...");
              p_tcp_connect_->disconnect();
            }
            break;

          case INFO :
            if (message_in_.info_type >= 0 && message_in_.info_type < E_INFO_TYPE_SIZE) {
              (this->*info_message_handlers_[message_in_.info_type])();
            }
            else {
              // TODO: send message about wrong protocol
              display_error("Server is using wrong version of protocol, disconnecting...");
              p_tcp_connect_->disconnect();
            }
            break;

          case ERROR :
            error_message_handler();
            break;
        }

        message_in_.data.clear();             // Clear any residual content.
      }

    } while (run_ == true);

    return retval_;
  }}}

  // // // // // // // // // // // //

  /**
   * Wrapping function for sending prepared message to the server.
   */
  inline void mediator::message_send()
  {{{
    assert(p_tcp_connect_ != NULL);

    p_tcp_connect_->async_send(message_out_);

    return;
  }}}

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
    assert(type == CTRL);               // Make sure we're sending the right type of message.

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
    assert(type == INFO);               // Make sure we're sending the right type of message.

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
    assert(type == ERROR);              // Make sure we're sending the right type of message.

    message_out_.type = type;
    message_out_.error_type = error_type;
    message_out_.status = status;
    message_out_.data = data;

    return;
  }}}

  // // // // // // // // // // // //
  
  /**
   * Displays the appropriate error message to the user and makes proper disconnect on client side if necessary.
   */
  void mediator::error_message_handler()
  {{{

    switch (message_in_.error_type) {
      case WRONG_PROTOCOL :
      case EMPTY_MESSAGE :
      case MULTIPLE_MESSAGES :
      case REJECTED_CONNECTION :
      case CONNECTION_CLOSED :
      case TIMEOUT :
      case HANDSHAKE :
      case ALREADY_PLAYED :
      case SERVER_ERROR :
      case SERVER_ERROR_INFO :
      case UNKNOWN_ERROR :
        p_tcp_connect_->disconnect();
        display_message_error();
        p_interface_->display_message("NOTE:  You can try to reconnect to server by writing 'reconnect' or"
                                      "\n\t    write 'quit' or 'exit' to end the program.");
        break;

      case CONNECTION_FAILED :
        display_message_error();
        p_interface_->display_message("NOTE:  You can specify the new IP address/port and try to connect again by using"
                                      "\n\t    the 'reconnect' feature. Write 'help' to see the available commands.");
        break;
      default :
        display_message_error();
        break;
    }
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
      
      if (message_in_.status == LOCAL) {
        p_interface_->display_message("ERROR: " + error_msg);
      }
      else {
        p_interface_->display_message("SERVER ERROR: " + error_msg);
      }
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


  /**
   * Sends a message to the server to list all available mazes.
   */
  void mediator::CMD_LIST_MAZES_handler()
  {{{
    message_prepare(CTRL, LIST_MAZES, QUERY);
    message_send();

    return;
  }}}


  /**
   * Sends a message to the server to list all available saves.
   */
  void mediator::CMD_LIST_SAVES_handler()
  {{{
    message_prepare(CTRL, LIST_SAVES, QUERY);
    message_send();

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


  void mediator::CMD_SET_SPEED_handler()
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


  void mediator::CMD_NEW_IPv4_ADRESSS_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_NEW_SERVER_PORT_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_RECONNECT_handler()
  {{{
    p_interface_->display_message("Command not implemented yet, sorry.");

    return;
  }}}


  void mediator::CMD_DISCONNECT_handler()
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

  void mediator::CTRL_MSG_SYN_handler()
  {{{
    return;
  }}}


  void mediator::CTRL_MSG_FIN_handler()
  {{{
    return;
  }}}


  void mediator::CTRL_MSG_LOGIN_OR_CREATE_USER_handler()
  {{{
    return;
  }}}


  void mediator::CTRL_MSG_SET_NICK_handler()
  {{{
    return;
  }}}

  // // // // // // // // // // // //
  
  /**
   * Displays the response from the server to request of listing all available mazes. It also stores a copy for later
   * use.
   */
  void mediator::CTRL_MSG_LIST_MAZES_handler()
  {{{
    assert(p_interface_ != NULL);

    available_mazes_ = message_in_.data;        // Make a backup for when user selects a game to play.

    p_interface_->display_message("×--------------------------×");
    p_interface_->display_message("| Mazes available to play: |");
    p_interface_->display_message("×--------------------------×");
    
    for (std::size_t i = 0; i < available_mazes_.size(); i++) {
      p_interface_->display_message("  [" + std::to_string(i + 1) + "] " + available_mazes_[i]);
    }
    
    return;
  }}}


  /**
   * Displays the response from the server to request of listing all available saves. It also stores a copy for later
   * use.
   */
  void mediator::CTRL_MSG_LIST_SAVES_handler()
  {{{
    assert(p_interface_ != NULL);

    available_saves_ = message_in_.data;        // Make a backup for when user selects a save game to load.

    p_interface_->display_message("×--------------------------×");
    p_interface_->display_message("| Saves available to load: |");
    p_interface_->display_message("×--------------------------×");
    
    if (available_saves_.size() == 0) {
      p_interface_->display_message(" · No available saves found...");
      return;
    }

    for (std::size_t i = 0; i < available_saves_.size(); i++) {
      p_interface_->display_message("  [" + std::to_string(i + 1) + "] " + available_saves_[i]);
    }
    
    return;
  }}}


  void mediator::CTRL_MSG_LIST_RUNNING_handler()
  {{{
    return;
  }}}


  void mediator::CTRL_MSG_CREATE_GAME_handler()
  {{{
    return;
  }}}


  void mediator::CTRL_MSG_LOAD_GAME_handler()
  {{{
    return;
  }}}


  void mediator::CTRL_MSG_SAVE_GAME_handler()
  {{{
    return;
  }}}


  void mediator::CTRL_MSG_JOIN_GAME_handler()
  {{{
    return;
  }}}


  void mediator::CTRL_MSG_LEAVE_GAME_handler()
  {{{
    return;
  }}}


  void mediator::CTRL_MSG_RESTART_GAME_handler()
  {{{
    return;
  }}}


  void mediator::CTRL_MSG_TERMINATE_GAME_handler()
  {{{
    return;

  }}}

  // // // // // // // // // // // //

  void mediator::INFO_MSG_HELLO_handler()
  {{{
    return;
  }}}


  void mediator::INFO_MSG_LOAD_DATA_handler()
  {{{
    return;
  }}}


  void mediator::INFO_MSG_GAMEs_DATA_handler()
  {{{
    return;
  }}}


  void mediator::INFO_MSG_PLAYER_JOINED_handler()
  {{{
    return;
  }}}


  void mediator::INFO_MSG_PLAYER_LEFT_handler()
  {{{
    return;
  }}}


  void mediator::INFO_MSG_PLAYER_TIMEOUT_handler()
  {{{
    return;
  }}}


  void mediator::INFO_MSG_PLAYER_KILLED_handler()
  {{{
    return;
  }}}


  void mediator::INFO_MSG_PLAYER_GAME_OVER_handler()
  {{{
    return;
  }}}


  void mediator::INFO_MSG_PLAYER_WIN_handler()
  {{{
    return;
  }}}


  void mediator::INFO_MSG_GAME_RESTARTED_handler()
  {{{
    return;
  }}}


  void mediator::INFO_MSG_GAME_TERMINATED_handler()
  {{{
    return;
  }}}
}


/* ****************************************************************************************************************** *
 * ***[ END OF CLIENT_MEDIATOR.CC ]********************************************************************************** *
 * ****************************************************************************************************************** */

