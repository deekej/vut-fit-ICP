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

#include <memory>

#include <boost/asio.hpp>
#include <boost/thread.hpp>


/* ****************************************************************************************************************** *
 ~ ~~~[ INSTANCE CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace mazed {
  class client_handler;
  class shared_resources;
}

namespace game {

  class maze;
  
  /**
   * Maze game instance.
   */
  class instance {

      // // // // // // // // // // //

      boost::asio::io_service                                   io_service_;
      boost::asio::deadline_timer                               timer_;
      
      game::maze                                                *p_maze_;
      mazed::shared_resources                                   *p_shared_res_;
      mazed::client_handler                                     *p_cl_handler_;

      std::unique_ptr<boost::thread>                            pu_thread_;
      
      // // // // // // // // // // //

      void run_game();
      void start_game();
      void timeout_loop_handler(const boost::system::error_code& error);
      inline void game_loop();
      
      // // // // // // // // // // //

    public:
      instance(game::maze *maze_ptr, mazed::shared_resources *shared_res_ptr, mazed::client_handler *cl_handler_ptr);
     ~instance();
      game::maze *get_maze();
      void run();
      void stop();
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_GAME_INSTANCE.HH ]****************************************************************************** *
 * ****************************************************************************************************************** */

#endif

