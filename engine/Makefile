# ============================================================================
# Makefile - Engine Layer (libbhs_engine.a)
# ============================================================================
# O cérebro da simulação. ECS, física, geodésicas, discos de acreção.
# Se algo orbitou, foi culpa desse módulo.
# ============================================================================

ROOT_DIR := ..
include $(ROOT_DIR)/src/config.mk

MODULE_DIR := $(BUILD_DIR)/engine
OBJ_DIR := $(MODULE_DIR)/obj
SHADER_SRC_DIR := shaders
SHADER_BIN_DIR := $(MODULE_DIR)/shaders

# Include path - raiz do projeto
CFLAGS += -I$(ROOT_DIR)

# ============================================================================
# SOURCES
# ============================================================================

SRCS := engine.c \
        ecs/ecs.c \
        ecs/events.c \
        assets/image_loader.c \
        assets/image_gen.c \
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

OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))
LIB := $(MODULE_DIR)/libbhs_engine.a

# Shaders (GLSL -> SPIR-V)
SHADER_SRCS := $(wildcard $(SHADER_SRC_DIR)/*.comp)
SHADER_BINS := $(patsubst $(SHADER_SRC_DIR)/%.comp,$(SHADER_BIN_DIR)/%.comp.spv,$(SHADER_SRCS))

# ============================================================================
# TARGETS
# ============================================================================

.PHONY: all clean shaders

all: $(LIB) shaders
	@echo "[ENGINE] Biblioteca pronta: $(LIB)"

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
	@mkdir -p $(OBJ_DIR)/ecs
	@mkdir -p $(OBJ_DIR)/assets
	@mkdir -p $(OBJ_DIR)/world
	@mkdir -p $(OBJ_DIR)/render_system
	@mkdir -p $(OBJ_DIR)/scene
	@mkdir -p $(OBJ_DIR)/systems
	@mkdir -p $(OBJ_DIR)/presets
	@mkdir -p $(OBJ_DIR)/integrator
	@mkdir -p $(OBJ_DIR)/body
	@mkdir -p $(OBJ_DIR)/spacetime
	@mkdir -p $(OBJ_DIR)/disk
	@mkdir -p $(OBJ_DIR)/geodesic
	@mkdir -p $(OBJ_DIR)/planets

clean:
	@echo "  CLEAN   engine"
	$(Q)rm -rf $(MODULE_DIR)
