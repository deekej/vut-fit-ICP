/**
 * @file      basic_maze.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   1.0
 * @brief     Game's maze basic class to be derived from.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF BASIC_MAZE.HH ]************************************************************************************* *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_BASIC_MAZE_HH
#define H_GUARD_BASIC_MAZE_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <utility>


/* ****************************************************************************************************************** *
 ~ ~~~[ BASIC_MAZE CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace game {
  class basic_maze {
    protected:
      std::string                                 name_;
      std::pair<signed char, signed char>         dimensions_;

    public:
      basic_maze() : dimensions_(0, 0)
      {{{
        return;
      }}}

      basic_maze(signed char row_size, signed char col_size) : dimensions_(row_size, col_size)
      {{{
        return;
      }}}

      virtual ~basic_maze()
      {{{
        return;
      }}}
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF BASIC_MAZE.HH ]*************************************************************************************** *
 * ****************************************************************************************************************** */

#endif

