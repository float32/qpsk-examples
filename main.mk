BUILD_DIR := build
TARGET_DIR := $(BUILD_DIR)/artifact

# ------------------------------------------------------------------------------
# Targets
# ------------------------------------------------------------------------------

SUBMAKEFILES := test.mk sim.mk example.mk

.DEFAULT_GOAL := tests

.PHONY: cog
cog:
	@cog -r inc/carrier_rejection_filter.h
	@cog -r inc/util.h
	@cog -r unit_tests/test_decoder.cpp
