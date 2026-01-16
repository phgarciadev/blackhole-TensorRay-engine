# ============================================================================
# Makefile - HAL OS
# ============================================================================

ROOT_DIR := ../../..
include $(ROOT_DIR)/src/config.mk

# Directories
MODULE_DIR := $(BUILD_DIR)/hal/os
OBJ_DIR := $(MODULE_DIR)/obj/$(PLATFORM)

# Arquivos fonte por backend
ifeq ($(PLATFORM),wayland)
    SRCS := wayland/wl_context.c \
            wayland/wl_window.c \
            wayland/wl_input.c \
            wayland/xdg-shell-protocol.c \
            wayland/pointer-constraints-protocol.c \
            wayland/relative-pointer-protocol.c
    PKG_DEPS := wayland-client wayland-cursor xkbcommon
    EXTRA_CFLAGS :=
else ifeq ($(PLATFORM),x11)
    SRCS := x11/x11.c
    PKG_DEPS := x11 xkbcommon
    EXTRA_CFLAGS :=
else ifeq ($(PLATFORM),cocoa)
    SRCS := cocoac/coc/cocoa.mm
    PKG_DEPS :=
    EXTRA_CFLAGS := -fobjc-arc
    CC := clang
    CXX := clang++
else ifeq ($(PLATFORM),win32)
    SRCS := win32/win32.cpp
    PKG_DEPS :=
    EXTRA_CFLAGS :=
endif

# Objetos
OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(filter %.c,$(SRCS)))
OBJS += $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(filter %.cpp,$(SRCS)))
OBJS += $(patsubst %.mm,$(OBJ_DIR)/%.o,$(filter %.mm,$(SRCS)))

# Biblioteca de saída
LIB := $(MODULE_DIR)/libbhs_hal_os_$(PLATFORM).a

# pkg-config (Append to flags)
ifneq ($(PKG_DEPS),)
    PKG_CFLAGS := $(shell pkg-config --cflags $(PKG_DEPS) 2>/dev/null)
    CFLAGS += $(PKG_CFLAGS)
    CXXFLAGS += $(PKG_CFLAGS)
endif

# ============================================================================
# TARGETS
# ============================================================================

.PHONY: all wayland x11 cocoa win32 debug release test clean info

all: $(LIB)
	@echo "[HAL] OS $(PLATFORM) ready: $<"

wayland:
	$(MAKE) PLATFORM=wayland

x11:
	$(MAKE) PLATFORM=x11

cocoa:
	$(MAKE) PLATFORM=cocoa

win32:
	$(MAKE) PLATFORM=win32

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

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	@echo "  CXX     $<"
	@mkdir -p $(dir $@)
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.mm | $(OBJ_DIR)
	@echo "  OBJCXX  $<"
	@mkdir -p $(dir $@)
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

$(MODULE_DIR) $(OBJ_DIR):
	@mkdir -p $@

test: $(LIB)
	@echo "[HAL] OS Tests not implemented"

clean:
	@echo "  CLEAN   hal/os/$(PLATFORM)"
	$(Q)rm -rf $(MODULE_DIR)

info:
	@echo "=== HAL OS Build Info ==="
	@echo "System:    $(UNAME)"
	@echo "Backend:   $(PLATFORM)"
	@echo "Output:    $(LIB)"