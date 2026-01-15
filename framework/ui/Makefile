# ============================================================================
# Makefile - Lib UI Framework
# ============================================================================

ROOT_DIR := ../../..
include $(ROOT_DIR)/src/config.mk

# Diretórios
MODULE_DIR := $(BUILD_DIR)/lib/ui_framework
OBJ_DIR := $(MODULE_DIR)/obj

# Relative paths to HAL (for dependencies check only)
# In the new architecture, we assume HAL is built by the root Makefile.
# But if checked here:
PLATFORM_DIR := ../../hal/os
RENDERER_DIR := ../../hal/gpu

# Arquivos fonte
SRCS := context.c window/window.c render/render2d.c layout.c theme.c widgets.c

# Shaders UI
SHADER_SRC_DIR := render/shaders
SHADER_OUT_DIR := $(ROOT_DIR)/build/shaders

SHADERS_SRC := $(wildcard $(SHADER_SRC_DIR)/*.vert) $(wildcard $(SHADER_SRC_DIR)/*.frag)
SHADERS_SPV := $(patsubst $(SHADER_SRC_DIR)/%,$(SHADER_OUT_DIR)/%.spv,$(SHADERS_SRC))

# Objetos
OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))

# Biblioteca
LIB := $(MODULE_DIR)/libbhs_ui_framework.a

# ============================================================================
# TARGETS
# ============================================================================

.PHONY: all debug release clean shaders

# We don't force deps here to avoid recursion loops, root makefile handles order.
all: $(LIB) shaders
	@echo "[LIB] UI Framework ready: $(LIB)"

debug:
	$(MAKE) DEBUG=1

release:
	$(MAKE) RELEASE=1

$(LIB): $(OBJS) | $(MODULE_DIR)
	@echo "  AR      $@"
	$(Q)$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@echo "  CC      $<"
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

# Regra de Shaders
shaders: $(SHADERS_SPV)

$(SHADER_OUT_DIR)/%.spv: $(SHADER_SRC_DIR)/% | $(SHADER_OUT_DIR)
	@echo "  GLSL    $<"
	@glslangValidator -V $< -o $@
	@mkdir -p $(ROOT_DIR)/shaders
	@cp $@ $(ROOT_DIR)/shaders/

$(MODULE_DIR) $(OBJ_DIR):
	@mkdir -p $@

$(SHADER_OUT_DIR):
	@mkdir -p $@

clean:
	@echo "  CLEAN   lib/ui_framework"
	$(Q)rm -rf $(MODULE_DIR)
