/**
 * @file      mazed_globals.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.2
 * @brief     Shared global declarations/definitions of the MAZE server daemon.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_GLOBALS.HH ]********************************************************************************** *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_MAZED_GLOBALS_HH
#define H_GUARD_MAZED_GLOBALS_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <tuple>
#include <string>

#include <boost/filesystem/path.hpp>


/* ****************************************************************************************************************** *
 ~ ~~~[ DECLARATIONS/DEFINITIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace mazed {
  enum E_settings {
    DAEMON_FOLDER = 0,
    PLAYERS_FOLDER,
    SAVES_FOLDER,
    SAVES_EXTENSION,
    MAZES_FOLDER,
    MAZES_EXTENSION,
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
    boost::filesystem::path,            // DAEMON_FOLDER
    std::string,                        // PLAYERS_FOLDER
    std::string,                        // SAVES_FOLDER
    std::string,                        // SAVES_EXTENSION
    std::string,                        // MAZES_FOLDER
    std::string,                        // MAZES_EXTENSION
    std::string,                        // LOG_FOLDER
    std::string,                        // SERVER_LOG_FILE
    long,                               // SLEEP_INTERVAL
    long,                               // MAX_PING
    unsigned short,                     // SERVER_PORT
    log_level                           // LOGGING
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


/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_GLOBALS.HH ]************************************************************************************ *
 * ****************************************************************************************************************** */

#endif

