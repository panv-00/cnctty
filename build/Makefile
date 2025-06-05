# Compiler and flags
CC := gcc
# CFLAGS := -D_GNU_SOURCE -std=c99 -Wall -Werror -O3 -g -O0
# CFLAGS := -D_GNU_SOURCE -std=c99 -Wall -Werror -O3 -g
CFLAGS := -D_GNU_SOURCE -std=c99 -Wall -O3 -g
LDFLAGS := -lssl -lcrypto

# Directories
SRC_DIR := ../src
OBJ_DIR := ./objects
BIN := ../cnctty

VERSION_HEADER := $(SRC_DIR)/version.h
GIT_HEAD := ../.git/HEAD
GEN_VERSION := ./version.sh

# Source and object files
SRC_FILES := $(shell find -L $(SRC_DIR) -name '*.c')
SRC_FILES := $(filter-out ../src/cnc_library/src/sample.c, $(SRC_FILES))
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

all: $(BIN)

prebuild:
	@rm -f $(VERSION_HEADER)

$(VERSION_HEADER): prebuild
	$(GEN_VERSION) $(VERSION_HEADER)

$(BIN): $(VERSION_HEADER) $(OBJ_FILES)
	@mkdir -p $(dir $@)
	$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(VERSION_HEADER)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
