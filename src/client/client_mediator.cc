/**
 * @file      client_mediator.cc
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.1
 * @brief     Contains implementations of client::mediator member functions.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF CLIENT_MEDIATOR.CC ]******************************************************************************** *
 * ****************************************************************************************************************** */


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include "client_mediator.hh"

/* ****************************************************************************************************************** *
 ~ ~~~[ MEMBER FUNCTIONS IMPLEMENTATIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

using command = ABC::user_interface::E_user_command;

namespace client {
  mediator::mediator(client::settings_tuple &settings) :
    interface_barrier_(2), connection_barrier_(2), settings_{settings} 
  {{{
    p_tcp_connect_ = new client::tcp_connection(settings_, message_in_, action_req_, action_req_mutex_,
                                                new_message_flag_, connection_barrier_);

    p_interface_ = new client::terminal_interface(action_req_, action_req_mutex_, interface_barrier_, user_command_,
                                                  additional_data_);
    return;
  }}}

  mediator::~mediator()
  {{{
    delete p_tcp_connect_;
    delete p_interface_;
    return;
  }}}

  client::exit_codes mediator::run()
  {{{
    boost::unique_lock<boost::mutex> action_lock(action_req_mutex_);
    p_interface_->initialize();
    interface_barrier_.wait();

    if (p_tcp_connect_->connect() == false) {
      return client::exit_codes::E_CONNECT_FAILED;
    }

    connection_barrier_.wait();
#if 0   
    boost::asio::io_service io_service;
    boost::asio::deadline_timer timer(io_service);

    for (unsigned i = 0; i < 6; i++) {
      std::cout << "Going to sleep, iteration #" << i << std::endl;
      timer.expires_from_now(boost::posix_time::seconds(5));
      timer.wait();
    }
#endif

    while (run_ == true) {
      action_req_.wait(action_lock);

      if (user_command_ != command::NONE) {
        if (user_command_ == command::EXIT) {
          run_ = false;
          p_interface_->display_message("exiting");
        }
        else {
          p_interface_->display_message("user input");
        }
      }

      if (new_message_flag_ == true) {
        p_interface_->display_message("new message");
        std::cout << message_in_.type << message_in_.ctrl_type << message_in_.status << message_in_.data[0] << std::endl;
      }
    }

    p_tcp_connect_->disconnect();
    std::cerr << "disconnect passed" << std::endl;
    p_interface_->terminate();
    std::cerr << "terminate passed" << std::endl;

    return client::exit_codes::NO_ERROR;
  }}}
}

/* ****************************************************************************************************************** *
 * ***[ END OF CLIENT_MEDIATOR.CC ]********************************************************************************** *
 * ****************************************************************************************************************** */

