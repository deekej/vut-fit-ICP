/**
 * @file      mazed_game_player.cc
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.1
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_GAME_PLAYER.CC ]****************************************************************************** *
 * ****************************************************************************************************************** */


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include "mazed_cl_handler.hh"
#include "mazed_game_player.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ STATIC MEMBERS DEFINITIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

using namespace protocol;

namespace game {
  unsigned long player::players_counter_ = 1;


/* ****************************************************************************************************************** *
 ~ ~~~[ MEMBER FUNCTION IMPLEMENTATIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

  player::player(const std::string &puid, const std::string &auth_key, game::maze *p_maze,
                 mazed::client_handler *p_client_handler) :
    basic_player(),
    socket_(io_service_),
    acceptor_(io_service_, tcp::endpoint(tcp::v4(), static_cast<unsigned short>(0))),
    UID_{puid}, auth_key_{auth_key}, p_maze_{p_maze}, p_cl_handler_{p_client_handler}
  {{{
    nick_ = "player-" + std::to_string(players_counter_++);
    pu_tcp_connect_ = std::unique_ptr<protocol::tcp_serialization>(new protocol::tcp_serialization(socket_));
    return;
  }}}


  player::player(const std::string &puid, const std::string &auth_key, const std::string &nick, game::maze *p_maze,
                 mazed::client_handler *p_client_handler) :
    basic_player(nick),
    socket_(io_service_),
    acceptor_(io_service_, tcp::endpoint(tcp::v4(), static_cast<unsigned short>(0))),
    UID_{puid}, auth_key_{auth_key}, p_maze_{p_maze}, p_cl_handler_{p_client_handler}
  {{{
    pu_tcp_connect_ = std::unique_ptr<protocol::tcp_serialization>(new protocol::tcp_serialization(socket_));
    return;
  }}}


  player::~player()
  {{{
    return;
  }}}

  // // // // // // // // // // //

  unsigned short player::port()
  {{{
    return acceptor_.local_endpoint().port();
  }}}


  void player::set_start_coords(signed char row, signed char col)
  {{{
    start_coords_.first = row;
    start_coords_.second = col;
    return;
  }}}

  // // // // // // // // // // //

  void player::run()
  {{{
    pu_thread_ = std::unique_ptr<boost::thread>(new boost::thread(&player::start_accept, this));
    return;
  }}}

  
  void player::stop()
  {{{
    if (socket_.is_open() == true) {
      boost::system::error_code ignored_error;
      socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_error);
      socket_.cancel();
      socket_.close();
    }

    io_service_.stop();

    if (pu_thread_ && (*pu_thread_).joinable() == true) {
      (*pu_thread_).join();
    }

    io_service_.reset();

    return;
  }}}

  // // // // // // // // // // //

  void player::start_accept()
  {{{
    acceptor_.async_accept(socket_, boost::bind(&player::handle_accept, this, _1));
    io_service_.run();
    return;
  }}}


  void player::handle_accept(const boost::system::error_code &error)
  {{{
    acceptor_.close();

    if (error) {
      p_cl_handler_->log(mazed::log_level::ERROR, error.message().c_str());
      acceptor_.async_accept(socket_, boost::bind(&player::handle_accept, this, _1));
      return;
    }
    
    pu_tcp_connect_->async_read(messages_in_, boost::bind(&player::handle_authentication, this,
                                                          boost::asio::placeholders::error));
    return;
  }}}


  void player::handle_authentication(const boost::system::error_code &error)
  {{{
    if (error) {
      p_cl_handler_->log(mazed::log_level::ERROR, error.message().c_str());
      acceptor_.async_accept(socket_, boost::bind(&player::handle_accept, this, _1));
      return;
    }

    if (messages_in_.size() != 1 || messages_in_[0].type != protocol::E_type::CTRL ||
        messages_in_[0].ctrl_type != protocol::E_ctrl_type::SYN ||
        messages_in_[0].status != protocol::E_status::UPDATE || messages_in_[0].data[0] != auth_key_) {

      p_cl_handler_->log(mazed::log_level::INFO, "Client's authentication failed");

      if (socket_.is_open() == true) {
        boost::system::error_code ignored_error;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_error);
        socket_.cancel();
        socket_.close();
      }

      acceptor_.async_accept(socket_, boost::bind(&player::handle_accept, this, _1));
      return;
    }
    
    async_receive();
    return;
  }}}

  
  void player::async_receive()
  {{{
    pu_tcp_connect_->async_read(commands_in_, boost::bind(&player::async_receive_handler, this,
                                                          boost::asio::placeholders::error));
    return;
  }}}


  void player::async_receive_handler(const boost::system::error_code &error)
  {{{
    if (error) {
      switch (error.value()) {
        case boost::asio::error::eof :
          p_cl_handler_->log(mazed::log_level::INFO, "Game's connection has been closed by client");
          return;

        case boost::asio::error::operation_aborted :
          p_cl_handler_->log(mazed::log_level::INFO, "Game's connection has timed out");
          return;

        default :
          p_cl_handler_->log(mazed::log_level::ERROR, error.message().c_str());
          return;
      }
    }

    access_mutex_.lock();
    {
      if (command_buffer_ != protocol::E_user_command::NONE || finished_ == true) {
        access_mutex_.unlock();
        return;
      }

      command_buffer_ = commands_in_[0].cmd;
    }
    access_mutex_.unlock();

    async_receive();
    return;
  }}}

  // // // // // // // // // // //
  
  inline void player::update_coords(game::E_move move)
  {{{
    p_maze_->matrix_[coords_.first][coords_.second].remove_player(this);

    switch (move) {
      case LEFT :
        coords_.second = (coords_.second - 1) % p_maze_->dimensions_.second;
        break;
      
      case RIGHT :
        coords_.second = (coords_.second + 1) % p_maze_->dimensions_.second;
        break;

      case UP :
        coords_.first = (coords_.first - 1) % p_maze_->dimensions_.first;
        break;

      case DOWN :
        coords_.first = (coords_.first + 1) % p_maze_->dimensions_.first;
        break;
        
      default :
        break;
    }
    
    p_maze_->matrix_[coords_.first][coords_.second].add_player(this);

    return;
  }}}


  inline protocol::E_move_result player::get_key()
  {{{
    std::pair<signed char, signed char> key_coords = coords_;

    switch (direction_) {
      case LEFT :
        key_coords.second--;
        break;
        
      case RIGHT :
        key_coords.second++;
        break;
      
      case UP :
        key_coords.first--;
        break;

      case DOWN :
        key_coords.first++;
      
      default :
        return NOT_POSSIBLE;
    }
    
    key_coords.first %= p_maze_->dimensions_.first;
    key_coords.second %= p_maze_->dimensions_.second;
    
    if (p_maze_->matrix_[key_coords.first][key_coords.second].get() == game::block::KEY) {
      p_maze_->matrix_[key_coords.first][key_coords.second].set(game::block::EMPTY);
      has_key_ = true;
      return POSSIBLE;
    }
    else if (p_maze_->matrix_[key_coords.first][key_coords.second].get() == game::block::GATE_DROPPED_KEY) {
      p_maze_->matrix_[key_coords.first][key_coords.second].set(game::block::GATE_OPEN);
      has_key_ = true;
      return POSSIBLE;
    }

    return NOT_POSSIBLE;
  }}}


  inline protocol::E_move_result player::open_gate()
  {{{
    std::pair<signed char, signed char> gate_coords = coords_;

    switch (direction_) {
      case LEFT :
        gate_coords.second--;
        break;
        
      case RIGHT :
        gate_coords.second++;
        break;
      
      case UP :
        gate_coords.first--;
        break;

      case DOWN :
        gate_coords.first++;
      
      default :
        return NOT_POSSIBLE;
    }
    
    gate_coords.first %= p_maze_->dimensions_.first;
    gate_coords.second %= p_maze_->dimensions_.second;
    
    if (p_maze_->matrix_[gate_coords.first][gate_coords.second].get() == game::block::GATE_CLOSED) {
      p_maze_->matrix_[gate_coords.first][gate_coords.second].set(game::block::GATE_OPEN);
      has_key_ = false;
      return POSSIBLE;
    }

    return NOT_POSSIBLE;
  }}}

  // // // // // // // // // // //

  bool player::update()
  {{{
    protocol::E_user_command command_act;

    access_mutex_.lock();
    {
      command_act = command_buffer_;
      command_buffer_ = protocol::E_user_command::NONE;

      // if (invulnerability_ > 0) {
      //   invulnerability_--;
      // }
    }
    access_mutex_.unlock();

    last_move_result_ = NOT_POSSIBLE;

    p_maze_->access_mutex_.lock();
    switch (command_act) {
      case LEFT :
        if (p_maze_->is_move_possible(coords_, LEFT) == true) {
          update_coords(LEFT);
          last_move_result_ = POSSIBLE;
          next_move_ = LEFT;
        }
        else if (direction_ != LEFT) {
          last_move_result_ = POSSIBLE;
        }

        direction_ = LEFT;
        break;

      case RIGHT:
        if (p_maze_->is_move_possible(coords_, RIGHT) == true) {
          update_coords(RIGHT);
          last_move_result_ = POSSIBLE;
          next_move_ = RIGHT;
        }
        else if (direction_ != RIGHT) {
          last_move_result_ = POSSIBLE;
        }

        direction_ = RIGHT;
        break;
        
      case UP :
        if (p_maze_->is_move_possible(coords_, UP) == true) {
          update_coords(UP);
          last_move_result_ = POSSIBLE;
          next_move_ = UP;
        }
        else if (direction_ != UP) {
          last_move_result_ = POSSIBLE;
        }

        direction_ = UP;
        break;
        
      case DOWN :
        if (p_maze_->is_move_possible(coords_, DOWN) == true) {
          update_coords(DOWN);
          last_move_result_ = POSSIBLE;
          next_move_ = DOWN;
        }
        else if (direction_ != DOWN) {
          last_move_result_ = POSSIBLE;
        }
        
        direction_ = UP;
        break;

      case STOP :
        last_move_result_ = POSSIBLE;
        next_move_ = STOP;
        break;
        
      case TAKE_OPEN :
        if (has_key_ == false) {
          last_move_result_ = get_key();
        }
        else {
          last_move_result_ = open_gate();
        }
        break;

      case START_CONTINUE :
      case PAUSE :
        break;
      
      // Moving in given direction again until solid object is found:
      default :
        if (p_maze_->is_move_possible(coords_, next_move_) == true) {
          update_coords(next_move_);
          last_move_result_ = POSSIBLE;
        }
        break;
    }

    if (p_maze_->matrix_[coords_.first][coords_.second].get() == game::block::TARGET) {
      p_maze_->access_mutex_.unlock();
      return true;
    }
    else {
      p_maze_->access_mutex_.unlock();
      return false;
    }

  }}}


  bool player::kill()
  {{{
    if (has_key_ == true) {
      p_maze_->access_mutex_.lock();
      {
        if (p_maze_->matrix_[coords_.first][coords_.second].get() == game::block::GATE_OPEN) {
          p_maze_->matrix_[coords_.first][coords_.second].set(game::block::GATE_DROPPED_KEY);
        }
        else {
          p_maze_->matrix_[coords_.first][coords_.second].set(game::block::KEY);
        }
      }
      p_maze_->access_mutex_.unlock();

      has_key_ = false;
    }

    access_mutex_.lock();
    {
      p_maze_->matrix_[coords_.first][coords_.second].remove_player(this);

      lifes_--;
      coords_ = start_coords_;

      p_maze_->matrix_[coords_.first][coords_.second].add_player(this);
    }
    access_mutex_.lock();

    return (lifes_ == 0) ? true : false;
  }}}
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_GAME_PLAYER.CC ]******************************************************************************** *
 * ****************************************************************************************************************** */

