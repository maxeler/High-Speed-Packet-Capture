DEFINE_FLAGS :=
BUILD_FLAGS := -Wall -Werror -pedantic -std=gnu99 -fgnu89-inline
DEBUG_FLAGS := -g
CFLAGS := $(DEBUG_FLAGS) $(BUILD_FLAGS) $(DEFINE_FLAGS) -I.
OBJ_DIR := .objects

CC := gcc
SOURCES := part_server.c
EXE := part_server

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.c $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(EXE): $(OBJ_DIR)/part_server.o
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	-rm -rf $(OBJ_DIR)
	-rm $(EXE)