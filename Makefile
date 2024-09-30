CXX = g++
CXXFLAGS = -std=c++11 -Wall -pthread

SRC = src/main.cpp src/Server.cpp src/ThreadPool.cpp
OBJ = $(SRC:.cpp=.o)
INCLUDE = -Iinclude

TARGET = http_server

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ) -lpthread

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
