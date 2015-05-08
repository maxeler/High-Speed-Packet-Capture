LIB_NAME := common

DEFINE_FLAGS :=
BUILD_FLAGS := -Wall -Werror -pedantic -std=gnu99 -fgnu89-inline -fPIC
DEBUG_FLAGS := -g
CFLAGS := $(DEBUG_FLAGS) $(BUILD_FLAGS) $(DEFINE_FLAGS) -I. -lm -pthread
OBJ_DIR := .objects

CC := gcc
AR := ar

SOURCES := $(wildcard *.c)
OBJECTS := $(addprefix $(OBJ_DIR)/, $(SOURCES:.c=.o))


.PHONY: clean all

all: lib$(LIB_NAME).a

lib$(LIB_NAME).a: $(OBJECTS)
	$(AR) rcs $@ $^

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm -rf $(OBJ_DIR)
	-rm lib$(LIB_NAME).a
