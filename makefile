CC = g++
CFLAGS = -Wall -Wextra -std=c++11

SRCS = src/main.cpp \
       src/helpers.cpp \
       src/prompt.cpp \
       src/builtins.cpp \
       src/pinfo.cpp \
       src/history.cpp \
       src/execute.cpp \
       src/readline_custom.cpp

OBJS = $(SRCS:.cpp=.o)
TARGET = shell

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)