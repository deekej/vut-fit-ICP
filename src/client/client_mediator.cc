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

namespace client {
  mediator::mediator(client::settings_tuple &settings) :
    terminal_barrier_(2), connection_barrier_(2), settings_{settings} 
  {{{
    p_tcp_connect_ = new client::tcp_connection(settings_, message_in_, action_req_, action_req_mutex_,
                                                new_message_flag_, connection_barrier_);
    return;
  }}}

  mediator::~mediator()
  {{{
    delete p_tcp_connect_;
    return;
  }}}

  client::exit_codes mediator::run()
  {{{
    if (p_tcp_connect_->connect() == false) {
      return client::exit_codes::E_CONNECT_FAILED;
    }
    
    boost::asio::io_service io_service;
    boost::asio::deadline_timer timer(io_service);

    boost::unique_lock<boost::mutex> action_lock(action_req_mutex_);
    connection_barrier_.wait();

    for (unsigned i = 0; i < 6; i++) {
      std::cout << "Going to sleep, iteration #" << i << std::endl;
      timer.expires_from_now(boost::posix_time::seconds(5));
      timer.wait();
    }

    p_tcp_connect_->disconnect();

    return client::exit_codes::NO_ERROR;
  }}}
}

/* ****************************************************************************************************************** *
 * ***[ END OF CLIENT_MEDIATOR.CC ]********************************************************************************** *
 * ****************************************************************************************************************** */

