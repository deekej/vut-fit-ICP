/**
 * @file      mazed_game_maze.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.1
 * @brief     Contains a declaration of a game::maze class used on server-side.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_GAME_MAZE.HH ]******************************************************************************** *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_MAZED_GAME_MAZE_HH
#define H_GUARD_MAZED_GAME_MAZE_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#include "mazed_game_globals.hh"
#include "mazed_game_block.hh"
#include "mazed_game_guardian.hh"
#include "mazed_game_player.hh"
#include "mazed_game_players_list.hh"
#include "../basic_maze.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ MAZE CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace mazed {
  class client_handler;
  class mazes_manager;
}

namespace game {

  /**
   * Maze class to be used by the game instance's logic.
   */
  class maze : public basic_maze {
      friend class mazed::client_handler;
      friend class mazed::mazes_manager;
      friend class game::player;
      friend class game::guardian;
      // game_instance

      // // // // // // // // // // //

      using uchar_t = unsigned char;

      boost::mutex                                              access_mutex_;
    
      long                                                      game_speed_;
      std::string                                               game_owner_;
      std::string                                               maze_scheme_;
      std::string                                               maze_version_;

      game::players_list                                        players_;
      std::unordered_map<std::string, uchar_t>                  previous_players_;

      std::array<std::pair<uchar_t, uchar_t>, GAME_MAX_PLAYERS> players_start_coords_;
      std::array<std::pair<uchar_t, uchar_t>, GAME_MAX_PLAYERS> players_saved_coords_;
      
      std::vector<game::guardian>                               guardians_;
      std::vector<game::block *>                                gates_;
      std::vector<std::vector<game::block>>                     matrix_;

      // // // // // // // // // // //
      
    public:
      maze(uchar_t row_num, uchar_t col_num, const std::string &scheme) :
        basic_maze(row_num, col_num),
        maze_scheme_{scheme}, matrix_(row_num, std::vector<game::block>(col_num))
      {{{
        return;
      }}}

      ~maze()
      {{{
        return;
      }}}
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_GAME_MAZE.HH ]********************************************************************************** *
 * ****************************************************************************************************************** */

#endif

