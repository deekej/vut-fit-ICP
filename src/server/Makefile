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

mazed: mazed_main.o mazed_server.o mazed_cl_handler.o
	$(LINKER) $(CXXFLAGS) $(LIBRARY_LINKAGE) -o $@ $^

mazed_main.o: mazed_main.cc
	$(CXX) $(CXXFLAGS) -o $@ -c $^

mazed_server.o: mazed_server.cc mazed_server.hh mazed_cl_handler.hh ../connection.hh ../protocol.hh
	$(CXX) $(CXXFLAGS) -o $@ -c mazed_server.cc

mazed_cl_handler.o: mazed_cl_handler.cc mazed_cl_handler.hh ../connection.hh ../protocol.hh
	$(CXX) $(CXXFLAGS) -o $@ -c mazed_cl_handler.cc
	
client: client.cc
	g++ $(CXXFLAGS) -o $@ client.cc $(LIBRARY_LINKAGE)

############################################################
# Other useful stuff:
############################################################

# Rule to mark "false-positive" targets in project folder.
.PHONY: run show kill clean clean-all

run: all kill
	@./mazed --logging 1 -t 4000
	@./show.sh

show:
	@./show.sh

kill:
	@./mazed-kill.sh

# Remove object files generated during compiling.
clean:
	@echo "make[2]: Cleaning object files..."
	@rm -f *.o

clean-all: clean
	@echo "make[2]: Cleaning executable files..."
	@rm -f mazed
