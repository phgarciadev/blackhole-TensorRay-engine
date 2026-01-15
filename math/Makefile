# ============================================================================
# Makefile - Math Layer (libbhs_math.a)
# ============================================================================
# Camada puramente computacional. ZERO dependências de OS/GPU.
# Se você incluir <vulkan.h> aqui, eu te encontro.
# ============================================================================

ROOT_DIR := ..
include $(ROOT_DIR)/src/config.mk

MODULE_DIR := $(BUILD_DIR)/math
OBJ_DIR := $(MODULE_DIR)/obj

# Fontes da camada matemática
SRCS := vec4.c \
        tensor/tensor.c \
        spacetime/kerr.c \
        spacetime/schwarzschild.c

OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))
LIB := $(MODULE_DIR)/libbhs_math.a

# Include path - raiz do projeto
CFLAGS += -I$(ROOT_DIR)

# ============================================================================
# TARGETS
# ============================================================================

.PHONY: all clean test

all: $(LIB)
	@echo "[MATH] Biblioteca pronta: $(LIB)"

$(LIB): $(OBJS) | $(MODULE_DIR)
	@echo "  AR      $@"
	$(Q)$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

$(MODULE_DIR) $(OBJ_DIR):
	@mkdir -p $@
	@mkdir -p $(OBJ_DIR)/tensor
	@mkdir -p $(OBJ_DIR)/spacetime

clean:
	@echo "  CLEAN   math"
	$(Q)rm -rf $(MODULE_DIR)

test:
	@echo "[MATH] Testes? Pfff, matemática não erra. Confia."
