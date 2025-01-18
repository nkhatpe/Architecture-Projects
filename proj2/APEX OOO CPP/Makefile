CC = g++
CFLAGS = -g -Wall
INCLUDES = -I./include/headers

SRCS = src/main.cpp src/apex_cpu.cpp src/rob.cpp src/register_manager.cpp \
       src/control_predictor.cpp src/lsq.cpp src/memory_fu.cpp src/int_fu.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = apex_sim

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)