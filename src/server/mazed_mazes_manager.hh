/**
 * @file      mazed_mazes_manager.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.2
 * @brief     Contains definition of mazed::mazes_manager which can list all available mazes or saves and load them or
 *            save them.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_MAZES_MANAGER.HH ]**************************************************************************** *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_MAZED_MAZES_MANAGER_HH
#define H_GUARD_MAZED_MAZES_MANAGER_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <string>
#include <vector>

#include <boost/filesystem.hpp>

#include "mazed_globals.hh"
#include "mazed_game_maze.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ MAZES_MANAGER CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace filesys = boost::filesystem;

namespace mazed {
  
  /**
   * Class encapsulating all operations with listing available mazes, loading them and saving them back.
   */
  class mazes_manager {
      std::string mazes_extension_;
      std::string saves_extension_;
      
      filesys::path mazes_dir_path_;
      filesys::path saves_dir_path_;
      filesys::path daemon_dir_path_;
      
      // // // // // // // // // // //

      std::vector<std::string> list_directory(filesys::path dir_path, std::string extension);

    public:
      mazes_manager(mazed::settings_tuple settings);

      std::vector<std::string> list_mazes();
      std::vector<std::string> list_saves();

      game::maze *load_maze(const std::string &maze_name);
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_MAZES_MANAGER.HH ]****************************************************************************** *
 * ****************************************************************************************************************** */

#endif

