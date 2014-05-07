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

const std::string HELP_STRING =
"This is the server daemon for MAZE-GAME application,\n"
"which is the part from project of ICP course @ BUT FIT, Czech Republic, 2014.\n\n"
"Version:       0.1\n"
"Written by:    Dee'Kej <deekej@linuxmail.org>\n"
"Website:       https://bitbucket.org/deekej\n\n"
"Optional arguments";

mazed::settings_tuple SETTINGS;

int LOG_FILE_FD {-1};
std::string DAEMON_LOG_FILE {"daemon.log"};
mazed::log_level LOGGING_LEVEL {mazed::log_level::NONE};

// // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // //

void process_params(int argc, char *argv[])
{
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
  catch (std::exception & e) {
    std::cerr << process_name << ": Error: " << e.what() << std::endl;
    exit(mazed::exit_codes::E_WRONG_PARAMS);
  }
  catch (...) {
    std::cerr << process_name << ": Error: Exception of unknown type during arguments processing!" << std::endl;
    exit(mazed::exit_codes::E_UNKNOWN_EXCEPT);
  }
}

void close_log()
{
  close(LOG_FILE_FD);
}

void terminate_children(const boost::system::error_code &error, int signal_number)
{
  if (error == false && (signal_number == SIGINT || signal_number == SIGTERM)) {
    syslog(LOG_INFO | LOG_USER, "terminating all child processes");
    pid_t process_group = getpgrp();
    killpg(process_group, SIGTERM);
  }
  else {
    syslog(LOG_ERR | LOG_USER, "Error: %s", error.message().c_str());
  }
}

/* ****************************************************************************************************************** *
 ~ ~~~[ MAIN FUNCTION ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */
int main(int argc, char *argv[])
{
  // std::ios::sync_with_stdio(false);

  process_params(argc, argv);

  // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // // //

  try {
    boost::asio::io_service io_service;

    // Initialise the server before becoming a daemon. If the process is
    // started from a shell, this means any errors will be reported back to the
    // user.
    mazed::server server(io_service, SETTINGS);

    // Register signal handlers so that the daemon may be shut down. You may
    // also want to register for other signals, such as SIGHUP to trigger a
    // re-read of a configuration file.
    boost::asio::signal_set io_service_stop(io_service, SIGINT, SIGTERM);
    io_service_stop.async_wait(boost::bind(&boost::asio::io_service::stop, &io_service));

    boost::asio::signal_set children_kill(io_service, SIGINT, SIGTERM);
    children_kill.async_wait(terminate_children);

    // Inform the io_service that we are about to become a daemon. The
    // io_service cleans up any internal resources, such as threads, that may
    // interfere with forking.
    io_service.notify_fork(boost::asio::io_service::fork_prepare);

    // Fork the process and have the parent exit. If the process was started
    // from a shell, this returns control to the user. Forking a new process is
    // also a prerequisite for the subsequent call to setsid().
    if (pid_t pid = fork())
    {
      if (pid > 0)
      {
        // We're in the parent process and need to exit.
        //
        // When the exit() function is used, the program terminates without
        // invoking local variables' destructors. Only global variables are
        // destroyed. As the io_service object is a local variable, this means
        // we do not have to call:
        //
        //   io_service.notify_fork(boost::asio::io_service::fork_parent);
        //
        // However, this line should be added before each call to exit() if
        // using a global io_service object. An additional call:
        //
        //   io_service.notify_fork(boost::asio::io_service::fork_prepare);
        //
        // should also precede the second fork().
        return mazed::exit_codes::NO_ERROR;
      }
      else
      {
        syslog(LOG_ERR | LOG_USER, "First fork failed: %s", strerror(errno));
        return mazed::exit_codes::E_FORKING;
      }
    }



    // Make the process a new session leader. This detaches it from the
    // terminal.
    setsid();

    // A process inherits its working directory from its parent. This could be
    // on a mounted filesystem, which means that the running daemon would
    // prevent this filesystem from being unmounted. Changing to the root
    // directory avoids this problem.
    chdir("/");

    // The file mode creation mask is also inherited from the parent process.
    // We don't want to restrict the permissions on files created by the
    // daemon, so the mask is cleared.
    umask(0);

    // A second fork ensures the process cannot acquire a controlling terminal.
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

    // Close the standard streams. This decouples the daemon from the terminal
    // that started it.
    close(0);
    close(1);
    close(2);

    // We don't want the daemon to have any standard input.
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

    // Send standard output to a log file.
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


    // Also send standard error to the same log file.
    if (dup(1) < 0)
    {
      syslog(LOG_ERR | LOG_USER, "Unable to dupplicate the output descriptor: %s", strerror(errno));
      return mazed::exit_codes::E_OPEN;
    }

    // Inform the io_service that we have finished becoming a daemon. The
    // io_service uses this opportunity to create any internal file descriptors
    // that need to be private to the new process.
    io_service.notify_fork(boost::asio::io_service::fork_child);

    std::get<mazed::LOGGING_LEVEL>(SETTINGS) = LOGGING_LEVEL;     // Starting logging if requested.

    // The io_service can now be used normally.
    syslog(LOG_INFO | LOG_USER, "server daemon started");
    server.run();
    syslog(LOG_INFO | LOG_USER, "server daemon stopped");
  }
  catch (std::exception& e)
  {
    syslog(LOG_ERR | LOG_USER, "Exception: %s", e.what());
  }
}
