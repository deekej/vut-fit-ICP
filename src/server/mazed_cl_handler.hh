/**
 * @file      mazed_cl_handler.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.3
 * @brief     Contains class definition where each instance of this class takes care of one new client.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_CL_HANDLER.HH ]******************************************************************************* *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_CLIENT_HANDLER_HH
#define H_GUARD_CLIENT_HANDLER_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <fstream>
#include <memory>
#include <vector>

#include <boost/asio.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/thread.hpp>

#include "mazed_globals.hh"
#include "../protocol.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ CLIENT_HANDLER CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace asio = boost::asio;
using namespace protocol;

namespace mazed {

  /**
   * Complex class for handling the client's requests, starting the game instances and synchronizing threads used for
   * handling the clients requests. This class uses 3 threads just for running. Do not mess with the synchronization!
   *
   * @note The game instance itself is started in another thread, which is running independently.
   */
  class client_handler {
      using tcp = boost::asio::ip::tcp;

      // References to already opened connection:
      tcp::socket                       &socket_;
      asio::io_service                  &io_service_;

      // Pointer to serialization over the established connection:
      std::unique_ptr<protocol::tcp_serialization> pu_tcp_connect_;
      
      // Objects for checking client's connection timeout:
      asio::io_service                  timeout_io_service_;
      asio::deadline_timer              timeout_;

      // Necessary objects for proper synchronization:
      boost::condition_variable         action_req_;
      boost::condition_variable         asio_continue_;
      boost::mutex                      action_req_mutex_;
      boost::mutex                      asio_mutex_;
      boost::mutex                      output_mutex_;
      boost::upgrade_mutex              run_mutex_;
      boost::barrier                    init_barrier_;
      bool run_                         {true};
      
      // Logging file, logging file mutex & formatting object for date/time string:
      boost::mutex                      log_mutex_;
      std::ofstream                     log_file_;
      std::locale                       dt_format_;
      
      // Incoming/outcoming messages' buffers:
      std::vector<protocol::message>    messages_in_;
      std::vector<protocol::message>    messages_out_;
      protocol::message                 message_in_;
      protocol::message                 message_out_;

      using pf_message_handler = void (client_handler::*)();
      
      // Array of pointers to message function handlers:
      pf_message_handler                ctrl_message_handlers[13] {
        &client_handler::SYN_handler,
        &client_handler::FIN_handler,
        &client_handler::LOGIN_OR_CREATE_USER_handler,
        &client_handler::SET_NICK_handler,
        &client_handler::LIST_SAVES_handler,
        &client_handler::LIST_GAMES_handler,
        &client_handler::CREATE_GAME_handler,
        &client_handler::LOAD_GAME_handler,
        &client_handler::SAVE_GAME_handler,
        &client_handler::JOIN_GAME_handler,
        &client_handler::LEAVE_GAME_handler,
        &client_handler::RESTART_GAME_handler,
        &client_handler::TERMINATE_GAME_handler,
      };

      mazed::settings_tuple             &settings_;         // Server daemon settings.

    public:
      client_handler(tcp::socket &sckt, asio::io_service &io_serv, mazed::settings_tuple &settings, unsigned conn_num);
     ~client_handler();
      void run();

    private:
      void run_processing();
      void terminate();

      // // // // // // // // // // //

      bool handshake_success();

      // // // // // // // // // // //

      void start_timeout();
      void check_timeout(const boost::system::error_code& error);
      inline void timeout_set();
      inline void timeout_stop();
      
      // // // // // // // // // // //

      void start_asio_loop();

      void asio_loop_receive();
      void asio_loop_receive_handler(const boost::system::error_code &error);

      void asio_loop_send();
      void asio_loop_send_handler(const boost::system::error_code &error);

      // // // // // // // // // // //

      void async_receive();
      void async_receive_handler(const boost::system::error_code &error);
      void async_send(protocol::message &msg);
      void empty_handler(const boost::system::error_code &error);

      // // // // // // // // // // //

      void SYN_handler();
      void FIN_handler();
      void LOGIN_OR_CREATE_USER_handler();
      void SET_NICK_handler();
      void LIST_SAVES_handler();
      void LIST_GAMES_handler();
      void CREATE_GAME_handler();
      void LOAD_GAME_handler();
      void SAVE_GAME_handler();
      void JOIN_GAME_handler();
      void LEAVE_GAME_handler();
      void RESTART_GAME_handler();
      void TERMINATE_GAME_handler();

      void error_message_handler();

      // // // // // // // // // // //

      inline void message_prepare(E_type type, E_ctrl_type ctrl_type, E_status status,
                                  std::vector<std::string> data = {""});
      inline void message_prepare(E_type type, E_info_type info_type, E_status status,
                                  std::vector<std::string> data = {""});
      inline void message_prepare(E_type type, E_error_type error_type, E_status status,
                                  std::vector<std::string> data = {""});

      inline std::string date_time_str();
      void log(mazed::log_level level, const char *str);
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_CL_HANDLER.HH ]********************************************************************************* *
 * ****************************************************************************************************************** */

#endif

