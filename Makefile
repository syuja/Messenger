#Makefile
CC=g++
#-Wall -D_GNU_SOURCE
CFLAGS= -std=c++11 -g -lpthread
COMP=-c
SRC=src/
INC=incl/
OBJ=obj/


.Phony: all clean server client

all: server.x client.x

server: server.x

client: client.x

server.x: $(OBJ)mainS.o $(OBJ)util.o $(OBJ)server.o
	$(CC) $(CFLAGS) -I$(INC) $(OBJ)mainS.o $(OBJ)util.o $(OBJ)server.o -o server.x

client.x: $(OBJ)mainC.o $(OBJ)util.o $(OBJ)client.o
	$(CC) $(CFLAGS) -I$(INC) $(OBJ)mainC.o $(OBJ)util.o $(OBJ)client.o -o client.x

$(OBJ)mainS.o: $(SRC)mainServ.cpp
			$(CC) $(CFLAGS) -I$(INC) $(COMP) $(SRC)mainServ.cpp -o $(OBJ)mainS.o

$(OBJ)mainC.o: $(SRC)mainCli.cpp
			$(CC) $(CFLAGS) -I$(INC) $(COMP) $(SRC)mainCli.cpp -o $(OBJ)mainC.o

$(OBJ)server.o: $(SRC)Server.cpp $(INC)Server.h
	$(CC) $(CFLAGS) -I$(INC) $(COMP) $(SRC)Server.cpp -o $(OBJ)server.o

$(OBJ)client.o: $(SRC)Client.cpp $(INC)Client.h
	$(CC) $(CFLAGS) -I$(INC) $(COMP) $(SRC)Client.cpp -o $(OBJ)client.o

$(OBJ)util.o:	$(SRC)Util.cpp $(INC)Util.h
	$(CC) $(CFLAGS) -I$(INC) $(COMP) $(SRC)Util.cpp -o $(OBJ)util.o


clean:
	rm $(OBJ)*.o  *.x *.out $(INCL)*gch
