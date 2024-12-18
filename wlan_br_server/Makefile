
BIN=wlan_br_server

SRC=$(wildcard *.c)
INC=-I.
INC += -I../libcli
LIB_DIR=-L.
LIB_DIR += -L../libcli

#LIB=-lsctp
LIB=-pthread -lcli
OBJ=$(patsubst %.c,%.o,$(SRC))
CFLAGS=-O0 -g
%.o: %.c
	gcc -Wall -o $@ -c $^ $(INC) $(CFLAGS)

$(BIN):$(OBJ)
	gcc -o $@ $^ $(LIB_DIR) $(LIB)  $(CFLAGS) 

clean:
	rm -rf $(BIN) $(OBJ)
all:$(BIN) 

.PHYON:$(BIN) clean all

#gcc -o $@ $^ -L$(LIB_DIR) -ldouble_list -static $(CFLAGS) 
