CC = gcc
CFLAG = -Wall -Wextra -Wpedantic -std=c17 -g
DIR = src
OBJ = dict.o
LIB = 
POST_FIX = 
ELF_FILES = 

ifeq ($(OS),Windows_NT)
	POST_FIX = dll
	LIB = -L. -ldict
else
	UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        CFLAG += -Wno-unused-result
		POST_FIX = so
		LIB = -L. -ldict
		ELF_FILES := $(shell find . -type f -executable -exec sh -c 'file -b {} | grep -q ELF' \; -print)
    endif
endif

all: dict
.PHONY: dict

dict: $(DIR)/dict.c $(DIR)/dict.h
	$(CC) $(CFLAG) -fPIC -shared $< -o lib$@.$(POST_FIX)

test%: test%.c
	$(CC) $(CFLAG) $< -o test $(LIB)

clean:
	rm *.dll *.exe *.o $(ELF_FILES)