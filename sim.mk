TARGET := sim
SOURCES := \
	sim/*.cpp \
	sim/vcd-writer/*.cpp \

TGT_DEFS :=
CPPFLAGS := -g -O3 -iquote .
TGT_CXXFLAGS := $(CPPFLAGS) -std=gnu++17

VCD_FILE := $(TARGET_DIR)/sim-qpsk.vcd

.PHONY: sim
sim: $(TARGET_DIR)/$(TARGET)

.PHONY: $(VCD_FILE)
$(VCD_FILE): $(TARGET_DIR)/$(TARGET)
	$< $@ unit_tests/data/data.bin

.PHONY: run-sim
run-sim: $(VCD_FILE)

define TGT_POSTCLEAN
	$(RM) $(VCD_FILE)
endef
