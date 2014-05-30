/**
 * @file      client_game_instance.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.1
 * @brief     Contains member function implementations of client::game_instance class.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF CLIENT_GAME_INSTANCE.CC ]*************************************************************************** *
 * ****************************************************************************************************************** */


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include "client_game_instance.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ GAME_INSTANCE CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace client {
  game_instance::game_instance(const std::string IP_address, const std::string port, const std::string auth_key,
                               const std::string maze_scheme, const std::string maze_rows, const std::string maze_cols,
                               boost::condition_variable &mediator_cv, boost::mutex &mediator_mutex,
                               protocol::message &mediator_message_in, bool &mediator_message_flag) :
    maze_scheme_{maze_scheme}
  {{{
    maze_rows_ = static_cast<signed char>(std::stoi(maze_rows));
    maze_cols_ = static_cast<signed char>(std::stoi(maze_cols));
    output_string_ = maze_scheme_;

    p_game_conn_ = new client::game_connection(IP_address, port, auth_key, update_in_, update_in_new_, update_in_mutex_,
                                               mediator_cv, mediator_mutex, mediator_message_in, mediator_message_flag);
    return;
  }}}


  game_instance::~game_instance()
  {{{
    delete p_game_conn_;
    return;
  }}}


  bool game_instance::run()
  {{{
    if (p_game_conn_->connect() == false) {
      return false;
    }

    pu_thread_ = std::unique_ptr<boost::thread>(new boost::thread(&game_instance::process_updates, this));

    return true;
  }}}

  
  void game_instance::stop()
  {{{
    run_mutex_.lock();
    {
      run_ = false;
      update_in_new_.notify_one();
    }
    run_mutex_.unlock();

    if (pu_thread_ && (*pu_thread_).joinable() == true) {
      (*pu_thread_).join();
      pu_thread_.reset();
    }

    p_game_conn_->disconnect();

    return;
  }}}


  void game_instance::send_command(const protocol::command &cmd)
  {{{
    p_game_conn_->async_send(cmd);
    return;
  }}}


  void game_instance::process_updates()
  {{{
    boost::unique_lock<boost::mutex> update_in_lock(update_in_mutex_);
    
    run_mutex_.lock();
    while (run_ == true) {
      run_mutex_.unlock();

      update_output_string();
      update_in_new_.wait(update_in_lock);

      run_mutex_.lock();
    }
    run_mutex_.unlock();


    return;
  }}}


  inline void game_instance::update_output_string()
  {{{
    long linear_pos;
    std::size_t player_num = 0;

    output_string_ = maze_scheme_;

    for (auto coords : update_in_.opened_gates_coords) {
      output_string_[(coords.first * maze_cols_ * 2) + (coords.second * 2)] = ' ';
    }

    for (auto coords : update_in_.keys_coords) {
      output_string_[(coords.first * maze_cols_ * 2) + (coords.second * 2)] = '*';
    }

    for (auto coords : update_in_.players_coords) {
      linear_pos = (coords.first * maze_cols_ * 2) + (coords.second * 2);
      player_num++;
      
      if (linear_pos < 0) {
        continue;
      }

      output_string_[linear_pos] = ('0' + player_num);
    }

    for (auto coords : update_in_.guardians_coords) {
      output_string_[(coords.first * maze_cols_ * 2) + (coords.second * 2)] = '@';
    }

    return;
  }}}

  // // // // // // // // // // // //
  
  std::string game_instance::get_rows()
  {{{
    return std::to_string(maze_rows_);
  }}}
  

  std::string game_instance::get_cols()
  {{{
    return std::to_string(maze_cols_ * 2);
  }}}

  
  std::string game_instance::get_output_string()
  {{{
    std::string return_string;

    update_in_mutex_.lock();
    {
      return_string = output_string_;
    }
    update_in_mutex_.unlock();

    return return_string;
  }}}
}

/* ****************************************************************************************************************** *
 * ***[ END OF CLIENT_GAME_INSTANCE.CC ]***************************************************************************** *
 * ****************************************************************************************************************** */

