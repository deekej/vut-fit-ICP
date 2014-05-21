/**
 * @file      basic_block.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   1.0
 * @brief     Maze's block base class for deriving.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF BASIC_BLOCK.HH ]************************************************************************************ *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_BASIC_BLOCK_HH
#define H_GUARD_BASIC_BLOCK_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */


/* ****************************************************************************************************************** *
 ~ ~~~[ BASIC_BLOCK CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace game {
  class basic_block {
    public:
      enum E_block_type {
        EMPTY = 0,
        PLAYER,
        PLAYER_ON_GATE,
        WALL,
        TARGET,
        GATE_CLOSED,
        GATE_OPEN,
        KEY,
      };

    protected:
      enum E_block_type type_;

    public:
      basic_block() : type_{EMPTY}
      {{{
        return;
      }}}

      basic_block(E_block_type type) : type_{type}
      {{{
        return;
      }}}

      virtual ~basic_block()
      {{{
        return;
      }}}

      void set(E_block_type type)
      {{{
        type_ = type;
        return;
      }}}
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF BASIC_BLOCK.HH ]************************************************************************************** *
 * ****************************************************************************************************************** */

#endif
