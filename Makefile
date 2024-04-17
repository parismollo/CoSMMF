.SILENT:

CC=gcc 
CFLAGS=-I./include

# Name of the executable
EXEC=psar

# Source files
SRC=$(wildcard src/*.c app/*.c)
OBJS=$(SRC:.c=.o)

# Default build target
all: $(EXEC)

$(EXEC): $(OBJS)
	@echo "Building $@"
	@$(CC) $(CFLAGS) $^ -o $@
	@echo "Build complete"

clean:
	@echo "Cleaning up"
	@rm -f $(OBJS) $(EXEC)
	@rm -f files/*
	@rm -rf logs/*
	@rm -rf merge/*
	@echo "Clean complete"

.PHONY: all clean
