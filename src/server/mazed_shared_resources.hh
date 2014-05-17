/**
 * @file      mazed_shared_resources.hh
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.1
 * @brief     Container with all the shared resources between threads.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_SHARED_RESOURCES.HH ]************************************************************************* *
 * ****************************************************************************************************************** */

#ifndef H_GUARD_MAZED_SHARED_RESOURCES_HH
#define H_GUARD_MAZED_SHARED_RESOURCES_HH


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <memory>

#include "mazed_globals.hh"
#include "mazed_mazes_manager.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ SHARED_RESOURCES CLASS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace mazed {
  
  /**
   * Encapsulates all the necessary access prerequisites for all the shared resources. It doesn't provide any access
   * control as itself. It's just like a storage container. We're expecting that every thread behaves properly and
   * clearly when accessing the contents. In other words - every thread should adequately lock the corresponding mutex
   * when accessing or making any changes and unlock it ASAP when it doesn't need the resources anymore.
   */
  class shared_resources {
    public:
      std::unique_ptr<mazed::mazes_manager>       p_mazes_manager;
      
      // // // // // // // // // // //

      shared_resources(mazed::settings_tuple settings)
      {{{
        p_mazes_manager = std::unique_ptr<mazed::mazes_manager>(new mazed::mazes_manager(settings));

        return;
      }}}
  };
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_MAZES_MANAGER.HH ]****************************************************************************** *
 * ****************************************************************************************************************** */

#endif

