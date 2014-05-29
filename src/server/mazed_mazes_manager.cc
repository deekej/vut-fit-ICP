/**
 * @file      mazed_mazes_manager.cc
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.2
 * @brief     Contains implementations of class member functions of mazed::mazes_manager.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_MAZES_MANAGER.CC ]**************************************************************************** *
 * ****************************************************************************************************************** */


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <fstream>
#include <list>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include <boost/algorithm/string.hpp>

#include "mazed_mazes_manager.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ MEMBER FUNCTIONS IMPLEMENTATIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace filesys = boost::filesystem;

namespace mazed {
  
  mazes_manager::mazes_manager(mazed::settings_tuple settings) :
    mazes_extension_{std::get<MAZES_EXTENSION>(settings)}, saves_extension_{std::get<SAVES_EXTENSION>(settings)},
    mazes_dir_path_(std::get<MAZES_FOLDER>(settings)), saves_dir_path_(std::get<SAVES_FOLDER>(settings)),
    daemon_dir_path_{std::get<DAEMON_FOLDER>(settings)}
  {{{
    return;
  }}}
  
  // // // // // // // // // // // //

  std::vector<std::string> mazes_manager::list_mazes()
  {{{
    try {
      filesys::current_path(daemon_dir_path_);
      return list_directory(mazes_dir_path_, mazes_extension_);
    }
    catch (filesys::filesystem_error) {
      return {};
    }
  }}}

  std::vector<std::string> mazes_manager::list_saves()
  {{{
    try {
      filesys::current_path(daemon_dir_path_);
      return list_directory(saves_dir_path_, saves_extension_);
    }
    catch (filesys::filesystem_error) {
      return {};
    }
  }}}

  game::maze *mazes_manager::load_maze(const std::string &maze_name)
  {{{
    try {
      filesys::current_path(daemon_dir_path_);
      filesys::current_path(mazes_dir_path_);
      
      std::ifstream maze_file(maze_name);

      if (maze_file.fail() == true) {
        return NULL;
      }


      std::string input, version;
      std::vector<std::string> input_tokens;
      std::size_t rows, cols;
      std::stringstream maze_scheme;

      game::maze *p_maze = NULL;


      std::getline(maze_file, input);

      if (maze_file.good() != true) {
        return NULL;
      }

      boost::split(input_tokens, input, boost::is_any_of("="));
      
      if (input_tokens.size() != 2 || input_tokens[0] != "version") {
        return NULL;
      }

      version = input_tokens[1];
      std::getline(maze_file, input);
      
      if (maze_file.good() != true) {
        return NULL;
      }

      boost::split(input_tokens, input, boost::is_any_of("=xX"));

      if (input_tokens.size() != 3 || input_tokens[0] != "size") {
        return NULL;
      }
      
      rows = std::stoul(input_tokens[1], nullptr, 0);
      cols = std::stoul(input_tokens[2], nullptr, 0);

      if (rows < MAZE_MIN_SIZE || rows > MAZE_MAX_SIZE || cols < MAZE_MIN_SIZE || cols > MAZE_MAX_SIZE) {
        return NULL;
      }

      std::getline(maze_file, input);             // Skipping the line delimiter.

      if (maze_file.good() != true) {
        return NULL;
      }
      
      
      for (std::size_t i = 0; i < rows; i++) {
        std::getline(maze_file, input);

        if (maze_file.good() != true || input.size() != (cols * 2 - 1)) {
          return NULL;
        }

        maze_scheme << input << "\n";
      }


      input = maze_scheme.str();
      p_maze = new game::maze(static_cast<signed char>(rows), static_cast<signed char>(cols), input);
      
      std::size_t linear_pos {0};

      for (std::size_t i = 0; i < rows; i++) {
        for (std::size_t j = 0; j < cols; j++) {

          linear_pos = (i * cols * 2) + (j * 2);

          switch (input[linear_pos]) {
            case ' ' :
              p_maze->matrix_[i][j].set(game::block::EMPTY);
              break;

            case 'X' :
              p_maze->matrix_[i][j].set(game::block::WALL);
              break;

            case '~' :
              p_maze->matrix_[i][j].set(game::block::GATE_CLOSED);
              p_maze->gates_.emplace_back(std::pair<signed char, signed char>(i, j));
              break;

            case '*' :
              p_maze->matrix_[i][j].set(game::block::KEY);
              p_maze->keys_.emplace_back(std::pair<signed char, signed char>(i, j));
              break;

            case 'G' :
              p_maze->matrix_[i][j].set(game::block::TARGET);
              break;
            
            case '1' :
              p_maze->players_start_coords_[0].first = i;
              p_maze->players_start_coords_[0].second = j;
              input[linear_pos] = ' ';
              break;
              
            case '2' :
              p_maze->players_start_coords_[1].first = i;
              p_maze->players_start_coords_[1].second = j;
              input[linear_pos] = ' ';
              break;
              
            case '3' :
              p_maze->players_start_coords_[2].first = i;
              p_maze->players_start_coords_[2].second = j;
              input[linear_pos] = ' ';
              break;
              
            case '4' :
              p_maze->players_start_coords_[3].first = i;
              p_maze->players_start_coords_[3].second = j;
              input[linear_pos] = ' ';
              break;
              
            case '@' :
              p_maze->guardians_.emplace_back(i, j, p_maze);
              input[linear_pos] = ' ';
              break;

            default :
              delete p_maze;
              return NULL;
          }
        }
      }

      p_maze->maze_version_ = version;
      p_maze->maze_scheme_ = input;
      p_maze->maze_scheme_.back() = ' ';

      return p_maze;
    }
    catch (filesys::filesystem_error) {
      return NULL;
    }
    catch (std::exception) {
      return NULL;
    }
  }}}

  // // // // // // // // // // // //

  std::vector<std::string> mazes_manager::list_directory(filesys::path dir_path, std::string extension)
  {{{
    std::list<std::string> files;
    
    try {
      filesys::current_path(dir_path);
      
      filesys::directory_iterator it_dir_end;
      boost::system::error_code error;
      std::string fname;
      std::size_t fname_length;

      std::size_t ext_length {extension.length()};

      for (filesys::directory_iterator it_dir("."); it_dir != it_dir_end; it_dir++) {

        if (filesys::is_regular_file(it_dir->path(), error) == true && !error) {
          fname = it_dir->path().filename().native();
          fname_length = fname.length();

          if (fname_length < ext_length) {
            continue;
          }
          
          if (ext_length == 0 || fname.find(extension, fname_length - ext_length) != std::string::npos) {
            files.push_back(fname);
          }
        }

      }
      
      files.sort();

      return {std::make_move_iterator(std::begin(files)), std::make_move_iterator(std::end(files))};
    }
    catch (filesys::filesystem_error) {
      return {};
    }
  }}}
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_MAZES_MANAGER.CC ]****************************************************************************** *
 * ****************************************************************************************************************** */

