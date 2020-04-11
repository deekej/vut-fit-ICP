/**
 * @file      abc_user_interface.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   1.0
 * @brief     Abstract Base Class (ABC) for creating a derived classes for USER INTERFACE.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF ABC_USER_INTERFACE.HH ]***************************************************************************** *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_ABC_USER_INTERFACE_HH
#define H_GUARD_ABC_USER_INTERFACE_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <string>

#include <boost/thread.hpp>

#include "client_game_instance.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ USER_INTERFACE CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace ABC {

  /**
   * Abstract base class to be derived for creating a new specific classes of user interface.
   */
  class user_interface {
    public:
      enum E_user_command {
        NONE = 0,
        LEFT,
        RIGHT,
        UP,
        DOWN,
        STOP,
        TAKE_OPEN,
        PAUSE_CONTINUE,
        LIST_MAZES,
        LIST_SAVES,
        LIST_RUNNING,
        GAME_START,
        GAME_RESTART,
        GAME_TERMINATE,
        GAME_JOIN,
        GAME_LEAVE,
        GAME_LOAD_LAST,
        GAME_LOAD,
        GAME_SAVE,
        GAME_SHOW_STATS,
        SET_SPEED,
        SET_NICK,
        NEW_IPv4_ADDRESS,
        NEW_SERVER_PORT,
        RECONNECT,
        DISCONNECT,
        HELP,
        EXIT,
        ERROR_INPUT_STREAM,
        ERROR_TIMER,
      };

    static const std::size_t                    USER_COMMANDS_SIZE {30};

    protected:
      boost::condition_variable                 &action_req_;
      boost::mutex                              &action_req_mutex_;
      boost::barrier                            &init_barrier_;
      
      enum E_user_command                       &command_;
      std::string                               &additional_data_;

      // // // // // // // // // // //

    public:
      virtual void initialize() = 0;
      virtual void display_message(const std::string &message) = 0;
      virtual void terminate() = 0;

      virtual bool maze_run(client::game_instance *instance_ptr, const std::string zoom) = 0;
      virtual void maze_stop() = 0;

      virtual void maze_pause() = 0;
      virtual void maze_continue() = 0;

      // // // // // // // // // // //

      user_interface(boost::condition_variable &action_req, boost::mutex &action_req_mutex, boost::barrier &barrier,
                     enum E_user_command &command_storage, std::string &additional_data_storage) :
        action_req_{action_req}, action_req_mutex_{action_req_mutex}, init_barrier_{barrier},
        command_{command_storage}, additional_data_{additional_data_storage}
      {{{
        return;
      }}}

      virtual ~user_interface()
      {{{
        return;
      }}}
  };
}


/* ****************************************************************************************************************** *
 * ***[ END OF USER_INTERFACE.HH ]*********************************************************************************** *
 * ****************************************************************************************************************** */

#endif

