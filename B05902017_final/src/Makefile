TARGET=server client
INCLUDE=-I include
GCC=g++
FLAG=-std=c++11 -pthread
SERVER_SRC = server.cpp command_handler.cpp protocol.cpp
SERVER_OBJ = $(SERVER_SRC:.cpp=.o)

CLIENT_SRC = client.cpp protocol.cpp client_cmd_handler.cpp
CLIENT_OBJ = $(CLIENT_SRC:.cpp=.o)

.PHONY: clean

all:$(TARGET)

server: $(SERVER_OBJ)
	@echo "making: " $@
	@$(GCC) -o $@ $(FLAG) $(INCLUDE) $^

client: $(CLIENT_OBJ)
	@echo "making: " $@
	@$(GCC) -o $@ $(FLAG) $(INCLUDE) $^

%.o: %.cpp
	@echo "making: " $@
	@$(GCC) $< -c $(FLAG) $(INCLUDE)

clean:
	rm -rf *.o $(TARGET)

