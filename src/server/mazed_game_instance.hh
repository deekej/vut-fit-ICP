/**
 * @file      mazed_game_instance.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.1
 * @brief     Contains declaration of the game instance class.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_GAME_INSTANCE.HH ]**************************************************************************** *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_MAZED_GAME_INSTANCE_HH
#define H_GUARD_MAZED_GAME_INSTANCE_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <fstream>
#include <memory>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "../protocol.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ INSTANCE CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace mazed {
  class client_handler;
  class shared_resources;
}

namespace game {

  class maze;
  class player;
  
  /**
   * Maze game instance.
   */
  class instance {

      // // // // // // // // // // //

      boost::asio::io_service                                   io_service_;
      boost::asio::deadline_timer                               timer_;
      
      game::maze                                                *p_maze_;
      mazed::client_handler                                     *p_cl_handler_;
      std::shared_ptr<mazed::shared_resources>                  ps_shared_res_;
      std::list<std::shared_ptr<game::instance>>::iterator      it_self_;
      bool                                                      shared_ {false};

      std::unique_ptr<boost::thread>                            pu_thread_;

      // // // // // // // // // // //
  
      void run_game();
      void start_game();
      void timeout_loop_handler(const boost::system::error_code& error);
      inline void game_loop();
      
      // // // // // // // // // // //

    public:
      instance(game::maze *maze_ptr, std::string game_owner, std::shared_ptr<mazed::shared_resources> ps_shared_res,
               mazed::client_handler *cl_handler_ptr);
     ~instance();

     std::string get_scheme();
     std::string get_rows();
     std::string get_cols();

#if 0
      protocol::E_game_status get_status();
#endif
      
//       bool check_player(game::player *player_ptr);
      bool add_player(game::player *player_ptr);
      void remove_player(game::player *player_ptr);

      std::shared_ptr<game::instance> run();
      bool stop(const std::string user);
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_GAME_INSTANCE.HH ]****************************************************************************** *
 * ****************************************************************************************************************** */

#endif

