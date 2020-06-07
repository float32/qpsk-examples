TARGET := sim
SOURCES := \
	sim/*.cpp \
	sim/vcd-writer/*.cpp \

TGT_DEFS :=
CPPFLAGS := -g -O3 -iquote .
TGT_CXXFLAGS := $(CPPFLAGS) -std=gnu++17

.PHONY: sim
sim: $(TARGET_DIR)/$(TARGET)

.PHONY: run-sim
run-sim: $(TARGET_DIR)/$(TARGET)
	mkdir -p $(TARGET_DIR)/sim-qpsk/
	$< $(TARGET_DIR)/sim-qpsk/

define TGT_POSTCLEAN
	$(RM) -r $(TARGET_DIR)/sim-qpsk
endef
