############################################################
# MAKEFILE for ICP course, 2014
############################################################
#	Author:		David Kaspar (aka Dee'Kej), 3BIT
#						BUT FIT, Czech Republic
#
# E-mails:	xkaspa34@stud.fit.vutbr.cz
#
# Date:			23-04-2014
############################################################

# Compiler.
CXX=colorgcc
LINKER=g++

#BOOST_PATH=../boost
LIBRARY_LINKAGE= -lboost_program_options -lboost_system -lboost_thread -lpthread -lboost_filesystem -lboost_serialization#-I $(BOOST_PATH)

# Parameters of compilation.
CXXFLAGS=-std=c++11 -pedantic -W -Wall -Wextra -g

# Default rule for creating all required files:
############################################################

all: mazed

mazed: build/mazed_main.o build/mazed_server.o build/mazed_server_connection.o build/mazed_cl_handler.o build/mazed_mazes_manager.o build/mazed_game_player.o build/mazed_game_instance.o
	$(LINKER) $(CXXFLAGS) $(LIBRARY_LINKAGE) -o $@ $^

build/mazed_main.o: mazed_main.cc mazed_globals.hh
	$(CXX) $(CXXFLAGS) -o $@ -c mazed_main.cc

build/mazed_server.o: mazed_server.cc mazed_server.hh mazed_globals.hh mazed_shared_resources.hh mazed_server_connection.hh
	$(CXX) $(CXXFLAGS) -o $@ -c mazed_server.cc

build/mazed_server_connection.o: mazed_server_connection.cc mazed_server_connection.hh mazed_globals.hh mazed_cl_handler.hh
	$(CXX) $(CXXFLAGS) -o $@ -c mazed_server_connection.cc

build/mazed_cl_handler.o: mazed_cl_handler.cc mazed_cl_handler.hh mazed_globals.hh mazed_shared_resources.hh mazed_game_maze.hh mazed_game_instance.hh mazed_game_player.hh ../serialization.hh ../protocol.hh
	$(CXX) $(CXXFLAGS) -o $@ -c mazed_cl_handler.cc

build/mazed_mazes_manager.o: mazed_mazes_manager.cc mazed_mazes_manager.hh mazed_globals.hh mazed_game_maze.hh mazed_game_guardian.hh
	$(CXX) $(CXXFLAGS) -o $@ -c mazed_mazes_manager.cc

build/mazed_game_player.o: mazed_game_player.cc mazed_game_player.hh mazed_game_globals.hh mazed_globals.hh mazed_cl_handler.hh
	$(CXX) $(CXXFLAGS) -o $@ -c mazed_game_player.cc

build/mazed_game_instance.o: mazed_game_instance.cc mazed_game_instance.hh mazed_game_globals.hh mazed_game_maze.hh mazed_game_player.hh mazed_game_guardian.hh mazed_game_block.hh mazed_globals.hh mazed_cl_handler.hh ../protocol.hh
	$(CXX) $(CXXFLAGS) -o $@ -c mazed_game_instance.cc

############################################################
# Other useful stuff:
############################################################

# Rule to mark "false-positive" targets in project folder.
.PHONY: run show kill clean clean-all

run: all kill
	@./mazed --logging 1
	@./.show.sh "./mazed"

show:
	@./.show.sh "./mazed"

kill:
	@./.mazed-kill.sh "./mazed"

# Remove object files generated during compiling.
clean:
	@echo "make[2]: Removing object files"
	@rm -rf build/*.o

clean-all: clean
	@echo "make[2]: Removing executable files"
	@rm -f mazed
