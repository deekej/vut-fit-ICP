/**
 * @file      mazed_main.cc
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @note      This file is build upon official example of Boost FORK PROCESS PER CONNECTION by Christopher M. Kohlhoff.
 * @version   1.0
 * @brief     Server daemon of Maze-game for ICP course @ BUT FIT, 2014.
 *
 * @detailed  This is a main program for running the server daemon of the game. It can be launched with several options
 *            which allows modifying the server's behaviour. See the help of the program for more info.
 */

/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_MAIN.HH ]************************************************************************************* *
 * ****************************************************************************************************************** */


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

// C++ header files:
#include <iterator>
#include <string>
#include <tuple>

// Boost header files:
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

// POSIX header files:
#include <syslog.h>

// Program header files:
#include "mazed_globals.hh"
#include "mazed_server.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ GLOBAL VARIABLES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

const std::string HELP_STRING =
"This is the server daemon for MAZE-GAME application,\n"
"which is the part from project of ICP course @ BUT FIT, Czech Republic, 2014.\n\n"
"Version:       0.1\n"
"Written by:    Dee'Kej <deekej@linuxmail.org>\n"
"Website:       https://bitbucket.org/deekej\n\n"
"Optional arguments";

mazed::settings_tuple SETTINGS;                             // Stores the server settings for other threads.

int LOG_FILE_FD {-1};                                       // Log file descriptor backup for closing at exit.
std::string DAEMON_LOG_FILE {"daemon.log"};                 // Name for starting log file of this process.
mazed::log_level LOGGING_LEVEL {mazed::log_level::NONE};    // Auxiliary variable for logging.


/* ****************************************************************************************************************** *
 ~ ~~~[ AUXILIARY FUNCTIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

/**
 *  Process the server daemon command line arguments and stores them to SETTINGS global variable.
 *
 *  @param[in] argc Number of command line arguments.
 *  @param[in] argv Array of the arguments.
 */
void process_params(int argc, char *argv[])
{{{
  std::string process_name {argv[0]};

  unsigned char logging;
  int           port;
  long          sleep;
  long          timeout;
  std::string   players_dir;
  std::string   mazes_dir;
  std::string   saves_dir;
  std::string   log_dir;
  std::string   server_log_file = "server.log";     // NOTE: Currently not supported parameter.

  try {
    namespace params = boost::program_options;

    params::options_description help(HELP_STRING, 100);
    help.add_options() ("help,h", "show this message and exit");
    help.add_options() ("port,p", params::value<int>(&port)->default_value(49429),
                        "specify the listening port (default: 49429)");

    help.add_options() ("sleep,s", params::value<long>(&sleep)->default_value(20),
                        "specify the sleep duration in ms (default: 20)");

    help.add_options() ("timeout,t", params::value<long>(&timeout)->default_value(20000),
                        "specify the maximum ping in ms (default 20000)");

    help.add_options() ("players-dir,i", params::value<std::string>(&players_dir)->default_value("./players"),
                        "specify the folder for players information (default: ./players)");

    help.add_options() ("mazes-dir,m", params::value<std::string>(&mazes_dir)->default_value("./mazes"),
                        "specify the game's mazes folder (default: ./mazes)");

    help.add_options() ("saves-dir,o", params::value<std::string>(&saves_dir)->default_value("./mazes/saves"),
                        "specify the folder of players' saves (default: ./mazes/saves");

    help.add_options() ("log-dir,l", params::value<std::string>(&log_dir)->default_value("/tmp/mazed"),
                        "specify the log folder (default: /tmp/mazed)");

    help.add_options() ("logging", params::value<unsigned char>(&logging)->default_value('3'),
                        "enables or disables logging [0|1|2|3] (default: 3)");

    params::variables_map var_map;
    params::store(params::parse_command_line(argc, argv, help), var_map);
    params::notify(var_map);

    if (var_map.count("help")) {
      std::cout << help << std::endl;
      exit(mazed::exit_codes::NO_ERROR);
    }

    if (var_map["port"].as<int>() < 1) {
      std::cerr << process_name << ": Error: the argument ('" << var_map["port"].as<int>();
      std::cerr << "') for option '--port' is invalid" << std::endl;
      exit(mazed::exit_codes::E_WRONG_PARAMS);
    }

    if (var_map["sleep"].as<long>() < 1) {
      std::cerr << process_name << ": Error: the argument ('" << var_map["sleep"].as<long>();
      std::cerr << "') for option '--sleep' is invalid" << std::endl;
      exit(mazed::exit_codes::E_WRONG_PARAMS);
    }

    if (var_map["timeout"].as<long>() < 1000) {
      std::cerr << process_name << ": Error: the argument ('" << var_map["timeout"].as<long>();
      std::cerr << "') for option '--timeout' is invalid" << std::endl;
      exit(mazed::exit_codes::E_WRONG_PARAMS);
    }

    std::get<mazed::PLAYERS_FOLDER>(SETTINGS) = players_dir;
    std::get<mazed::SAVES_FOLDER>(SETTINGS) = saves_dir;
    std::get<mazed::MAZES_FOLDER>(SETTINGS) = mazes_dir;
    std::get<mazed::LOG_FOLDER>(SETTINGS) = log_dir;
    std::get<mazed::SERVER_LOG_FILE>(SETTINGS) = server_log_file;
    std::get<mazed::SLEEP_INTERVAL>(SETTINGS) = sleep;
    std::get<mazed::MAX_PING>(SETTINGS) = timeout;
    std::get<mazed::SERVER_PORT>(SETTINGS) = port;
    std::get<mazed::LOGGING_LEVEL>(SETTINGS) = mazed::log_level::NONE;       // Avoiding too-early logging.
    LOGGING_LEVEL = static_cast<mazed::log_level>(logging - '0');

    return;
  }
  catch (std::exception & ex) {
    std::cerr << process_name << ": Error: " << ex.what() << std::endl;
    exit(mazed::exit_codes::E_WRONG_PARAMS);
  }
  catch (...) {
    std::cerr << process_name << ": Error: Exception of unknown type during arguments processing!" << std::endl;
    exit(mazed::exit_codes::E_UNKNOWN_EXCEPT);
  }
}}}


