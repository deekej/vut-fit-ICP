/**
 * @file      mazed_game_players_list.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   1.0
 * @brief     Contains the class encapsulation all the players in the game.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_GAME_PLAYERS_LIST.HH ]************************************************************************ *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_MAZED_GAME_PLAYERS_LIST_HH
#define H_GUARD_MAZED_GAME_PLAYERS_LIST_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <array>

#include <boost/thread.hpp>

#include <cassert>

#include "mazed_game_globals.hh"
#include "mazed_game_player.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ PLAYER CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace game {

  /**
   * Class which acts as a std::vector, containing pointers to the game::player classes, but it also acts as a mutex,
   * allowing locking the access to the contents of the class.
   */
  class players_list {
      std::array<player *, GAME_MAX_PLAYERS>      players_;
      unsigned char                               first_empty_ {0};
      unsigned char                               used_slots_ {0};
      boost::upgrade_mutex                        access_mutex_;

    public:
      players_list()
      {{{
        players_.fill(NULL);
        return;
      }}}


      ~players_list()
      {{{
        return;
      }}}


      void lock()
      {{{
        access_mutex_.lock();
        return;
      }}}


      void unlock()
      {{{
        access_mutex_.unlock();
        return;
      }}}


      void lock_upgrade()
      {{{
        access_mutex_.lock_upgrade();
        return;
      }}}


      void unlock_upgrade()
      {{{
        access_mutex_.unlock_upgrade();
        return;
      }}}


      std::array<player *, GAME_MAX_PLAYERS>::iterator begin()
      {{{
        return players_.begin();
      }}}


      std::array<player *, GAME_MAX_PLAYERS>::iterator end()
      {{{
        return players_.end();
      }}}


      unsigned char get_used_slots()
      {{{
        return used_slots_;
      }}}


      unsigned char add(game::player *p_player)
      {{{
        unsigned char retval = first_empty_;
        
        if (first_empty_ < GAME_MAX_PLAYERS) {
          players_[first_empty_++] = p_player;
          used_slots_++;

          std::array<player *, GAME_MAX_PLAYERS>::iterator it;

          for (it = players_.begin() + first_empty_; it != players_.end() && *it != NULL; it++) {
            first_empty_++;
          }
        }

        return retval;
      }}}


#ifndef NDEBUG
      void remove(unsigned char player_num, game::player *p_player)
      {{{
        assert(players_[player_num] == p_player);
        used_slots_--;

        players_[player_num] = NULL;
        
        if (player_num < first_empty_) {
          first_empty_ = player_num;
        }

        return;
      }}}
#else
      void remove(unsigned char player_num)
      {{{
        players_[player_num] = NULL;
        used_slots_--;
        
        if (player_num < first_empty_) {
          first_empty_ = player_num;
        }

        return;
      }}}
#endif
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_GAME_PLAYERS_LIST.HH ]************************************************************************** *
 * ****************************************************************************************************************** */

#endif

