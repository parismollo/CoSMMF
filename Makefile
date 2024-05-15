# no prints to the terminal
.SILENT: 

CC=gcc # compiler
CFLAGS=-I./include # tells compiler to include the include folder during header file lookups

# Name of the executable
EXEC=psar

# Source files
SRC=$(wildcard src/*.c app/*.c)
OBJS=$(SRC:.c=.o)

# build target
all: $(EXEC)

$(EXEC): $(OBJS)
	@echo "Building $@"
	@$(CC) $(CFLAGS) $^ -o $@
	@echo "Build complete"
# clean project set up
clean: 
	@echo "Cleaning up"
	@rm -f $(OBJS) $(EXEC)
	@rm -f files/*
	@rm -rf logs/*
	@rm -rf merge/*
	@echo "Clean complete"

# in case if files were named like all or clean.
.PHONY: all clean
