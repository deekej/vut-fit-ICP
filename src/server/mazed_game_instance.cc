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
  instance::instance(game::maze *maze_ptr, std::string game_owner,
                     std::shared_ptr<mazed::shared_resources> ps_shared_res, mazed::client_handler *cl_handler_ptr) : 
    timer_(io_service_), p_maze_{maze_ptr}, p_cl_handler_{cl_handler_ptr}, ps_shared_res_{ps_shared_res}
  {{{
    p_maze_->game_owner_ = game_owner;
    return;
  }}}


  instance::~instance()
  {{{
    if (shared_ == true) {
      shared_ = false;

      ps_shared_res_->access_mutex.lock();
      {
        ps_shared_res_->game_instances.erase(it_self_);
      }
      ps_shared_res_->access_mutex.unlock();
    }


    std::array<player *, GAME_MAX_PLAYERS>::iterator it_players;
    game::player *p_player;

    for (it_players = p_maze_->players_.begin(); it_players != p_maze_->players_.end(); it_players++) {
      if (*it_players != NULL) {
        p_player = *it_players;
        remove_player(*it_players);
        p_player->game_finished();
      }
      else {
        continue;
      }
    }


    timer_.cancel();
    io_service_.stop();

    if (pu_thread_ && (*pu_thread_).joinable() == true) {
      (*pu_thread_).join();
    }

    delete p_maze_;
    p_maze_ = NULL;
    
    return;
  }}}

  // // // // // // // // // // //

  std::shared_ptr<game::instance> instance::run()
  {{{
    assert(pu_thread_.get() == nullptr);
    
    pu_thread_ = std::unique_ptr<boost::thread>(new boost::thread(&instance::start_game, this));

    ps_shared_res_->access_mutex.lock();
    {
      ps_shared_res_->game_instances.emplace_back(this);
      it_self_ = ps_shared_res_->game_instances.end();
      it_self_--;
      shared_ = true;
    }
    ps_shared_res_->access_mutex.unlock();

    
    return *it_self_;
  }}}


  bool instance::stop(const std::string user)
  {{{
    bool retval {false};
    std::shared_ptr<game::instance> ps_tmp_this;

    p_maze_->access_mutex_.lock();
    {
      if (p_maze_->game_owner_ == user) {
                
        if (shared_ == true) {
          shared_ = false;

          ps_shared_res_->access_mutex.lock();
          {
            ps_tmp_this = *it_self_;
            ps_shared_res_->game_instances.erase(it_self_);
          }
          ps_shared_res_->access_mutex.unlock();
        }


        std::array<player *, GAME_MAX_PLAYERS>::iterator it_players;
        game::player *p_player;

        for (it_players = p_maze_->players_.begin(); it_players != p_maze_->players_.end(); it_players++) {
          if (*it_players != NULL) {
            // TODO: GAME TERMINATING INFORM

            p_player = *it_players;
            remove_player(*it_players);
            p_player->game_finished();
          }
          else {
            continue;
          }
        }


        timer_.cancel();
        io_service_.stop();

        if (pu_thread_ && (*pu_thread_).joinable() == true) {
          (*pu_thread_).join();
        }

        io_service_.reset();
        retval = true;
      }
    }
    p_maze_->access_mutex_.unlock();

    return retval;
  }}}


#if 0
  protocol::E_game_status instance::get_status()
  {{{
    protocol::E_game_status retval;

    p_maze_->access_mutex_.lock();
    {
      if (p_maze_->game_finished_ == true) {
        retval = FINISHED;
      }
      else {
        retval = (p_maze_->game_run_ == true) ? RUNNING : PAUSED;
      }
    }
    p_maze_->access_mutex_.unlock();

    return retval;
  }}}
