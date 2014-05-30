/**
 * @file      mazed_game_player.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.1
 * @brief     Contains derived class (specialization) of the basic_player.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_GAME_PLAYER.HH ]****************************************************************************** *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_MAZED_GAME_PLAYER_HH
#define H_GUARD_MAZED_GAME_PLAYER_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <fstream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "mazed_globals.hh"
#include "mazed_game_globals.hh"
#include "mazed_game_maze.hh"

#include "../protocol.hh"
#include "../basic_player.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ PLAYER CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace asio = boost::asio;
namespace ip = boost::asio::ip;
using     tcp = boost::asio::ip::tcp;

namespace mazed {
  class client_handler;
}

namespace game {

  class maze;

  /**
   * Derived from basic_player class for server-side purposes.
   */
  class player : public basic_player {
      boost::mutex                                  access_mutex_;

      asio::io_service                              io_service_;
      tcp::socket                                   socket_;
      tcp::acceptor                                 acceptor_;
      
      std::unique_ptr<protocol::tcp_serialization>  pu_tcp_connect_;
      bool                                          connected_ {false};

      std::unique_ptr<boost::thread>                pu_thread_;
      
      std::vector<protocol::update>                 updates_out_;
      std::vector<protocol::command>                commands_in_;
      std::vector<protocol::message>                messages_in_;

      protocol::E_user_command                      command_buffer_;
      game::E_move                                  direction_;
      game::E_move                                  next_move_ {NONE};
      protocol::E_move_result                       last_move_result_;
           
      std::string                                   UID_;
      std::string                                   auth_key_;

      std::pair<signed char, signed char>           start_coords_;                // Respawn coordinates.
      bool                                          invulnerability_ {true};
      bool                                          game_over_ {false};


      game::maze                                    *p_maze_ {NULL};
      mazed::client_handler                         *p_cl_handler_ {NULL};

      // Some stats.

      static unsigned long                          players_counter_;
      
      // // // // // // // // // // //

      void start_accept();
      void handle_accept(const boost::system::error_code &error);
      void handle_authentication(const boost::system::error_code &error);
      void async_receive();
      void async_receive_handler(const boost::system::error_code &error);
      void update_client_handler(const boost::system::error_code &error);

      inline void update_coords(game::E_move);
      inline protocol::E_move_result get_key();
      inline protocol::E_move_result open_gate();

    public:
      player(const std::string &puid, const std::string &auth_key, const std::string &nick,
             mazed::client_handler *p_client_handler);
     ~player();

      // // // // // // // // // // //

      unsigned short port();
      
      void set_maze(game::maze *maze_ptr);
      void set_start_coords(std::pair<signed char, signed char> coords);

      void run();
      void stop();

      std::pair<signed char, signed char> get_coords();

      bool update();
      bool kill();

      void game_finished();
      void update_client(std::vector<protocol::update> &updates);
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_GAME_PLAYER.HH ]******************************************************************************** *
 * ****************************************************************************************************************** */

#endif

