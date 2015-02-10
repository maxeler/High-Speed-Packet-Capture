DEFINE_FLAGS :=
BUILD_FLAGS := -Wall -Werror -pedantic -std=gnu99 -fgnu89-inline
DEBUG_FLAGS := -g
CFLAGS := $(DEBUG_FLAGS) $(BUILD_FLAGS) $(DEFINE_FLAGS) -I. -lm
OBJ_DIR := .objects

CC := gcc
SOURCES := capture_server.c pcap.c
EXE := capture_server

.PHONY: clean

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.c $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(EXE): $(OBJ_DIR)/capture_server.o $(OBJ_DIR)/pcap.o
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	-rm -rf $(OBJ_DIR)
	-rm $(EXE)
