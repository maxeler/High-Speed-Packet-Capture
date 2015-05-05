LIB_COMMON := ../common/libcommon.a
LIB_COMMON_DIR := $(dir $(LIB_COMMON))

DEFINE_FLAGS :=
BUILD_FLAGS := -Wall -Werror -pedantic -std=gnu99 -fgnu89-inline
DEBUG_FLAGS := -g
LIB_FLAGS := -L$(LIB_COMMON_DIR) -lm -lcommon -pthread
CFLAGS := $(DEBUG_FLAGS) $(BUILD_FLAGS) $(DEFINE_FLAGS) -I. -I$(LIB_COMMON_DIR) $(LIB_FLAGS)
OBJ_DIR := .objects

CC := gcc
SOURCES := $(wildcard *.c)
OBJECTS := $(addprefix $(OBJ_DIR)/, $(SOURCES:.c=.o))
LIBS := $(LIB_COMMON)
EXE := capture_server

.PHONY: clean all

all: $(EXE)

$(EXE): $(OBJECTS) $(LIBS)
	$(CC) -o $@ $(filter %.o, $^) $(CFLAGS)

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm -rf $(OBJ_DIR)
	-rm $(EXE)
