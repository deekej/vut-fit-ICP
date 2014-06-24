/**
 * @file      mazed_game_guardian.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   1.0
 * @brief     Contains a specialization of the guardian class to be used on server-side.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_GAME_GUARDIAN.HH ]**************************************************************************** *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_MAZED_GAME_GUARDIAN_HH
#define H_GUARD_MAZED_GAME_GUARDIAN_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include "mazed_game_maze.hh"
#include "../basic_guardian.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ BASIC_MAZE CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace game {

  class maze;

  /**
   * Specialization of the guardian, which encapsulates the AI and can therefore act autonomously.
   */
  class guardian : public basic_guardian {
      std::pair<signed char, signed char>         start_coords_;
      game::maze                                  *p_maze_;
      game::E_move                                direction_;
      bool                                        prev_move_success_;
    public:
      guardian()
      {{{
        return;
      }}}


      guardian(signed char row, signed char column, game::maze *ptr) :
        basic_guardian(row, column),
        start_coords_(row, column), p_maze_{ptr}
      {{{
        return;
      }}}


      ~guardian()
      {{{
        return;
      }}}


      void set_coords(signed char row, signed char column)
      {{{
        coords_.first = row;
        coords_.second = column;
      }}}


      std::pair<signed char, signed char> get_coords()
      {{{
        return coords_;
      }}}
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_GAME_GUARD.HH ]********************************************************************************* *
 * ****************************************************************************************************************** */

#endif

