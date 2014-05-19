/**
 * @file      mazed_game_block.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.1
 * @brief     Contains class of game block for server-side purposes.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_GAME_BLOCK.HH ]******************************************************************************* *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_MAZED_GAME_BLOCK_HH
#define H_GUARD_MAZED_GAME_BLOCK_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <vector>

#include "mazed_game_player.hh"
#include "../basic_block.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ BLOCK CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace game {
  /**
   * Class of game block for server-side purposes.
   */
  class block : public basic_block {
      // Normally, list might be better, but the maximum size of the vector will be 4 - this is acceptable:
      std::vector<game::player *>       *p_players_ {NULL};

    public:
      block() : basic_block()
      {{{
        return;
      }}}

      ~block()
      {{{
        delete p_players_;
        return;
      }}}
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_GAME_BLOCK.HH ]********************************************************************************* *
 * ****************************************************************************************************************** */

#endif

