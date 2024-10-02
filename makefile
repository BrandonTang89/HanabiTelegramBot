# Define the compiler
CC = g++

# Define the target executable
TARGET = server

# Define the source files
SRCS = server.cpp sockets.cpp player.cpp session.cpp game.cpp deck.cpp card.cpp

# Define the object files
OBJS = $(SRCS:.cpp=.o)

# Define the compiler flags
CFLAGS = -DBOOST_LOG_DYN_LINK -Wunused -Wall -Wextra -pedantic -O0 -std=c++23 -fsanitize=address

# Define the linker flags
LDFLAGS = -lboost_system -lboost_log -lboost_thread -lpthread

# Default target
all: $(TARGET)

# Rule to build the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Rule to build the object files
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)