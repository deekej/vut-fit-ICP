/**
 * @file      mazed_mazes_manager.cc
 * @author    Dee'Kej (David Kaspar - xkaspa34)
 * @version   0.2
 * @brief     Contains implementations of class member functions of mazed::mazes_manager.
 */


/* ****************************************************************************************************************** *
 * ***[ START OF MAZED_MAZES_MANAGER.CC ]**************************************************************************** *
 * ****************************************************************************************************************** */


/* ****************************************************************************************************************** *
 ~ ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

#include <tuple>


#include "mazed_mazes_manager.hh"


/* ****************************************************************************************************************** *
 ~ ~~~[ MEMBER FUNCTIONS IMPLEMENTATIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ~
 * ****************************************************************************************************************** */

namespace filesys = boost::filesystem;

namespace mazed {
  
  mazes_manager::mazes_manager(mazed::settings_tuple settings) :
    mazes_extension_{std::get<MAZES_EXTENSION>(settings)}, saves_extension_{std::get<SAVES_EXTENSION>(settings)},
    mazes_dir_path_(std::get<MAZES_FOLDER>(settings)), saves_dir_path_(std::get<SAVES_FOLDER>(settings)),
    daemon_dir_path_{std::get<DAEMON_FOLDER>(settings)}
  {{{
    return;
  }}}
  
  // // // // // // // // // // // //

  std::vector<std::string> mazes_manager::list_mazes()
  {{{
    try {
      filesys::current_path(daemon_dir_path_);
      return list_directory(mazes_dir_path_, mazes_extension_);
    }
    catch (filesys::filesystem_error) {
      return {};
    }
  }}}

  std::vector<std::string> mazes_manager::list_saves()
  {{{
    try {
      filesys::current_path(daemon_dir_path_);
      return list_directory(saves_dir_path_, saves_extension_);
    }
    catch (filesys::filesystem_error) {
      return {};
    }
  }}}

  // // // // // // // // // // // //

  std::vector<std::string> mazes_manager::list_directory(filesys::path dir_path, std::string extension)
  {{{
    std::vector<std::string> files;
    
    try {
      filesys::current_path(dir_path);
      
      filesys::directory_iterator it_dir_end;
      boost::system::error_code error;
      std::string fname;
      std::size_t fname_length;

      std::size_t ext_length {extension.length()};

      for (filesys::directory_iterator it_dir(filesys::current_path()); it_dir != it_dir_end; it_dir++) {

        if (filesys::is_regular_file(it_dir->path(), error) == true && !error) {
          fname = it_dir->path().filename().native();
          fname_length = fname.length();

          if (fname_length < ext_length) {
            continue;
          }
          
          if (ext_length == 0 || fname.find(extension, fname_length - ext_length) != std::string::npos) {
            files.push_back(fname);
          }
        }

      }

      return files;
    }
    catch (filesys::filesystem_error) {
      return {};
    }
  }}}
}

/* ****************************************************************************************************************** *
 * ***[ END OF MAZED_MAZES_MANAGER.CC ]****************************************************************************** *
 * ****************************************************************************************************************** */

