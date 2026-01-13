# ============================================================================
# Makefile - Black Hole Simulator (Event Horizon Architecture)
# ============================================================================

# Project Root
ROOT_DIR := .
include src/config.mk

# Directories
SRC_DIR := src
CMD_DIR := src/cmd
ENGINE_DIR := src/engine
HAL_DIR := src/hal
LIB_DIR := src/lib

# Linker Flags (Libraries)
LDFLAGS := -lm -pthread $(LDFLAGS_OPT)

ifeq ($(PLATFORM),wayland)
    LDFLAGS += -lwayland-client -lwayland-cursor -lwayland-egl -lxkbcommon
endif

ifeq ($(PLATFORM),x11)
    LDFLAGS += -lX11 -lxkbcommon
endif

ifeq ($(RENDERER),vulkan)
    LDFLAGS += -lvulkan
endif

ifeq ($(RENDERER),opengl)
    LDFLAGS += -lGL
endif
# ============================================================================
# TARGETS
# ============================================================================

.PHONY: all hal engine lib cmd clean

all: hal lib engine cmd shaders
	@echo ""
	@echo "=== Event Horizon Build Complete ==="
	@echo "Platform: $(PLATFORM)"
	@echo "Renderer: $(RENDERER)"
	@echo "Executable: $(BUILD_DIR)/blackhole_sim"
	@echo ""

# --- HAL (Hardware Abstraction Layer) ---
hal:
	@echo ">>> Building HAL..."
	$(MAKE) -C $(HAL_DIR)

# --- Lib (Core & Frameworks) ---
lib:
	@echo ">>> Building Lib..."
	$(MAKE) -C $(LIB_DIR)

# --- Engine (Simulation) ---
engine: lib
	@echo ">>> Building Engine..."
	$(MAKE) -C $(ENGINE_DIR)

# --- CMD (App Entry Point) ---
# Links everything together
cmd: hal lib engine
	@echo ">>> Building Command Module (Executable)..."
	$(MAKE) -C $(CMD_DIR)

# --- Shaders (C -> SPIR-V) ---
SHADER_SRC_DIR := src/assets/shaders
SHADER_OUT_DIR := assets/shaders
SHADER_SRCS := $(wildcard $(SHADER_SRC_DIR)/*.c)
SHADER_OBJS := $(patsubst $(SHADER_SRC_DIR)/%.c,$(SHADER_OUT_DIR)/%.spv,$(SHADER_SRCS))

# Flags para compilar C como SPIR-V
# -target spirv64: Alvo Vulkan/SPIR-V 64-bit
# -x cl: Força linguagem OpenCL C (C com extensions de GPU)
# -cl-std=CL2.0: Padrão moderno
# -Xclang -finclude-default-header: Inclui stdlib do OpenCL (get_global_id, etc)
# -Wno-unused-value: Silencia warnings comuns em macros
CLANG_SPIRV_FLAGS := -c -target spirv64 -x cl -cl-std=CL2.0 -Xclang -finclude-default-header -O3 -I src -D BHS_SHADER_COMPILER -D BHS_USE_FLOAT -Wno-unused-value

shaders: $(SHADER_OBJS)
	@echo ">>> Shaders Compiled."

$(SHADER_OUT_DIR)/%.spv: $(SHADER_SRC_DIR)/%.c
	@mkdir -p $(SHADER_OUT_DIR)
	@echo "  SPIR-V  $<"
	clang $(CLANG_SPIRV_FLAGS) $< -o $@

# ============================================================================
# UTILS
# ============================================================================

clean:
	@echo ">>> Cleaning build..."
	rm -rf $(BUILD_DIR)
	$(MAKE) -C $(HAL_DIR) clean
	$(MAKE) -C $(LIB_DIR) clean
	$(MAKE) -C $(ENGINE_DIR) clean
	@echo ">>> Cleaned."

test:
	@echo ">>> Running Tests..."
	$(MAKE) -C $(LIB_DIR) test

info:
	@echo "=== Build Info ==="
	@echo "Platform: $(PLATFORM)"
	@echo "Renderer: $(RENDERER)"

help:
	@echo "Targets: all, hal, lib, engine, cmd, clean, test, info"