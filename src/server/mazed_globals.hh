/**
 * @file      main.cc
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.1
 * @brief     Server daemon of Maze-game for ICP course @ BUT FIT, 2014.
 *
 * @detailed  TODO
 *
 * @note      TODO
 */

#ifndef H_GUARD_MAZED_GLOBAL_HH
#define H_GUARD_MAZED_GLOBAL_HH

#include <tuple>
#include <string>

namespace mazed {
  enum E_settings {
    PLAYERS_FOLDER = 0,
    SAVES_FOLDER,
    MAZES_FOLDER,
    LOG_FOLDER,
    SERVER_LOG_FILE,
    SLEEP_INTERVAL,
    MAX_PING,
    SERVER_PORT,
    LOGGING_LEVEL,
  };

  enum class log_level : unsigned char {
    NONE = 0,
    ALL,
    INFO,
    ERROR,
  };

  using settings_tuple = std::tuple<
    std::string,        // PLAYERS_FOLDER
    std::string,        // SAVES_FOLDER
    std::string,        // MAZES_FOLDER
    std::string,        // LOG_FOLDER
    std::string,        // SERVER_LOG_FILE
    long,               // SLEEP_INTERVAL
    long,               // MAX_PING
    unsigned short,     // SERVER_PORT
    log_level           // LOGGING
  >;
 
  namespace exit_codes {
    enum {
      NO_ERROR = 0,
      E_WRONG_PARAMS,
      E_FORKING,
      E_FOLDER_CREATE,
      E_OPEN,
      E_WRITE,
      E_UNKNOWN_EXCEPT,
    };
  }
}

#endif
