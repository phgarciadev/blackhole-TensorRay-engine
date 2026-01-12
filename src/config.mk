# ============================================================================
# Black Hole Simulator - Build Configuration
# ============================================================================
# Included by all sub-Makefiles to ensure consistency.
# ============================================================================

# --- 1. System Detection ---
UNAME := $(shell uname -s)
ifeq ($(UNAME),Linux)
    DEFAULT_PLATFORM := wayland
    DEFAULT_RENDERER := vulkan
else ifeq ($(UNAME),Darwin)
    DEFAULT_PLATFORM := cocoa
    DEFAULT_RENDERER := metal
else
    DEFAULT_PLATFORM := win32
    DEFAULT_RENDERER := vulkan
endif

# User Overrides
PLATFORM ?= $(DEFAULT_PLATFORM)
RENDERER ?= $(DEFAULT_RENDERER)

# --- 2. Directories ---
# Helper to locate root from anywhere inside src/
# If ROOT_DIR is not defined by parent, try to guess, but ideally parent sets it.
# We assume this file is in src/config.mk
# So if included from src/sim/Makefile, logic needs to be robust.
# Better strategy: Rely on ROOT_DIR being passed or relative path.
# We will define BUILD_DIR relative to the location where make is run, 
# assuming standard depth or ROOT_DIR variable.

# If ROOT_DIR is defined, use it. Otherwise assume we are at root if Makefile exists.
ifeq ($(ROOT_DIR),)
    # Fallback/Default
    BUILD_DIR := build
else
    BUILD_DIR := $(ROOT_DIR)/build
endif

# --- 3. Compiler & Tools ---
CC ?= clang
CXX ?= clang++
AR ?= llvm-ar
LD := $(CC)

# --- 4. Global Flags ---
# Kernel-style strictness
CFLAGS_COMMON := -std=c11 -Wall -Wextra -Werror
CFLAGS_COMMON += -Wno-unused-parameter -Wno-missing-field-initializers
# Allow unused functions for now as we have stubs
CFLAGS_COMMON += -Wno-unused-function 

# Include src/ for all modules
CFLAGS_COMMON += -I$(abspath $(dir $(lastword $(MAKEFILE_LIST))))

# C++ Flags
CXXFLAGS_COMMON := -std=c++17 -Wall -Wextra -Werror -Wno-unused-parameter
CXXFLAGS_COMMON += -I$(abspath $(dir $(lastword $(MAKEFILE_LIST))))

# --- 5. Debug / Release ---
ifneq ($(V),1)
    Q := @
endif

ifeq ($(DEBUG),1)
    CFLAGS_OPT := -g -O0 -DBHS_DEBUG -fsanitize=address,undefined
    CXXFLAGS_OPT := -g -O0 -DBHS_DEBUG -fsanitize=address,undefined
    LDFLAGS_OPT := -fsanitize=address,undefined
else ifeq ($(RELEASE),1)
    CFLAGS_OPT := -O3 -DNDEBUG -flto
    CXXFLAGS_OPT := -O3 -DNDEBUG -flto
    LDFLAGS_OPT := -flto
else
    # Default development mode
    CFLAGS_OPT := -g -O0 -DBHS_DEBUG
    CXXFLAGS_OPT := -g -O0 -DBHS_DEBUG
    LDFLAGS_OPT := 
endif

# Combine
CFLAGS   := $(CFLAGS_COMMON) $(CFLAGS_OPT) $(EXTRA_CFLAGS)
CXXFLAGS := $(CXXFLAGS_COMMON) $(CXXFLAGS_OPT) $(EXTRA_CFLAGS)

# --- 6. Helper Functions ---
# (Can be added here if needed)

# --- 7. Linker Flags (Moved from Root Makefile) ---
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