#endif

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
        // This error occurs every time we asynchronously cancel the events of the timer.
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
            if ((*it_players)->update() == true) {
              p_maze_->game_winners_.emplace_back(*it_players);
            }
          }
          else {
            continue;
          }
        }

        if (p_maze_->game_winners_.empty() == true) {
          // TODO: guardians updating
          // In case of death of a player -> check if he's dead.
          // In case of all player's death -> Game Over.
        }
        else {
          p_maze_->game_run_ = false;
          p_maze_->game_finished_ = true;

          for (it_players = p_maze_->players_.begin(); it_players != p_maze_->players_.end(); it_players++) {
            if (*it_players != NULL) {
              // TODO: INFORM players with INFO message.
              (*it_players)->game_finished();
            }
            else {
              continue;
            }
          }
        }

        
        p_maze_->next_updates_[0].keys_coords.clear();
        p_maze_->next_updates_[0].opened_gates_coords.clear();
        p_maze_->next_updates_[0].players_coords.clear();
        p_maze_->next_updates_[0].guardians_coords.clear();


        std::vector<std::pair<signed char, signed char>>::iterator iter;

        for (iter = p_maze_->keys_.begin(); iter != p_maze_->keys_.end(); iter++) {
          p_maze_->next_updates_[0].keys_coords.push_back(*iter);
        }

        for (iter = p_maze_->gates_.begin(); iter != p_maze_->gates_.end(); iter++) {
          if (p_maze_->matrix_[(*iter).first][(*iter).second].get() == game::block::GATE_CLOSED) {
            continue;
          }
          else {
            p_maze_->next_updates_[0].opened_gates_coords.push_back(*iter);
          }
        }


        std::vector<game::guardian>::iterator it_guardians;

        for (it_guardians = p_maze_->guardians_.begin(); it_guardians != p_maze_->guardians_.end(); it_guardians++) {
          p_maze_->next_updates_[0].guardians_coords.push_back((*it_guardians).get_coords());
        }


        for (it_players = p_maze_->players_.begin(); it_players != p_maze_->players_.end(); it_players++) {
          if (*it_players != NULL) {
            p_maze_->next_updates_[0].players_coords.push_back((*it_players)->get_coords());
          }
          else {
            p_maze_->next_updates_[0].players_coords.push_back(std::pair<signed char, signed char>(-1, -1));
          }
        }


        for (it_players = p_maze_->players_.begin(); it_players != p_maze_->players_.end(); it_players++) {
          if (*it_players != NULL) {
            (*it_players)->update_client(p_maze_->next_updates_);
          }
          else {
            continue;
          }
        }

      }
      p_maze_->players_.unlock_upgrade();

    }
    p_maze_->access_mutex_.unlock();

    return;
  }}}

  // // // // // // // // // // //

  bool instance::check_player(game::player *player_ptr)
  {{{
    return false;
  }}}

  
  bool instance::add_player(game::player *player_ptr)
  {{{
    bool retval {true};
    unsigned char player_num;
    
    p_maze_->players_.lock_upgrade();
    {
      player_num = p_maze_->players_.add(player_ptr);

      if (player_num < GAME_MAX_PLAYERS) {
        p_maze_->players_alive_++;
        player_ptr->set_maze(p_maze_);
        player_ptr->set_number(player_num);
        player_ptr->set_start_coords(p_maze_->players_start_coords_[player_num]);
      }
      else {
        retval = false;
      }

    }
    p_maze_->players_.unlock_upgrade();

    return retval;
  }}}


  void instance::remove_player(game::player *player_ptr)
  {{{
    // TODO: Add player to already played list.

    p_maze_->players_.lock_upgrade();
    {
#ifndef NDEBUG
      p_maze_->players_.remove(player_ptr->get_number(), player_ptr);
#else
      p_maze_->players_.remove(player_ptr->get_number());
#endif

      p_maze_->players_alive_--;
    }
    p_maze_->players_.unlock_upgrade();

    return;
  }}}
}


/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_GAME_INSTANCE.CC ]****************************************************************************** *
 * ****************************************************************************************************************** */

