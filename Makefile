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

############################################################
# Default rule for creating all required files:
############################################################

all:
	@$(MAKE) -C src/server

############################################################
# Other useful stuff:
############################################################

# Rule to mark "false-positive" targets in project folder.
.PHONY: run doxygen pack clean clean-all

run:

doxygen:

pack:

# Remove object files generated during compiling.
clean:
	@cd server && $(MAKE) clean

clean-all:
	@cd server && $(MAKE) clean-all
