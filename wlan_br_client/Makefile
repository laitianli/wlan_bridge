PWD=$(shell pwd)
BIN_DIR=$(PWD)/bin/
BIN=wlan_br_client

OBJ_DIR=$(PWD)/obj
SRC=$(wildcard *.c)
INC=-I.
INC += -I../src_lib/libcli
INC += -I../src_lib/uthash

LIB_DIR=-L.
LIB_DIR += -L../src_lib/libcli

LIB=-pthread -lcli
OBJ=$(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC))
CFLAGS=-O2 -g

define build_dir
	if [ ! -z "$1" ] && [ ! -d $1 ];then mkdir -p $1; fi
endef

$(OBJ_DIR)/%.o: %.c
	@$(call build_dir,$(OBJ_DIR))
	gcc -Wall -o $@ -c $^ $(INC) $(CFLAGS)

$(BIN_DIR)/$(BIN):$(OBJ)
	@$(call build_dir,$(BIN_DIR))
	gcc -o $@ $^ $(LIB_DIR) $(LIB)  $(CFLAGS) 

clean:
	rm -rf $(BIN) $(OBJ) $(OBJ_DIR) $(BIN_DIR)
all:$(BIN) 

.PHYON:$(BIN) clean all

