/**
 * @file      protocol.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.9
 * @brief     File for implementation of the client-server communication protocol.
 *
 * @detailed  This file includes C++ structures for representing communication protocol messages. These structures are
 *            serialized with Boost's to make sure there's no problem with little/big endian representation.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF PROTOCOL.HH ]*************************************************************************************** *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_PROTOCOL_HH
#define H_GUARD_PROTOCOL_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <vector>
#include <string>

#include <boost/serialization/vector.hpp>   // For simpler includes.

#include "serialization.hh"                 // Including the serialization connection for simpler includes.


/* ****************************************************************************************************************** *
 ~ ~~~[ PROTOCOL'S STRUCTURES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace protocol {
  enum E_move_result {
    POSSIBLE,
    NOT_POSSIBLE,
  };
  
  /**
   *  Update message send to client's game instance: 
   */
  struct update {
    // long long           update_num;         // Sequence number - disabled for now.
    enum E_move_result  last_move;
    unsigned char       players_count;      // Actual players count.
    unsigned char       guardians_count;    // Actual number of guardians.

    std::vector<std::pair<unsigned char, unsigned char>>  coords;   // Coordinates of each character.

    template <typename Archive> void serialize(Archive &ar, const unsigned int version __attribute__((unused)))
    {
      // ar & update_num;
      ar & last_move;
      ar & players_count;
      ar & guardians_count;
      ar & coords;
    }
  };

  // // // // // // // // // // // // // // // //
  
  enum E_user_command {
    LEFT,
    RIGHT,
    UP,
    DOWN,
    TAKE_OPEN,
    START_CONTINUE,
    PAUSE,
  };
  
  // Command issued by the player:
  struct command {
    // unsigned char player_num;
    enum E_user_command cmd;

    template <typename Archive> void serialize(Archive &ar, const unsigned int version __attribute__((unused)))
    {
      // ar & player_num;
      ar & cmd;
    }
  };

  // // // // // // // // // // // // // // // //
  
  enum E_game_status {
    LOBBY,
    RUNNING,
    PAUSED,
    FINISHED,
  };

  /**
   *  Structure containing information about one specific game instance.
   */
  struct game_info {
    unsigned char       used_slots;
    enum E_game_status  status;
    std::string         UID;
    std::string         maze_name;
  
    std::vector<std::string> players;

    template <typename Archive> void serialize(Archive &ar, const unsigned int version __attribute__((unused)))
    {
      ar & used_slots;
      ar & status;
      ar & UID;
      ar & maze_name;
      ar & players;
    }
  };

  // // // // // // // // // // // // // // // //

  enum E_type {
    CTRL = 0,
    INFO,
    ERROR,
  };

  enum E_ctrl_type {
    SYN = 0,
    FIN,
    LOGIN_OR_CREATE_USER,
    SET_NICK,
    LIST_MAZES,
    LIST_RUNNING,
    LIST_SAVES,
    CREATE_GAME,
    LOAD_GAME,
    SAVE_GAME,
    JOIN_GAME,
    LEAVE_GAME,
    RESTART_GAME,
    TERMINATE_GAME,
  };

  #define E_CTRL_TYPE_SIZE 14U    // Used as a control mechanism against enum overflow. Always update!

  enum E_info_type {
    HELLO = 0,
    LOAD_DATA,
    GAMES_DATA,
    PLAYER_JOINED,
    PLAYER_LEFT,
    PLAYER_TIMEOUT,
    PLAYER_KILLED,
    PLAYER_GAME_OVER,
    PLAYER_WIN,
    GAME_RESTARTED,
    GAME_TERMINATED,
  };

  #define E_INFO_TYPE_SIZE 11U    // Used as a control mechanism against enum overflow. Always update!

  enum E_error_type {
    WRONG_PROTOCOL = 0,
    EMPTY_MESSAGE,
    MULTIPLE_MESSAGES,
    CONNECTION_FAILED,
    ALREADY_CONNECTED,
    REJECTED_CONNECTION,
    CLOSED_CONNECTION,
    TIMEOUT,
    HANDSHAKE,
    ALREADY_PLAYED,
    UNKNOWN_ERROR,
  };

  #define E_ERROR_TYPE_SIZE 11U   // Used as a control mechanism against enum overflow. Always update!

  enum E_status {
    ACK = 0,
    NACK,
    QUERY,
    UPDATE,
    SET,
    LOCAL,
  };

  /**
   *  Messages used between client & server over TCP connection.
   */
  struct message {
    enum E_type type;

    union {
      enum E_ctrl_type ctrl_type;
      enum E_info_type info_type;
      enum E_error_type error_type;
    };

    enum E_status status;

    std::vector<std::string> data;

    template <typename Archive> void serialize(Archive &ar, const unsigned int version __attribute__((unused)))
    {
      ar & type;
      
      switch (type) {
        case CTRL :
          ar & ctrl_type;
          break;
        
        case INFO :
          ar & info_type;
          break;

        case ERROR :
          ar & error_type;
          break;

        default :
          assert(false);    // You forgot to update serialization after updating the message!
          break;
      }

      ar & status;
      ar & data;
    }
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF PROTOCOL.HH ]***************************************************************************************** *
 * ****************************************************************************************************************** */

#endif

