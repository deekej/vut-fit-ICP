/**
 * @file      mazed_game_instance.cc
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.1
 * @brief     Member function implementations of game::instance class.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_GAME_INSTANCE.CC ]**************************************************************************** *
 * ****************************************************************************************************************** */


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include "mazed_cl_handler.hh"
#include "mazed_shared_resources.hh"

#include "mazed_game_globals.hh"
#include "mazed_game_maze.hh"
#include "mazed_game_guardian.hh"
#include "mazed_game_player.hh"
#include "../protocol.hh"

#include "mazed_game_instance.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ MEMBER FUNCTIONS IMPLEMENTATIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace game {
  instance::instance(game::maze *maze_ptr, mazed::shared_resources *shared_res_ptr,
                     mazed::client_handler *cl_handler_ptr) : 
    timer_(io_service_), p_maze_{maze_ptr}, p_shared_res_{shared_res_ptr}, p_cl_handler_{cl_handler_ptr}
  {{{
    return;
  }}}


  instance::~instance()
  {{{
    timer_.cancel();
    io_service_.stop();

    if (pu_thread_ && (*pu_thread_).joinable() == true) {
      (*pu_thread_).join();
    }
    
    return;
  }}}

  
  game::maze *instance::get_maze()
  {{{
    return p_maze_;
  }}}


  void instance::run()
  {{{
    pu_thread_ = std::unique_ptr<boost::thread>(new boost::thread(&instance::run_game, this));
    return;
  }}}


  void instance::stop()
  {{{
    timer_.cancel();
    io_service_.stop();

    if (pu_thread_ && (*pu_thread_).joinable() == true) {
      (*pu_thread_).join();
    }

    io_service_.reset();
    return;
  }}}

  // // // // // // // // // // //

  void instance::run_game()
  {{{
    return;
  }}}
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_GAME_INSTANCE.CC ]****************************************************************************** *
 * ****************************************************************************************************************** */

