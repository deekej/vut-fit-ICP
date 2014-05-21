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
    protected:
      std::string                                 nick_;
      unsigned char                               player_num_;
      unsigned char                               lifes_;
      std::pair<signed char, signed char>         coords_;
      bool                                        has_key_ {false};
  
    public:
      basic_player() : nick_{"player-N/A"}, player_num_{0}, lifes_{3}
      {{{
        return;
      }}}


      basic_player(const std::string &nick) : nick_{nick}, player_num_{0}, lifes_{3}
      {{{
        return;
      }}}


      basic_player(const std::string &nick, unsigned char player_num) : nick_{nick}, player_num_{player_num}, lifes_{3}
      {{{
        return;
      }}}


      basic_player(const std::string &nick, unsigned char player_num, unsigned char lifes) :
        nick_{nick}, player_num_{player_num}, lifes_{lifes}
      {{{
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
      

      virtual void set_lifes(unsigned char lifes)
      {{{
        lifes_ = lifes;

        return;
      }}}


      virtual void decr_lifes()
      {{{
        if (lifes_ > 0) {
          lifes_--;
        }

        return;
      }}}
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF BASIC_PLAYER.HH ]************************************************************************************* *
 * ****************************************************************************************************************** */

#endif

