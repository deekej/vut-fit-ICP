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
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "mazed_game_globals.hh"
#include "mazed_game_block.hh"
#include "mazed_game_guardian.hh"
#include "mazed_game_players_list.hh"
#include "../protocol.hh"
#include "../basic_maze.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ MAZE CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace mazed {
  class client_handler;
  class mazes_manager;
}

namespace game {

  class player;
  class instance;

  /**
   * Maze class to be used by the game instance's logic.
   */
  class maze : public basic_maze {
      friend class mazed::client_handler;
      friend class mazed::mazes_manager;
      friend class game::player;
      friend class game::guardian;
      friend class game::instance;

      // // // // // // // // // // //

      using schar_t = signed char;

      boost::mutex                                              access_mutex_;

      std::string                                               game_owner_;
      long                                                      game_speed_ {1000};
      bool                                                      game_run_ {false};
      bool                                                      game_finished_ {false};

      std::string                                               maze_scheme_;
      std::string                                               maze_version_;

      game::players_list                                        players_;
      std::unordered_map<std::string, schar_t>                  previous_players_;

      std::array<std::pair<schar_t, schar_t>, GAME_MAX_PLAYERS> players_start_coords_;
      std::array<std::pair<schar_t, schar_t>, GAME_MAX_PLAYERS> players_saved_coords_;
      
      std::vector<game::guardian>                               guardians_;
      std::vector<std::pair<schar_t, schar_t>>                  gates_;
      std::vector<std::pair<schar_t, schar_t>>                  keys_;
      std::vector<std::vector<game::block>>                     matrix_;

      std::queue<std::pair<protocol::E_info_type, std::string>> events_queue_;
      protocol::update                                          next_update_;

      // // // // // // // // // // //
      
    public:
      maze(schar_t row_num, schar_t col_num, const std::string &scheme) :
        basic_maze(row_num, col_num),
        maze_scheme_{scheme}, gates_(), keys_(), matrix_(row_num, std::vector<game::block>(col_num))
      {{{
        return;
      }}}

      ~maze()
      {{{
        return;
      }}}

      bool is_move_possible(std::pair<signed char, signed char> coords, game::E_move move)
      {{{
        game::block::E_block_type block;

        switch (move) {
          case game::E_move::LEFT :
            coords.second--;
            break;

          case game::E_move::RIGHT :
            coords.second++;
            break;

          case game::E_move::UP :
            coords.first--;
            break;

          case game::E_move::DOWN :
            coords.first++;
            break;

          case game::E_move::STOP :
            return true;

          default :
            return false;
        }

        block = matrix_[coords.first % dimensions_.first][coords.second % dimensions_.second].get();

        return (block == game::block::WALL || block == game::block::GATE_CLOSED) ? false : true;
      }}}
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_GAME_MAZE.HH ]********************************************************************************** *
 * ****************************************************************************************************************** */

#endif

