# --------------------------------- VARIABLES ---------------------------------

# User-defined variables
BUILD_DIR := build

BIN := conwaygol

SRCS := main.c
OBJS := $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))

CC      := gcc
CFLAGS  := -std=c89 -Wall -Wextra -pedantic
LDFLAGS := -lc -lncurses


# ----------------------------------- GOALS -----------------------------------

.PHONY: release clean

# Main goal
release: $(BUILD_DIR)/$(BIN)

# Linking
$(BUILD_DIR)/$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

# Compiling
$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

# Clean
clean:
	$(RM) -r $(BUILD_DIR)
