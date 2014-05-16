/**
 * @file      client_interface_terminal.cc
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.6
 * @brief     This file contains implementations of the member functions of the client::terminal_interface class.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF CLIENT_INTERFACE_TERMINAL.CC ]********************************************************************** *
 * ****************************************************************************************************************** */


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <iostream>
#include <limits>

#include "client_interface_terminal.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ MEMBER FUNCTIONS IMPLEMENTATIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace client {

  // Static strings of the terminal_interface class:
  const std::string terminal_interface::welcome_message_ {
    "|-** Welcome to MAZE-game!\n"
    "|-** Write 'help' to see available commands or 'quit' / 'exit' to end the program."
  };

  const std::string terminal_interface::exit_message_ {
    "|->> OK, bye!"
  };

  const std::string terminal_interface::help_start_string_ {
    "   Available commands:"
  };

  const std::string terminal_interface::help_end_string_ {
    "   NOTE: In case program gets stuck you can press 'CTRL^C' to end it."
  };

  const std::string terminal_interface::prompt_ {"|-?? "};
  const std::string terminal_interface::prompt_reply_ {"|->> "};

  const std::multimap<std::string, enum ABC::user_interface::E_user_command> terminal_interface::mappings_ {
    {"left", E_user_command::LEFT},
    {"right", E_user_command::RIGHT},
    {"up", E_user_command::UP},
    {"down", E_user_command::DOWN},
    {"stop", E_user_command::STOP},
    {"take", E_user_command::TAKE_OPEN},
    {"open", E_user_command::TAKE_OPEN},
    {"run", E_user_command::PAUSE_CONTINUE},
    {"pause", E_user_command::PAUSE_CONTINUE},
    {"continue", E_user_command::PAUSE_CONTINUE},
    {"list-mazes", E_user_command::LIST_MAZES},
    {"list-saves", E_user_command::LIST_SAVES},
    {"list-running", E_user_command::LIST_RUNNING},
    {"start-game", E_user_command::GAME_START},
    {"restart", E_user_command::GAME_RESTART},
    {"terminate", E_user_command::GAME_TERMINATE},
    {"join-game", E_user_command::GAME_JOIN},
    {"leave-game", E_user_command::GAME_LEAVE},
    {"load-last", E_user_command::GAME_LOAD_LAST},
    {"load-game", E_user_command::GAME_LOAD},
    {"save-game", E_user_command::GAME_SAVE},
    {"show-stats", E_user_command::GAME_SHOW_STATS},
    {"set-nick", E_user_command::SET_NICK},
    {"quit", E_user_command::EXIT},
    {"exit", E_user_command::EXIT},
    {"help", E_user_command::HELP},
  };

  const std::map<std::size_t, std::pair<std::string, std::string>> terminal_interface::help_lobby_commands_ {
    {0, {"help", "display this help page"}},
    {1, {"quit/exit", "exit the program (same as pressing 'CTRL^C')"}},
    {2, {"list-mazes", "display available mazes to play from start"}},
    {3, {"list-saves", "display available saved instances to play"}},
    {4, {"list-running", "display all running game instances on the server"}},
    {5, {"start-game [number]", "new game of a maze specified by the number"}},
    {6, {"restart", "restart the current game instance"}},
    {7, {"terminate", "end a current game instance and return to a lobby"}},
    {8, {"join-game [number]", "join a game instance specified by the number"}},
    {9, {"leave-game", "leave current game instance and return to lobby"}},
    {10, {"load-last", "load your last save game, if any"}},
    {11, {"load-game [number]", "load a saved game instance specified by the number"}},
    {12, {"save-game", "save current game instance on the server"}},
    {13, {"show-stats", "show stats of last game"}},
    {14, {"set-nick [nick]", "set new nickname to [nick]"}},
  };

  const std::map<std::size_t, std::pair<std::string, std::string>> terminal_interface::help_ctrl_commands_ {
    {0, {"run", "start the new or loaded game"}},
    {1, {"pause", "pause the current game"}},
    {2, {"continue", "continue in the current game"}},
    {3, {"take", "take the nearby key"}},
    {4, {"open", "open the nearby gate"}},
    {5, {"left", "go left until any object is reached"}},
    {6, {"right", "go right until any object is reached"}},
    {7, {"up", "go up until any object is reached"}},
    {8, {"down", "go down until any object is reached"}},
    {9, {"stop", "stop the movement"}},
  };

  // // // // // // // // // // // //

  terminal_interface::terminal_interface(boost::condition_variable &action_req, boost::mutex &action_req_mutex,
                                         boost::barrier &barrier, enum E_user_command &command_storage,
                                         std::string &additional_data_storage) :
    user_interface(action_req, action_req_mutex, barrier, command_storage, additional_data_storage), output_barrier_(2)
  {{{
    return;
  }}}


  /**
   * Overridden destructor. Making sure we don't get abort() or get stuck while unexpected destruction started.
   */
  terminal_interface::~terminal_interface()
  {{{
    run_mutex_.lock_upgrade();
    {
      run_ = false;
    }
    run_mutex_.unlock_upgrade();
  
    // Notify the output thread:
    output_req_.notify_one();
    
    // Threads should be able to successfully join right now:
    if (pu_input_thread_ && (*pu_input_thread_).joinable() == true) {
      (*pu_input_thread_).join();
    }

    if (pu_output_thread_ && (*pu_output_thread_).joinable() == true) {
      (*pu_output_thread_).join();
    }

    return;
  }}}

  // // // // // // // // // // // //
  
  /**
   * Starts the output and input thread.
   */
  void terminal_interface::initialize()
  {{{
    std::cout << welcome_message_ << std::endl;

    pu_output_thread_ = std::unique_ptr<boost::thread>(new boost::thread(&terminal_interface::output_thread, this));
    pu_input_thread_ = std::unique_ptr<boost::thread>(new boost::thread(&terminal_interface::input_thread, this));

    return;
  }}}

  
  /**
   * Terminates the output and input thread, allowing another subsequent call of initialize.
   */
  void terminal_interface::terminate()
  {{{
    run_mutex_.lock_upgrade();
    {
      run_ = false;
    }
    run_mutex_.unlock_upgrade();

    // Notify the output thread:
    output_req_.notify_one();

      
    // Thread should be able to successfully join right now:
    if (pu_input_thread_ && (*pu_input_thread_).joinable() == true) {
      (*pu_input_thread_).join();
      pu_input_thread_ = NULL;          // Make sure we don't call .join() multiple times.
    }

    if (pu_output_thread_ && (*pu_output_thread_).joinable() == true) {
      (*pu_output_thread_).join();
      pu_output_thread_ = NULL;         // Make sure we don't call .join() multiple times.
    }

    return;
  }}}
  
  // // // // // // // // // // // //

  /**
   * Runs an input thread for processing user's commands.
   */
  void terminal_interface::input_thread()
  {{{
    output_barrier_.wait();             // Wait for output thread.
    init_barrier_.wait();               // Wait for notification from client that we can start.
    

    bool run {true};
    bool print_prompt {false};          // Switch for making output prettier.

    std::string input;                  // String to be read into.
    enum E_user_command last_command;   // Auxiliary variable for extracting user's command.


    do {
      // Print the prompt only if requested so we don't end up with 2 prompts:
      if (print_prompt == true) {
        output_mutex_.lock();
        {
          std::cout << prompt_ << std::flush;
        }
        output_mutex_.unlock();
      }

      input = get_word();               // Read one word from std::cin.

      if (std::cin.bad() == true) {
        report_istream_error();         // Error occurred, bail out.
        run = false;
      }
      else if (input == "\n") {
        print_prompt = true;
        continue;                       // Simple ENTER pressed, nothing to do.
      }
      else if (input.length() == 0) {
        // CTRL+D was pressed, making output consistent (read prettier ^_^):
        output_mutex_.lock();
        {
          std::cout << std::endl;
        }
        output_mutex_.unlock();

        print_prompt = true;
        continue;
      }
      else if (mappings_.count(input)) {
        // Command is known, getting the associated enumerate value:
        last_command = mappings_.find(input)->second;
        input.clear();                  // Prepare for next reading.

        switch (last_command) {
          case HELP :
            display_help();
            print_prompt = true;
            continue;
          
          case EXIT :
            output_mutex_.lock();
            {
              std::cout << exit_message_ << std::endl;
            }
            output_mutex_.unlock();

            run = false;
            break;

          case STOP :
            print_prompt = false;
            break;
          
          // Commands which require additional argument:
          case GAME_START :
          case GAME_JOIN :
          case GAME_LOAD :
          case SET_NICK :
            input = get_word();

            if (std::cin.bad() == true) {
              report_istream_error();   // Error occurred, bail out.
              run = false;
              continue;
            }
            
            print_prompt = true;
            break;
          
          default :
            print_prompt = true;
            break;
        }

        // Prepare the processed command for the mediator:
        action_req_mutex_.lock();
        {
          command_ = last_command;
          additional_data_ = input;
          action_req_.notify_one();
        }
        action_req_mutex_.unlock();
      }
      else {
        display_message("Error: Unknown command '" + input + "' (write 'help' to see available commands)");
        print_prompt = false;
      }

    } while (run == true);

    return;
  }}}


  /**
   * Runs an output thread for displaying messages to user.
   */
  void terminal_interface::output_thread()
  {{{
    // Acquire the lock as a first one and wait for other thread:
    boost::unique_lock<boost::mutex> output_lock(output_mutex_);
    output_barrier_.wait();
    
    // run_mutex_.lock();                  // NOTE: Enable again in case of any deadlock!
    do {
      run_mutex_.unlock();
      
      // Print all of the queue contents, if any, as one output:
      while (output_queue_.empty() != true) {
        std::cout << prompt_reply_ << output_queue_.front() << "\n";
        output_queue_.pop();
      }

      std::cout << prompt_ << std::flush;

      output_req_.wait(output_lock);    // Wait for next request.

      run_mutex_.lock();
    } while (run_ == true);
    run_mutex_.unlock();

    return;
  }}}

  // // // // // // // // // // // //

  /**
   * Reads the user's input and returns it as a string, skipping all the whitespace characters preceding it. In case of
   * badbit error the bit stays unset so it can be checked. The eofbit and failbit are reset after the reading.
   *
   * @return  String that was read or empty string in case EOF occurred.
   */
  std::string terminal_interface::get_word()
  {{{
    user_input_.clear();

    int ch;

    // Skip the initial whitespace characters except the '\n' which indicates just simple pressing of ENTER key:
    while ((ch = std::cin.get()) != EOF && ch != '\n' && std::isspace(static_cast<char>(ch))) {
      ;
    }

    if (ch == '\n') {
      return "\n";
    }

    if (std::cin.good() == false) {
      if (std::cin.bad() == false) {
        std::cin.clear();               // We're ignoring the eofbit and failbit.
      }
      
      return user_input_;               // Empty string.
    }

    // Last key wasn't ENTER, store it and read characters until whitespace character occurs:
    do {
      user_input_.push_back(ch);
      ch = std::cin.get();
    } while (std::isspace(static_cast<char>(ch)) == false);

    std::cin.clear();                   // We're ignoring the eofbit and failbit.
    return user_input_;
  }}}
  

  /**
   * Reports an error of the input stream to the mediator.
   */
  inline void terminal_interface::report_istream_error()
  {{{
    action_req_mutex_.lock();
    {
      command_ = E_user_command::ERROR_INPUT_STREAM;
      action_req_.notify_one();
    }
    action_req_mutex_.unlock();

    return;
  }}}

  // // // // // // // // // // // //

  /**
   * Stores a string into a queue of messages that are supposed to be printed out on the std::cout and informs the
   * output thread to wake up.
   *
   * @param[in]   message   String to be displayed to the user.
   */
  void terminal_interface::display_message(const std::string &message)
  {{{
    output_mutex_.lock();
    {
      output_queue_.push(message);
    }
    output_mutex_.unlock();

    // We're notifying the output thread after the unlock, so other threads can also store into the queue, allowing less
    // wake ups of output thread.
    output_req_.notify_one();

    return;
  }}}


  /**
   * Displays the help to the user.
   */
  void terminal_interface::display_help()
  {{{
    output_mutex_.lock();
    {
      std::cout << "  --------------------\n" << help_start_string_ << "\n  --------------------\n";

      for (auto lobby: help_lobby_commands_) {
        std::cout << "| " << std::setw(20) << std::left << lobby.second.first;
        std::cout << " - " << lobby.second.second << "\n";
      }

      std::cout << "  --------------------\n";

      for (auto ctrl: help_ctrl_commands_) {
        std::cout << "| " << std::setw(20) << std::left << ctrl.second.first;
        std::cout << " - " << ctrl.second.second << "\n";
      }

      std::cout << "  --------------------\n" << help_end_string_ << "\n  --------------------" << std::endl;
    }
    output_mutex_.unlock();
  }}}

  // // // // // // // // // // // //

  /**
   * TODO: This is dummy function for now.
   */
  void terminal_interface::start_maze()
  {{{
    return;
  }}}

  
  /**
   * TODO: This is dummy function for now.
   */
  void terminal_interface::stop_maze()
  {{{
    return;
  }}}
}


/* ****************************************************************************************************************** *
 * ***[ END OF CLIENT_TERMINAL_INTERFACE.CC ]************************************************************************ *
 * ****************************************************************************************************************** */

