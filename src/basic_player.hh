/**
 * @file      basic_player.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   1.0
 * @brief     Player's base class for deriving.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF BASIC_PLAYER.HH ]*********************************************************************************** *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_BASIC_PLAYER_HH
#define H_GUARD_BASIC_PLAYER_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <string>
#include <utility>


/* ****************************************************************************************************************** *
 ~ ~~~[ BASIC_PLAYER CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace game {
  class basic_player {
      std::string                                 nick_;
      unsigned char                               player_num_;
      unsigned char                               lifes_;
      std::pair<unsigned char, unsigned char>     coords_;
      bool                                        has_key_ {false};

      static unsigned long                        players_counter_;
  
    public:
      basic_player() : player_num_{0}, lifes_{3}
      {{{
        players_counter_++;
        nick_ = "player_" + std::to_string(players_counter_);

        return;
      }}}

      basic_player(const std::string &nick) : nick_{nick}, player_num_{0}, lifes_{3}
      {{{
        players_counter_++;
        return;
      }}}

      virtual ~basic_player()
      {{{
        return;
      }}}

      // // // // // // // // // // //

      virtual std::string &get_nick()
      {{{
        return nick_;
      }}}

      virtual void set_nick(const std::string &nick)
      {{{
        nick_ = nick;
        return;
      }}}

      virtual unsigned char get_lifes()
      {{{
        return lifes_;
      }}}

      virtual void decr_lifes()
      {{{
        if (lifes_ > 0) {
          lifes_--;
        }

        return;
      }}}
  };

  unsigned long basic_player::players_counter_ = 0;
}

/* ****************************************************************************************************************** *
 * ***[ END OF BASIC_PLAYER.HH ]************************************************************************************* *
 * ****************************************************************************************************************** */

#endif

