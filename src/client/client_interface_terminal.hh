/**
 * @file      client_interface_terminal.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.6
 * @brief     This file contains specialized (derived) class of ABC::user_interface for simple terminal UI.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF CLIENT_INTERFACE_TERMINAL.HH ]********************************************************************** *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_CLIENT_INTERFACE_TERMINAL_HH
#define H_GUARD_CLIENT_INTERFACE_TERMINAL_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <fstream>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <streambuf>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "abc_user_interface.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ TERMINAL_INTERFACE CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace client {

  class game_instance;

  /**
   * Class for creating and running the TERMINAL interface.
   */
  class terminal_interface : public ABC::user_interface {
      std::unique_ptr<boost::thread>                    pu_input_thread_;
      std::unique_ptr<boost::thread>                    pu_output_thread_;
      std::unique_ptr<boost::thread>                    pu_maze_thread_;
      
      boost::upgrade_mutex                              run_mutex_;
      bool                                              run_ {true};

      boost::condition_variable                         output_req_;
      boost::mutex                                      output_mutex_;
      boost::barrier                                    output_barrier_;
      std::string                                       output_string_;
      std::queue<std::string>                           output_queue_;
      bool                                              print_newline_ {true};

      std::string                                       user_input_;

      boost::asio::io_service                           io_service_;
      boost::asio::deadline_timer                       timer_;
      std::ofstream                                     terminal_output_;
      pid_t                                             terminal_output_pid_;
      client::game_instance                             *p_instance_ {NULL};

      std::string                                       process_name_;
      
      static const std::string                          welcome_message_;
      static const std::string                          exit_message_;
      static const std::string                          help_start_string_;
      static const std::string                          help_end_string_;
      static const std::string                          prompt_;
      static const std::string                          prompt_reply_;

      static const std::multimap<std::string, enum E_user_command>            mappings_;
      static const std::map<std::size_t, std::pair<std::string, std::string>> help_lobby_commands_;
      static const std::map<std::size_t, std::pair<std::string, std::string>> help_ctrl_commands_;
      static const std::map<std::size_t, std::pair<std::string, std::string>> help_connection_commands_;

      // // // // // // // // // // //

      void input_thread();
      void output_thread();
      void display_help();

      std::string get_word();

      inline void report_istream_error();
      inline void report_timer_error();
      void redraw_maze();
      void redraw_maze_handler(const boost::system::error_code &error);

      // // // // // // // // // // //

    public:
      terminal_interface(boost::condition_variable &action_req, boost::mutex &action_req_mutex, boost::barrier &barrier,
                         enum E_user_command &command_storage, std::string &additional_data_storage,
                         std::string process_name);
     ~terminal_interface();

      void initialize() override;
      void terminate() override;

      void display_message(const std::string &message) override;
      
      bool maze_run(client::game_instance *instance_ptr, const std::string zoom) override;
      void maze_stop() override;

      void maze_pause() override;
      void maze_continue() override;
  };
}


/* ****************************************************************************************************************** *
 * ***[ END OF CLIENT_TERMINAL_INTERFACE.HH ]************************************************************************ *
 * ****************************************************************************************************************** */

#endif

