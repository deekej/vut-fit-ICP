
#ifndef H_GUARD_PROTOCOL_HH
#define H_GUARD_PROTOCOL_HH

#include <vector>
#include <string>

#include "connection.hh"

namespace protocol {
  enum E_move_result {
    POSSIBLE,
    NOT_POSSIBLE,
  };

  struct update {
    long long           update_num;
    enum E_move_result  last_move;
    unsigned char       players_count;
    unsigned char       guardians_count;

    std::vector<std::pair<unsigned char, unsigned char>>  coords;

    template <typename Archive> void serialize(Archive &ar, const unsigned int version __attribute__((unused)))
    {
      ar & update_num;
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

  struct command {
    unsigned char player_num;
    enum E_user_command cmd;

    template <typename Archive> void serialize(Archive &ar, const unsigned int version __attribute__((unused)))
    {
      ar & player_num;
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

  struct game_info {
    unsigned char used_slots;
    enum E_game_status status;
    std::string UID;
    std::string maze_name;
  
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
    CTRL,
    INFO,
    ERROR,
  };

  enum E_ctrl_type {
    SYN,
    FIN,
    LOGIN_OR_CREATE_USER,
    SET_NICK,
    LIST_SAVES,
    LIST_GAMES,
    CREATE_GAME,
    LOAD_GAME,
    SAVE_GAME,
    JOIN_GAME,
    LEAVE_GAME,
    RESTART_GAME,
    TERMINATE_GAME,
  };

  #define E_CTRL_TYPE_SIZE 13U

  enum E_info_type {
    HELLO,
    LOAD_DATA,
    GAMES_DATA,
    PLAYER_JOINED,
    PLAYER_LEFT,
    PLAYER_KILLED,
    PLAYER_GAME_OVER,
    PLAYER_WIN,
    GAME_RESTARTED,
    GAME_TERMINATED,
  };

  enum E_error_type {
    WRONG_PROTOCOL,
    EMPTY_MESSAGE,
    MULTIPLE_MESSAGES,
    TIMEOUT,
    ALREADY_PLAYED,
    UNKNOWN_ERROR,
  };

  enum E_status {
    ACK,
    NACK,
    QUERY,
    UPDATE,
    SET,
  };

  struct message {
    enum E_type type;

    union {
      enum E_ctrl_type ctrl_type;
      enum E_info_type info_type;
      enum E_error_type error_type;
    };

    enum E_status status;

    std::string data;

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

#endif
