# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -std=c11 -g

# SDL2 library flags
LDFLAGS = -lSDL2 -lm -pthread

# Source file and executable name
SRC = Board.c Grid.c Community.c Helpers.c Action.c
OBJ = $(SRC:.c=.o)
EXEC = Board

# Default target
all: $(EXEC)

# Build the executable
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(EXEC) $(LDFLAGS)

# Rule to compile .c files into .o files
%.o: %.c config.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJ) $(EXEC)

# Run the program
run: $(EXEC)
	./$(EXEC)