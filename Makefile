# suppresses command echo - no prints to the terminal
.SILENT: 

CC=gcc # compiler
CFLAGS=-I./include # tells compiler to include the include folder during header file lookups

# Name of the executable
EXEC=psar

# Source files - find all .c files
SRC=$(wildcard src/*.c app/*.c)
OBJS=$(SRC:.c=.o)

# Default build target
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
# avoid confusion if files were named like this
.PHONY: all clean
