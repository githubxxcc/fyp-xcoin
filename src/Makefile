CC=g++
CFLAGS=-Wall
LIBS=-lboost_system -lboost_thread
INCLUDE=/usr/include/

DEPS=main.h
ODIR=obj
_OBJ = main.o
OBJ=$(patsubst %, $(ODIR)/%,$(_OBJ))


$(ODIR)/%.o : %.cpp $(DEPS)
	$(CC) -std=c++11 -g -c -o $@ $< $(CFLAGS)

all: $(OBJ) 
	$(CC) -std=c++11 -g -o xcoin $^ $(CFLAGS) -I $(INCLUDE) $(LIBS)

.PHONY: test
test: 
	./xcoin 

.PHONY: clean
clean: 
	rm obj/*
