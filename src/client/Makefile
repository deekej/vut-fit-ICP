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

# Parameters of compilation.
CXXFLAGS=-std=c++11 -pedantic -W -Wall -Wextra -g

#BOOST_PATH=../boost
LIBRARY_LINKAGE= -lboost_program_options -lboost_system -lboost_thread -lpthread -lboost_filesystem -lboost_serialization#-I $(BOOST_PATH)


############################################################
# Default rule for creating all required files:
############################################################

all: client

client: build/client_main.o build/client_mediator.o build/client_connections.o build/client_terminal_UI.o
	$(LINKER) $(CXXFLAGS) $(LIBRARY_LINKAGE) -o $@ $^

build/client_main.o: client_main.cc client_globals.hh client_mediator.hh
	$(CXX) $(CXXFLAGS) -o $@ -c client_main.cc

build/client_mediator.o: client_mediator.cc client_mediator.hh client_globals.hh client_connections.hh
	$(CXX) $(CXXFLAGS) -o $@ -c client_mediator.cc

build/client_connections.o: client_connections.cc client_connections.hh abc_connection.hh client_globals.hh ../protocol.hh ../serialization.hh
	$(CXX) $(CXXFLAGS) -o $@ -c client_connections.cc

build/client_terminal_UI.o:	client_interface_terminal.cc client_interface_terminal.hh abc_user_interface.hh
	$(CXX) $(CXXFLAGS) -o $@ -c client_interface_terminal.cc

############################################################
# Other useful stuff:
############################################################

# Rule to mark "false-positive" targets in project folder.
.PHONY: run clean clean-all

run: all
	@./client

# Remove object files generated during compiling.
clean:
	@echo "make[2]: Removing object files"
	@rm -rf build/*

clean-all: clean
	@echo "make[2]: Removing executable files"
	@rm -f client
