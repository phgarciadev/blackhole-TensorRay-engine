# ============================================================================
# Makefile - Testes de Integração
# ============================================================================
# Compila e roda testes de integração dos módulos UX
#
# Uso:
#   make              # Compila todos os testes
#   make run          # Compila e roda test_window
#   make debug        # Compila com debug
#   make clean        # Limpa
# ============================================================================

# Diretórios
BUILD_DIR := ../../build/tests/integration
ROOT_DIR := ../..
PLATFORM_LIB := $(ROOT_DIR)/build/ux/platform
RENDERER_LIB := $(ROOT_DIR)/build/ux/renderer
FRAMEWORK_LIB := $(ROOT_DIR)/build/ux/lib
UI_LIB := $(ROOT_DIR)/build/ux/ui

# Detecta plataforma
UNAME := $(shell uname -s)
ifeq ($(UNAME),Linux)
    PLATFORM := wayland
    RENDERER := vulkan
    PKG_DEPS := wayland-client xkbcommon vulkan
else ifeq ($(UNAME),Darwin)
    PLATFORM := cocoa
    RENDERER := metal
    PKG_DEPS :=
endif

# Compilador (LLVM padrão)
CC := clang

# Flags
CFLAGS := -std=c11 -Wall -Wextra -I$(ROOT_DIR)/src
LDFLAGS := -L$(UI_LIB) -L$(FRAMEWORK_LIB) -L$(PLATFORM_LIB) -L$(RENDERER_LIB)
LDFLAGS += -lbhs_ui -lbhs_lib -lbhs_platform_$(PLATFORM) -lbhs_renderer_$(RENDERER)

ifneq ($(PKG_DEPS),)
    CFLAGS += $(shell pkg-config --cflags $(PKG_DEPS))
    LDFLAGS += $(shell pkg-config --libs $(PKG_DEPS))
endif

ifeq ($(DEBUG),1)
    CFLAGS += -g -O0 -DBHS_DEBUG -fsanitize=address,undefined
    LDFLAGS += -fsanitize=address,undefined
else
    CFLAGS += -g -O0 -DBHS_DEBUG
endif

.PHONY: all run run_ui debug clean libs

all: libs $(BUILD_DIR)/test_window $(BUILD_DIR)/test_ui
	@echo "[TESTS] Testes compilados"

# Garante que as libs estão compiladas
libs:
	$(MAKE) -C $(ROOT_DIR) all

$(BUILD_DIR)/test_window: test_window.c | $(BUILD_DIR)
	@echo "  CC      $<"
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(BUILD_DIR)/test_ui: test_ui.c | $(BUILD_DIR)
	@echo "  CC      $<"
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(BUILD_DIR):
	@mkdir -p $@

run: all
	@echo ""
	@echo "=== Executando test_window ==="
	@echo ""
	$(BUILD_DIR)/test_window

debug:
	$(MAKE) DEBUG=1 all

clean:
	rm -rf $(BUILD_DIR)
