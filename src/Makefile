# ============================================================================
# Makefile - App / Entry Point (src/)
# ============================================================================
# Onde tudo se junta. Entry point, UI específica, debug tools.
# Se crashar, começa a procurar por aqui.
# ============================================================================

ROOT_DIR := ..
include config.mk

MODULE_DIR := $(BUILD_DIR)/src
OBJ_DIR := $(MODULE_DIR)/obj

# Include path - raiz do projeto
CFLAGS += -I$(ROOT_DIR)

# ============================================================================
# SOURCES
# ============================================================================

SRCS := main.c \
        ui/camera/camera.c \
        ui/camera/camera_controller.c \
        ui/render/spacetime_renderer.c \
        ui/screens/view_spacetime.c \
        ui/screens/hud.c \
        debug/telemetry.c

OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))

EXEC := $(BUILD_DIR)/blackhole_sim

# ============================================================================
# LINKAGE
# ============================================================================

# Ordem importa: Engine usa Framework e Math, Framework usa Math
LIBS := -L$(BUILD_DIR)/engine -lbhs_engine \
        -L$(BUILD_DIR)/framework -lbhs_framework \
        -L$(BUILD_DIR)/math -lbhs_math

# ============================================================================
# TARGETS
# ============================================================================

.PHONY: all clean

all: $(EXEC)
	@echo "[APP] Executável pronto: $(EXEC)"
	@echo "      Platform: $(PLATFORM)"
	@echo "      Renderer: $(RENDERER)"

$(EXEC): $(OBJS)
	@mkdir -p $(dir $@)
	@echo "  LD      $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@ $(LIBS) $(LDFLAGS)

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

$(MODULE_DIR) $(OBJ_DIR):
	@mkdir -p $@
	@mkdir -p $(OBJ_DIR)/ui/camera
	@mkdir -p $(OBJ_DIR)/ui/render
	@mkdir -p $(OBJ_DIR)/ui/screens
	@mkdir -p $(OBJ_DIR)/debug

clean:
	@echo "  CLEAN   src"
	$(Q)rm -rf $(MODULE_DIR)
