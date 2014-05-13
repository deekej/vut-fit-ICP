/**
 * @file      client_globals.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.2
 * @brief     Shared global declarations/definitions for the MAZE client.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF CLIENT_GLOBALS.HH ]********************************************************************************* *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_CLIENT_GLOBALS_HH
#define H_GUARD_CLIENT_GLOBALS_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <string>
#include <tuple>


/* ****************************************************************************************************************** *
 ~ ~~~[ DECLARATIONS/DEFINITIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace client {
  enum E_settings {
    PROCESS_NAME = 0x0,
    IPv4_ADDRESS,
    SERVER_PORT,
    CONFIG_FILE_LOC,
    HELLO_INTERVAL,
    MAX_PING,
    NICK,
    PUID,
  };

  enum E_event {
    MESSAGE = 0x0,
    USER_INPUT,
  };

  using settings_tuple = std::tuple<
    std::string,        // PROCESS_NAME
    std::string,        // IPv4_ADDRESS
    std::string,        // SERVER_PORT
    std::string,        // CONFIG_FILE_LOC
    long,               // HELLO_INTERVAL
    long,               // MAX_PING
    std::string,        // NICK
    std::string         // PUID
  >;
 
  enum exit_codes {
    NO_ERROR = 0x0,
    E_WRONG_PARAMS,
    E_OPEN,
    E_WRITE,
    E_CONNECT_FAILED,
    E_UNKNOWN_EXCEPT,
  };
}


/* ****************************************************************************************************************** *
 * ***[ END OF CLIENT_GLOBALS.HH ]*********************************************************************************** *
 * ****************************************************************************************************************** */

#endif

