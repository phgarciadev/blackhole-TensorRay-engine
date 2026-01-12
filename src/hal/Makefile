ROOT_DIR := ../..
include $(ROOT_DIR)/src/config.mk

.PHONY: all os gpu clean

all: os gpu
	@echo ">>> HAL Built."

os:
	$(MAKE) -C os PLATFORM=$(PLATFORM)

gpu:
	$(MAKE) -C gpu RENDERER=$(RENDERER)

clean:
	$(MAKE) -C os clean
	$(MAKE) -C gpu clean
