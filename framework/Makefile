# ============================================================================
# Makefile - Framework Layer (libbhs_framework.a)
# ============================================================================
# HAL (Platform + RHI) + UI Framework.
# Não sabe o que é buraco negro. Só sabe desenhar triângulos.
# ============================================================================

ROOT_DIR := ..
include $(ROOT_DIR)/src/config.mk

MODULE_DIR := $(BUILD_DIR)/framework
OBJ_DIR := $(MODULE_DIR)/obj

# Include path - raiz do projeto
CFLAGS += -I$(ROOT_DIR)
CXXFLAGS += -I$(ROOT_DIR)

# ============================================================================
# PLATFORM SOURCES (OS-specific)
# ============================================================================

ifeq ($(PLATFORM),wayland)
    PLATFORM_SRCS := platform/wayland/wl_context.c \
                     platform/wayland/wl_window.c \
                     platform/wayland/wl_input.c \
                     platform/wayland/xdg-shell-protocol.c \
                     platform/wayland/pointer-constraints-protocol.c \
                     platform/wayland/relative-pointer-protocol.c
    PKG_PLATFORM := wayland-client wayland-cursor xkbcommon
else ifeq ($(PLATFORM),x11)
    PLATFORM_SRCS := platform/x11/x11.c
    PKG_PLATFORM := x11 xkbcommon
else ifeq ($(PLATFORM),cocoa)
    PLATFORM_SRCS := platform/cocoa/cocoa.mm
    PKG_PLATFORM :=
else ifeq ($(PLATFORM),win32)
    PLATFORM_SRCS := platform/win32/win32.cpp
    PKG_PLATFORM :=
endif

# ============================================================================
# RHI SOURCES (Renderer-specific)
# ============================================================================

ifeq ($(RENDERER),vulkan)
    RHI_SRCS := rhi/vulkan/vk_context.c \
                rhi/vulkan/vk_swapchain.c \
                rhi/vulkan/vk_pipeline.c \
                rhi/vulkan/vk_buffer.c \
                rhi/vulkan/vk_texture.c \
                rhi/vulkan/vk_sync.c
    PKG_RHI := vulkan
else ifeq ($(RENDERER),metal)
    RHI_SRCS := rhi/metal/metal.mm
    PKG_RHI :=
else ifeq ($(RENDERER),opengl)
    RHI_SRCS := rhi/opengl/opengl.c
    PKG_RHI := gl
else ifeq ($(RENDERER),dx)
    RHI_SRCS := rhi/dx/directx.cpp
    PKG_RHI :=
endif

# ============================================================================
# UI SOURCES
# ============================================================================

UI_SRCS := ui/context.c \
           ui/layout.c \
           ui/theme.c \
           ui/widgets.c \
           ui/render/render2d.c \
           ui/window/window.c

# ============================================================================
# CORE SOURCES (Always compiled)
# ============================================================================

CORE_SRCS := log.c

# ============================================================================
# COMBINE ALL
# ============================================================================

SRCS := $(CORE_SRCS) $(PLATFORM_SRCS) $(RHI_SRCS) $(UI_SRCS)

# Objects (handling multiple extensions)
OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(filter %.c,$(SRCS)))
OBJS += $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(filter %.cpp,$(SRCS)))
OBJS += $(patsubst %.mm,$(OBJ_DIR)/%.o,$(filter %.mm,$(SRCS)))

LIB := $(MODULE_DIR)/libbhs_framework.a

# pkg-config
PKG_DEPS := $(PKG_PLATFORM) $(PKG_RHI)
ifneq ($(PKG_DEPS),)
    PKG_CFLAGS := $(shell pkg-config --cflags $(PKG_DEPS) 2>/dev/null)
    CFLAGS += $(PKG_CFLAGS)
    CXXFLAGS += $(PKG_CFLAGS)
endif

# ============================================================================
# TARGETS
# ============================================================================

.PHONY: all clean

all: $(LIB)
	@echo "[FRAMEWORK] Biblioteca pronta: $(LIB)"
	@echo "            Platform: $(PLATFORM)"
	@echo "            Renderer: $(RENDERER)"

$(LIB): $(OBJS) | $(MODULE_DIR)
	@echo "  AR      $@"
	$(Q)$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "  CXX     $<"
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.mm | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "  OBJCXX  $<"
	$(Q)$(CXX) $(CXXFLAGS) -fobjc-arc -c $< -o $@

$(MODULE_DIR) $(OBJ_DIR):
	@mkdir -p $@

clean:
	@echo "  CLEAN   framework"
	$(Q)rm -rf $(MODULE_DIR)
