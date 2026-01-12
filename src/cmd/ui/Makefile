# ============================================================================
# Makefile - UX UI (Aplicação Visual)
# ============================================================================

ROOT_DIR := ../../..
include $(ROOT_DIR)/src/config.mk

# Diretórios
MODULE_DIR := $(BUILD_DIR)/ux/ui
OBJ_DIR := $(MODULE_DIR)/obj

# Fontes
SRCS := screens/view_spacetime.c
OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))

# Biblioteca
LIB := $(MODULE_DIR)/libbhs_ui.a

# Target
.PHONY: all clean

all: $(LIB)
	@echo "[UI] libbhs_ui.a pronta"

$(LIB): $(OBJS) | $(MODULE_DIR)
	@echo "  AR      $@"
	$(Q)$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

$(MODULE_DIR) $(OBJ_DIR):
	@mkdir -p $@

clean:
	@echo "  CLEAN   ui"
	$(Q)rm -rf $(MODULE_DIR)