/**
 *  Cleaning function registered to be called at exit for closing the log file.
 */
void close_log()
{{{
  close(LOG_FILE_FD);
  return;
}}}


/* ****************************************************************************************************************** *
 ~ ~~~[ MAIN FUNCTION ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

/**
 *  Main function is here.
 */
int main(int argc, char *argv[])
{{{
  process_params(argc, argv);

  // // // // // // // // //

  try {
    boost::asio::io_service io_service;

    // Initializing the server before becoming a daemon. If the process is started from shell, all initial errors will
    // be reported to the user.
    mazed::server server(io_service, SETTINGS);

    // Making safe fork by informing io_service about upcoming fork:
    io_service.notify_fork(boost::asio::io_service::fork_prepare);

    // Forking and exit the parent. This returns control to the user if the daemon was started from shell.
    if (pid_t pid = fork())
    {
      if (pid > 0)
      {
        return mazed::exit_codes::NO_ERROR;         // We're in the parent process -> exiting.
      }
      else
      {
        syslog(LOG_ERR | LOG_USER, "First fork failed: %s", strerror(errno));
        return mazed::exit_codes::E_FORKING;
      }
    }

    setsid();     // Make the process a new session leader, this detaches it from the terminal.
    chdir("/");   // Make sure we don't block any mounted filesystem from unmounting.
    umask(0);     // Clearing the files permissions mask for upcoming files creating so there's no restriction.

    // Make sure the server doesn't acquire a controlling terminal.
    if (pid_t pid = fork())
    {
      if (pid > 0)
      {
        return mazed::exit_codes::NO_ERROR;
      }
      else
      {
        syslog(LOG_ERR | LOG_USER, "Second fork failed: %s", strerror(errno));
        return mazed::exit_codes::E_FORKING;
      }
    }

    // Closing standard streams. This is final step for decoupling from the starting terminal.
    close(0);
    close(1);
    close(2);

    // Disabling standard input. (Blocking file/pipe input, etc.)
    if (open("/dev/null", O_RDONLY) < 0)
    {
      syslog(LOG_ERR | LOG_USER, "Unable to open /dev/null: %s", strerror(errno));
      return mazed::exit_codes::E_FORKING;
    }

    // Create the log directory for the daemon and client handlers, if it doesn't exists yet:
    boost::filesystem::path log_dir(std::get<mazed::LOG_FOLDER>(SETTINGS));

    if (boost::filesystem::exists(log_dir) || boost::filesystem::create_directory(log_dir)) {
      DAEMON_LOG_FILE.insert(0, "/");
      DAEMON_LOG_FILE.insert(0, std::get<mazed::LOG_FOLDER>(SETTINGS));
    }
    else {
      syslog(LOG_ERR | LOG_USER, "Failed to create log folder: %s", std::get<mazed::LOG_FOLDER>(SETTINGS).c_str());
      return mazed::exit_codes::E_FOLDER_CREATE;
    }

    // Sending the standard output of the daemon to the file:
    // NOTE: The server itself has it's own log file.
    const int flags = O_WRONLY | O_CREAT | O_APPEND;
    const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    if ((LOG_FILE_FD = open(DAEMON_LOG_FILE.c_str(), flags, mode)) < 0)
    {
      syslog(LOG_ERR | LOG_USER, "Unable to open output file %s: %s", DAEMON_LOG_FILE.c_str(), strerror(errno));
      return mazed::exit_codes::E_OPEN;
    }
    else {
      std::atexit(close_log);
    }

    // Sending the standard error stream to the same file:
    if (dup(1) < 0)
    {
      syslog(LOG_ERR | LOG_USER, "Unable to dupplicate the output descriptor: %s", strerror(errno));
      return mazed::exit_codes::E_OPEN;
    }

    // Informing the io_service that the process has become a proper daemon process:
    io_service.notify_fork(boost::asio::io_service::fork_child);

    std::get<mazed::LOGGING_LEVEL>(SETTINGS) = LOGGING_LEVEL;     // Starting the logging if requested.

    syslog(LOG_INFO | LOG_USER, "Server daemon started");
    server.run();                                                 // Starting the game server itself.
    syslog(LOG_INFO | LOG_USER, "Server daemon stopped");

    return mazed::exit_codes::NO_ERROR;
  }
  catch (std::exception& e)
  {
    syslog(LOG_ERR | LOG_USER, "Exception: %s", e.what());
  }

  // No return / exit here - > The daemon is running in endless loop. Use SIGINT, SIGTERM, SIGKILL to terminate process.
}}}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_MAIN.HH ]*************************************************************************************** *
 * ****************************************************************************************************************** */

