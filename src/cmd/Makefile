# ============================================================================
# Makefile - Command Module (Application)
# ============================================================================

ROOT_DIR := ../..
include $(ROOT_DIR)/src/config.mk

# Directories
MODULE_DIR := $(BUILD_DIR)/cmd
OBJ_DIR := $(MODULE_DIR)/obj

# Sources
SRCS := main.c \
        ui/camera/camera.c \
        ui/camera/camera_controller.c \
        ui/render/spacetime_renderer.c \
        ui/render/spacetime_renderer.c \
        ui/screens/view_spacetime.c \
        ui/screens/hud.c

# Scan for other sources matching pattern if needed
# SRCS += $(wildcard ui/*.c) # Careful with duplicates

OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))

# Output
EXEC := $(BUILD_DIR)/blackhole_sim

# Libs from other modules
LIBS := -L$(BUILD_DIR)/engine -lbhs_engine \
        -L$(BUILD_DIR)/lib -lbhs_core \
        -L$(BUILD_DIR)/lib/ui_framework -lbhs_ui_framework \
        -L$(BUILD_DIR)/hal/os -lbhs_hal_os_$(PLATFORM) \
        -L$(BUILD_DIR)/hal/gpu -lbhs_hal_gpu_$(RENDERER)

# ============================================================================
# TARGETS
# ============================================================================

.PHONY: all clean

all: $(EXEC)
	@echo "[CMD] Executable ready: $(EXEC)"

$(EXEC): $(OBJS)
	@echo "  LD      $@"
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) $^ -o $@ $(LIBS) $(LDFLAGS)

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@echo "  CC      $<"
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

$(MODULE_DIR) $(OBJ_DIR):
	@mkdir -p $@

clean:
	@echo "  CLEAN   cmd"
	$(Q)rm -rf $(MODULE_DIR)
