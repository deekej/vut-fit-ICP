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
  class player;

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


      E_block_type get()
      {{{
        return type_;
      }}}


      inline void set(E_block_type type)
      {{{
        type_ = type;
        return;
      }}}


      void add_player(game::player *p_player)
      {{{
        if (p_players_ == NULL) {
          p_players_ = new std::vector<game::player *>();
        }

        p_players_->push_back(p_player);
        has_player_ = true;

        return;
      }}}


      void remove_player(game::player *p_player)
      {{{
        assert(has_player_ == true);
        assert(p_players_ != NULL);

        std::vector<game::player *>::iterator it;

        for (it = p_players_->begin(); it != p_players_->end() && *it != p_player; it++) {
          ;
        }

        assert(it != p_players_->end());
        assert(*it == p_player);

        p_players_->erase(it);

        if (p_players_->empty() == true) {
          delete p_players_;
          p_players_ = NULL;
          has_player_ = false;
        }

        return;
      }}}
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_GAME_BLOCK.HH ]********************************************************************************* *
 * ****************************************************************************************************************** */

#endif

