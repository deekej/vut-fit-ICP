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

  /**
   * Class for creating and running the TERMINAL interface.
   */
  class terminal_interface : public ABC::user_interface {
      std::unique_ptr<boost::thread>                    pu_input_thread_;
      std::unique_ptr<boost::thread>                    pu_output_thread_;
      
      boost::upgrade_mutex                              run_mutex_;
      bool                                              run_ {true};

      boost::condition_variable                         output_req_;
      boost::mutex                                      output_mutex_;
      boost::barrier                                    output_barrier_;
      std::queue<std::string>                           output_queue_;

      std::string                                       user_input_;
      
      static const std::string                          welcome_message_;
      static const std::string                          exit_message_;
      static const std::string                          help_start_string_;
      static const std::string                          help_end_string_;
      static const std::string                          prompt_;
      static const std::string                          prompt_reply_;
      static const std::string                          terminate_string_;

      static const std::multimap<std::string, enum E_user_command>            mappings_;
      static const std::map<std::size_t, std::pair<std::string, std::string>> help_lobby_commands_;
      static const std::map<std::size_t, std::pair<std::string, std::string>> help_ctrl_commands_;

      // // // // // // // // // // //

      void input_thread();
      void output_thread();
      void display_help();

      std::string get_word();

      inline void report_istream_error();

      // // // // // // // // // // //

    public:
      terminal_interface(boost::condition_variable &action_req, boost::mutex &action_req_mutex, boost::barrier &barrier,
                         enum E_user_command &command_storage, std::string &additional_data_storage);
     ~terminal_interface();

      void initialize() override;
      void terminate() override;

      void display_message(const std::string &message) override;

      void start_maze() override;
      void stop_maze() override;
  };
}


/* ****************************************************************************************************************** *
 * ***[ END OF CLIENT_TERMINAL_INTERFACE.HH ]************************************************************************ *
 * ****************************************************************************************************************** */

#endif

