/**
 * @file      mazed_game_player.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.1
 * @brief     Contains derived class (specialization) of the basic_player.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_GAME_PLAYER.HH ]****************************************************************************** *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_MAZED_GAME_PLAYER_HH
#define H_GUARD_MAZED_GAME_PLAYER_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "../basic_player.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ PLAYER CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace game {

  /**
   * Derived from basic_player class for server-side purposes.
   */
  class player : public basic_player {
      static unsigned long                        players_counter_;
      
      // udp socket
      // io service
      // cmd buffer
      // next-move
      // last-move-result
      // player-upgrade-mutex
           
      std::string                                 puid_;
      unsigned char                               invulnerability_ {3};

      // Coordinates for player so he known where to respawn after he has been killed:
      std::pair<signed char, signed char>         start_coords_;

    public:
      player() : basic_player()
      {{{
        nick_ = "player-" + std::to_string(players_counter_++);
        return;
      }}}


      player(const std::string &nick) : basic_player(nick)
      {{{
        return;
      }}}


      player(const std::string &nick, unsigned char player_num, unsigned char lifes = 3) :
        basic_player(nick, player_num, lifes)
      {{{
        return;
      }}}


      ~player()
      {{{
        return;
      }}}

      // // // // // // // // // // //
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_GAME_PLAYER.HH ]******************************************************************************** *
 * ****************************************************************************************************************** */

#endif

