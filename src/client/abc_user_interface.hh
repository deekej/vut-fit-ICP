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
        TAKE_OPEN,
        PAUSE_CONTINUE,
        LIST_GAMES,
        LIST_RUNNING_GAMES,
        GAME_START,
        GAME_RESTART,
        GAME_TERMINATE,
        GAME_JOIN,
        GAME_LEAVE,
        GAME_LOAD_LAST,
        GAME_LOAD_GAME,
        GAME_SAVE,
        // GAME_SHOW_STATS,
        SET_NICK,
      };

    protected:
      boost::condition_variable                 &action_req_;
      boost::mutex                              &action_req_mutex_;
      boost::barrier                            &init_barrier_;
      
      enum E_user_command                       &command_;
      std::string                               &additional_data_;

      // // // // // // // // // // //

    public:
      virtual void start() = 0;
      virtual void display(std::string &message) = 0;
      virtual void stop() = 0;

      // // // // // // // // // // //

      user_interface(boost::condition_variable &action_req, boost::mutex &action_req_mutex, boost::barrier barrier,
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

