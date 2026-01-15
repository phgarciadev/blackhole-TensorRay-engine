# ============================================================================
# Makefile - Black Hole Simulator (Layered Architecture)
# ============================================================================
#
# Arquitetura em 4 camadas (na raiz do projeto):
#   math/      -> Matemática pura (tensores, métricas, vetores)
#   framework/ -> HAL + RHI + UI (janelas, GPU, widgets)
#   engine/    -> Simulação (ECS, física, geodésicas)
#   src/       -> Entry point e glue code
#
# ============================================================================

ROOT_DIR := .
include src/config.mk

# Subdirectories (ordem de dependência)
SUBDIRS := math framework engine src

# ============================================================================
# TARGETS
# ============================================================================

.PHONY: all $(SUBDIRS) clean test shaders info help

all: math framework engine src shaders
	@echo ""
	@echo "=== Black Hole Simulator Build Complete ==="
	@echo "Platform: $(PLATFORM)"
	@echo "Renderer: $(RENDERER)"
	@echo "Executable: $(BUILD_DIR)/blackhole_sim"
	@echo ""

# Dependências explícitas entre camadas
math:
	@echo ">>> Building Math Layer..."
	$(MAKE) -C $@

framework: math
	@echo ">>> Building Framework Layer..."
	$(MAKE) -C $@

engine: math framework
	@echo ">>> Building Engine Layer..."
	$(MAKE) -C $@

src: math framework engine
	@echo ">>> Building App (src/)..."
	$(MAKE) -C $@

# ============================================================================
# SHADERS
# ============================================================================

SHADER_SRC_DIR := src/assets/shaders
SHADER_OUT_DIR := assets/shaders
SHADER_SRCS := $(wildcard $(SHADER_SRC_DIR)/*.c)
SHADER_OBJS := $(patsubst $(SHADER_SRC_DIR)/%.c,$(SHADER_OUT_DIR)/%.spv,$(SHADER_SRCS))

CLANG_SPIRV_FLAGS := -c -target spirv64 -x cl -cl-std=CL2.0 \
                     -Xclang -finclude-default-header -O3 \
                     -I . -D BHS_SHADER_COMPILER -D BHS_USE_FLOAT \
                     -Wno-unused-value

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
	@echo ">>> Cleaning all layers..."
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean 2>/dev/null || true; done
	rm -rf $(BUILD_DIR)
	@echo ">>> Cleaned."

test:
	@echo ">>> Running Tests..."
	$(MAKE) -C math test
	$(MAKE) -C engine test 2>/dev/null || echo "[TEST] Engine tests not implemented"

info:
	@echo "=== Build Info ==="
	@echo "Platform: $(PLATFORM)"
	@echo "Renderer: $(RENDERER)"
	@echo "Layers:   math/ -> framework/ -> engine/ -> src/"

help:
	@echo "Targets:"
	@echo "  all       - Build everything (default)"
	@echo "  math      - Build Math layer only"
	@echo "  framework - Build Framework layer only"
	@echo "  engine    - Build Engine layer only"
	@echo "  src       - Build App (entry point) only"
	@echo "  shaders   - Compile SPIR-V shaders"
	@echo "  clean     - Remove all build artifacts"
	@echo "  test      - Run tests"
	@echo "  info      - Show build configuration"