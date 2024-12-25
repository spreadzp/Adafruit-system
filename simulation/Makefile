CXX = g++
CXXFLAGS = -std=c++11 -I. -pthread -DARDUINO=100 -include mock_libraries.h

SRCS = Arduino.cpp main_sim.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = simulator

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(CXXFLAGS)

%.o: %.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)
