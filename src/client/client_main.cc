/**
 * @file      client_main.cc
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   1.0
 * @brief     Client for Maze-game for ICP course @ BUT FIT, 2014.
 *
 * @detailed  This is a main program for running the client for the game, it connects to specified server and allows
 *            user to play the Maze-game.
 */


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

// C++ header files:
#include <iostream>
#include <iterator>
#include <string>
#include <tuple>

// Boost header files:
#include <boost/program_options.hpp>

// Program header files:
#include "client_globals.hh"
#include "client_connections.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ GLOBAL VARIABLES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

const std::string HELP_STRING =
"This is the client for MAZE-GAME application,\n"
"which is the part from project of ICP course @ BUT FIT, Czech Republic, 2014.\n\n"
"Version:       0.1\n"
"Written by:    Dee'Kej <deekej@linuxmail.org>\n"
"Website:       https://bitbucket.org/deekej\n\n"
"Optional arguments";

client::settings_tuple SETTINGS;                            // Stores the client settings for other processes.


/* ****************************************************************************************************************** *
 ~ ~~~[ AUXILIARY FUNCTIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

/**
 *  Process the client command line arguments and stores them to SETTINGS global variable.
 *
 *  @param[in] argc Number of command line arguments.
 *  @param[in] argv Array of the arguments.
 */
void process_params(int argc, char *argv[])
{{{
  std::string process_name {argv[0]};

  int           port;
  long          hello_interval;
  long          max_ping;
  std::string   IPv4_address;
  std::string   config_file_loc;

  try {
    namespace params = boost::program_options;

    params::options_description help(HELP_STRING, 100);
    help.add_options() ("help,h", "show this message and exit");
    help.add_options() ("ip,4", params::value<std::string>(&IPv4_address)->default_value("localhost"),
                        "specify the IPv4 address of the server (default: localhost)");

    help.add_options() ("port,p", params::value<int>(&port)->default_value(49429),
                        "specify the listening port of the server (default: 49429)");

    help.add_options() ("keep-alive,k", params::value<long>(&hello_interval)->default_value(5000),
                        "specify the keep-alive interval in ms (default: 5000)");

    help.add_options() ("config,c", params::value<std::string>(&config_file_loc)->default_value("~/.maze_client"),
                        "specify the config file location (default: ~/.maze_client)");

    help.add_options() ("timeout,t", params::value<long>(&max_ping)->default_value(20000),
                        "specify the max ping between client and server (default: 20000)");

    params::variables_map var_map;
    params::store(params::parse_command_line(argc, argv, help), var_map);
    params::notify(var_map);

    if (var_map.count("help")) {
      std::cout << help << std::endl;
      exit(client::exit_codes::NO_ERROR);
    }

    if (var_map["port"].as<int>() < 1) {
      std::cerr << process_name << ": Error: the argument ('" << var_map["port"].as<int>();
      std::cerr << "') for option '--port' is invalid" << std::endl;
      exit(client::exit_codes::E_WRONG_PARAMS);
    }

    if (var_map["keep-alive"].as<long>() < 500 || var_map["keep-alive"].as<long>() > 10000) {
      std::cerr << process_name << ": Error: the argument ('" << var_map["keep-alive"].as<long>();
      std::cerr << "') for option '--keep-alive' is invalid" << std::endl;
      exit(client::exit_codes::E_WRONG_PARAMS);
    }

    if (var_map["timeout"].as<long>() < 1000) {
      std::cerr << process_name << ": Error: the argument ('" << var_map["timeout"].as<long>();
      std::cerr << "') for option '--timeout' is invalid" << std::endl;
      exit(client::exit_codes::E_WRONG_PARAMS);
    }
    
    std::get<client::PROCESS_NAME>(SETTINGS) = process_name;
    std::get<client::IPv4_ADDRESS>(SETTINGS) = IPv4_address;
    std::get<client::CONFIG_FILE_LOC>(SETTINGS) = config_file_loc;
    std::get<client::HELLO_INTERVAL>(SETTINGS) = hello_interval;
    std::get<client::MAX_PING>(SETTINGS) = max_ping;

    // FIXME: Make proper int -> string conversion:
    std::get<client::SERVER_PORT>(SETTINGS) = port;
    return;
  }
  catch (std::exception & ex) {
    std::cerr << process_name << ": Error: " << ex.what() << std::endl;
    exit(client::exit_codes::E_WRONG_PARAMS);
  }
  catch (...) {
    std::cerr << process_name << ": Error: Exception of unknown type during arguments processing!" << std::endl;
    exit(client::exit_codes::E_UNKNOWN_EXCEPT);
  }
}}}


/* ****************************************************************************************************************** *
 ~ ~~~[ MAIN FUNCTION ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

/**
 *  Main function is here.
 */
int main(int argc, char *argv[])
{{{
  std::ios::sync_with_stdio(false);

  process_params(argc, argv);

  // // // // // // // // //

  return client::exit_codes::NO_ERROR;
}}}

/* ****************************************************************************************************************** *
 ~ ~~~[ END OF THE MAZED_MAIN.CC ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

