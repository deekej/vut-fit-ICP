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

all: client

client: client_main.o client_mediator.o client_connections.o
	$(LINKER) $(CXXFLAGS) $(LIBRARY_LINKAGE) -o $@ $^

client_main.o: client_main.cc client_globals.hh client_mediator.hh
	$(CXX) $(CXXFLAGS) -o $@ -c client_main.cc

client_mediator.o: client_mediator.cc client_mediator.hh client_globals.hh client_connections.hh
	$(CXX) $(CXXFLAGS) -o $@ -c client_mediator.cc

client_connections.o: client_connections.cc client_connections.hh abc_connection.hh client_globals.hh ../protocol.hh ../serialization.hh
	$(CXX) $(CXXFLAGS) -o $@ -c client_connections.cc

############################################################
# Other useful stuff:
############################################################

# Rule to mark "false-positive" targets in project folder.
.PHONY: run clean clean-all

run:

# Remove object files generated during compiling.
clean:
	@echo "make[2]: Cleaning object files..."
	@rm -f *.o

clean-all: clean
	@echo "make[2]: Cleaning executable files..."
	@rm -f client
