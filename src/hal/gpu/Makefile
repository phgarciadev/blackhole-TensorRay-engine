# ============================================================================
# Makefile - HAL GPU
# ============================================================================

ROOT_DIR := ../../..
include $(ROOT_DIR)/src/config.mk

# Directories
MODULE_DIR := $(BUILD_DIR)/hal/gpu
OBJ_DIR := $(MODULE_DIR)/obj/$(RENDERER)

# Arquivos fonte por backend
ifeq ($(RENDERER),vulkan)
    SRCS := vulkan/vk_context.c \
            vulkan/vk_buffer.c \
            vulkan/vk_texture.c \
            vulkan/vk_swapchain.c \
            vulkan/vk_pipeline.c \
            vulkan/vk_sync.c
    PKG_DEPS := vulkan
    EXTRA_CFLAGS :=
else ifeq ($(RENDERER),metal)
    SRCS := metal/metal.mm
    PKG_DEPS :=
    EXTRA_CFLAGS := -fobjc-arc
    CC := clang
    CXX := clang++
    FRAMEWORKS := -framework Metal -framework QuartzCore -framework Foundation
else ifeq ($(RENDERER),dx)
    SRCS := dx/directx.cpp
    PKG_DEPS :=
    EXTRA_CFLAGS :=
else ifeq ($(RENDERER),opengl)
    SRCS := opengl/opengl.c
    PKG_DEPS := gl
    EXTRA_CFLAGS :=
endif

# Objetos
OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(filter %.c,$(SRCS)))
OBJS += $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(filter %.cpp,$(SRCS)))
OBJS += $(patsubst %.mm,$(OBJ_DIR)/%.o,$(filter %.mm,$(SRCS)))

# Biblioteca de saída
LIB := $(MODULE_DIR)/libbhs_hal_gpu_$(RENDERER).a

# pkg-config (Append to flags)
ifneq ($(PKG_DEPS),)
    PKG_CFLAGS := $(shell pkg-config --cflags $(PKG_DEPS) 2>/dev/null)
    CFLAGS += $(PKG_CFLAGS)
    CXXFLAGS += $(PKG_CFLAGS)
endif

# ============================================================================
# TARGETS
# ============================================================================

.PHONY: all vulkan metal dx opengl debug release test clean info

all: $(LIB)
	@echo "[HAL] GPU $(RENDERER) ready: $<"

vulkan:
	$(MAKE) RENDERER=vulkan

metal:
	$(MAKE) RENDERER=metal

dx:
	$(MAKE) RENDERER=dx

opengl:
	$(MAKE) RENDERER=opengl

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
	@echo "[HAL] GPU Tests not implemented"

clean:
	@echo "  CLEAN   hal/gpu/$(RENDERER)"
	$(Q)rm -rf $(MODULE_DIR)

info:
	@echo "=== HAL GPU Build Info ==="
	@echo "System:    $(UNAME)"
	@echo "Backend:   $(RENDERER)"
	@echo "Output:    $(LIB)"
