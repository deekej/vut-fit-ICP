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
    pu_thread_ = std::unique_ptr<boost::thread>(new boost::thread(&instance::start_game, this));
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

  void instance::start_game()
  {{{
    timer_.expires_from_now(boost::posix_time::milliseconds(p_maze_->game_speed_));
    timer_.async_wait(boost::bind(&instance::timeout_loop_handler, this, boost::asio::placeholders::error));
    io_service_.run();
    return;
  }}}


  void instance::timeout_loop_handler(const boost::system::error_code& error)
  {{{
    switch (error.value()) {
      case boost::system::errc::operation_canceled :
        p_cl_handler_->log(mazed::log_level::INFO, "Something is messing with game instance's timer");
        timer_.expires_at(timer_.expires_at() + boost::posix_time::milliseconds(p_maze_->game_speed_));
        timer_.async_wait(boost::bind(&instance::timeout_loop_handler, this, boost::asio::placeholders::error));
        break;

      case boost::system::errc::success :
        game_loop();
        timer_.expires_at(timer_.expires_at() + boost::posix_time::milliseconds(p_maze_->game_speed_));
        timer_.async_wait(boost::bind(&instance::timeout_loop_handler, this, boost::asio::placeholders::error));
        break;

      default :
        p_cl_handler_->log(mazed::log_level::ERROR, error.message().c_str());
        break;
    }

    return;
  }}}


  inline void instance::game_loop()
  {{{
    p_maze_->access_mutex_.lock();
    {

      if (p_maze_->game_run_ == false || p_maze_->game_finished_ == true) {
        p_maze_->access_mutex_.unlock();
        return;
      }

      std::array<player *, GAME_MAX_PLAYERS>::iterator it_players;
      
      p_maze_->players_.lock_upgrade();
      {

        for (it_players = p_maze_->players_.begin(); it_players != p_maze_->players_.end(); it_players++) {
          if (*it_players != NULL) {
            (*it_players)->update();
          }
          else {
            break;
          }
        }

        // TODO: guardians updating
        
        p_maze_->next_update_.keys_coords.clear();
        p_maze_->next_update_.gates_opened_coords.clear();
        p_maze_->next_update_.gates_closed_coords.clear();
        p_maze_->next_update_.players_coords.clear();
        p_maze_->next_update_.guardians_coords.clear();


        std::vector<std::pair<signed char, signed char>>::iterator iter;


        for (iter = p_maze_->keys_.begin(); iter != p_maze_->keys_.end(); iter++) {
          p_maze_->next_update_.keys_coords.push_back(*iter);
        }


        for (iter = p_maze_->gates_.begin(); iter != p_maze_->gates_.end(); iter++) {
          if (p_maze_->matrix_[(*iter).first][(*iter).second].get() == game::block::GATE_CLOSED) {
            p_maze_->next_update_.gates_closed_coords.push_back(*iter);
          }
          else {
            p_maze_->next_update_.gates_opened_coords.push_back(*iter);
          }
        }


        std::vector<game::guardian>::iterator it_guardians;

        for (it_guardians = p_maze_->guardians_.begin(); it_guardians != p_maze_->guardians_.end(); it_guardians++) {
          p_maze_->next_update_.guardians_coords.push_back((*it_guardians).get_coords());
        }


        for (it_players = p_maze_->players_.begin(); it_players != p_maze_->players_.end(); it_players++) {
          if (*it_players != NULL) {
            p_maze_->next_update_.players_coords.push_back((*it_players)->get_coords());
          }
          else {
            break;
          }
        }


        for (it_players = p_maze_->players_.begin(); it_players != p_maze_->players_.end(); it_players++) {
          if (*it_players != NULL) {
            (*it_players)->update_client(p_maze_->next_update_);
          }
          else {
            break;
          }
        }

      }
      p_maze_->players_.unlock_upgrade();

    }
    p_maze_->access_mutex_.unlock();
    return;
  }}}
}


/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_GAME_INSTANCE.CC ]****************************************************************************** *
 * ****************************************************************************************************************** */

