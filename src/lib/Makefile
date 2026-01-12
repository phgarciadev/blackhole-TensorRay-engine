# ============================================================================
# Makefile - Lib (Core + UI)
# ============================================================================

ROOT_DIR := ../..
include $(ROOT_DIR)/src/config.mk

# Directories
MODULE_DIR := $(BUILD_DIR)/lib
OBJ_DIR := $(MODULE_DIR)/obj

# Submodules
UI_FRAMEWORK_DIR := ui_framework

# Core Sources (Math, Tensor, Spacetime, etc)
# We find all .c files in subdirectories except ui_framework
# Using wildcard is a bit dangerous if structure changes, but works for now.
# Better: Explicit list.
CORE_SRCS := math/vec4.c \
             tensor/tensor.c \
             spacetime/schwarzschild.c \
             spacetime/kerr.c

# Objects
CORE_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(CORE_SRCS))

# Libraries
LIB_CORE := $(MODULE_DIR)/libbhs_core.a

# ============================================================================
# TARGETS
# ============================================================================

.PHONY: all ui_framework core clean test

all: core ui_framework
	@echo "[LIB] Core and UI ready."

ui_framework:
	$(MAKE) -C $(UI_FRAMEWORK_DIR)

core: $(LIB_CORE)

$(LIB_CORE): $(CORE_OBJS) | $(MODULE_DIR)
	@echo "  AR      $@"
	$(Q)$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@echo "  CC      $<"
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

$(MODULE_DIR) $(OBJ_DIR):
	@mkdir -p $@

clean:
	@echo "  CLEAN   lib (core)"
	$(Q)rm -rf $(MODULE_DIR)
	$(MAKE) -C $(UI_FRAMEWORK_DIR) clean

test:
	@echo "Running lib tests..."
