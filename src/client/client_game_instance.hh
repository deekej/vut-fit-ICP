/**
 * @file      client_game_instance.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.1
 * @brief     Contains a class declaration of client's side game instance.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF CLIENT_GAME_INSTANCE.HH ]*************************************************************************** *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_CLIENT_GAME_INSTANCE_HH
#define H_GUARD_CLIENT_GAME_INSTANCE_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <memory>
#include <string>

#include <boost/thread.hpp>

#include "../protocol.hh"
#include "client_globals.hh"
#include "client_connections.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ GAME_INSTANCE CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace client {

  class game_instance {
    public:
      bool                              paused {true};

    private:
      client::game_connection           *p_game_conn_ {NULL};

      std::string                       output_string_;
      std::string                       maze_scheme_;
      signed char                       maze_rows_;
      signed char                       maze_cols_;

      protocol::update                  update_in_;
      boost::condition_variable         update_in_new_;
      boost::mutex                      update_in_mutex_;
      boost::mutex                      run_mutex_;
      bool                              run_ {true};

      std::unique_ptr<boost::thread>    pu_thread_;

    // // // // // // // // // // // //
  
      void process_updates();
      inline void update_output_string();

    public:
      game_instance(const std::string IP_address, const std::string port, const std::string auth_key,
                    const std::string maze_scheme, const std::string maze_rows, const std::string maze_cols,
                    boost::condition_variable &mediator_cv, boost::mutex &mediator_mutex,
                    protocol::message &mediator_message_in, bool &mediator_message_flag);
     ~game_instance();
      
      bool run();
      void stop();

      void send_command(const protocol::command &cmd);
      
      std::string get_rows();
      std::string get_cols();
      std::string get_output_string();
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF CLIENT_GAME_INSTANCE.HH ]***************************************************************************** *
 * ****************************************************************************************************************** */

#endif

