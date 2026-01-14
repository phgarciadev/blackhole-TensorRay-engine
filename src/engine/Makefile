# ============================================================================
# Makefile - Engine (Physics + Compute)
# ============================================================================

ROOT_DIR := ../..
include $(ROOT_DIR)/src/config.mk

# Directories
MODULE_DIR := $(BUILD_DIR)/engine
OBJ_DIR := $(MODULE_DIR)/obj
SHADER_SRC_DIR := shaders
SHADER_BIN_DIR := $(MODULE_DIR)/shaders

# Arquivos fonte
SRCS := engine.c \
        world/world.c \
        render_system/render_system.c \
        scene/scene.c \
        systems/physics_system.c \
        systems/gravity_system.c \
        systems/celestial_system.c \
        presets/presets.c \
        integrator/integrator.c \
        body/body_dynamics.c \
        body/body_factory.c \
        spacetime/spacetime_data.c \
        spacetime/spacetime_physics.c \
        disk/disk.c \
        geodesic/geodesic.c \
        $(wildcard planets/*.c)

# (Assuming these files exist, based on previous structure analysis)
OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))

# Shaders (GLSL -> SPIR-V)
SHADER_SRCS := $(SHADER_SRC_DIR)/grid_fiel.comp $(SHADER_SRC_DIR)/blackhole.comp
SHADER_BINS := $(SHADER_BIN_DIR)/grid_fiel.comp.spv $(SHADER_BIN_DIR)/blackhole.comp.spv

# Biblioteca
LIB := $(MODULE_DIR)/libbhs_engine.a

# Targets
.PHONY: all clean shaders

all: $(LIB) shaders
	@echo "[ENGINE] Engine ready: $(LIB)"

$(LIB): $(OBJS) | $(MODULE_DIR)
	@echo "  AR      $@"
	$(Q)$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

shaders: $(SHADER_BINS)

$(SHADER_BIN_DIR)/%.comp.spv: $(SHADER_SRC_DIR)/%.comp | $(SHADER_BIN_DIR)
	@echo "  GLSLC   $<"
	$(Q)glslc $< -o $@

$(MODULE_DIR) $(OBJ_DIR) $(SHADER_BIN_DIR):
	@mkdir -p $@
	@mkdir -p $(OBJ_DIR)/scene

clean:
	@echo "  CLEAN   engine"
	$(Q)rm -rf $(MODULE_DIR)
