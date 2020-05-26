TARGET := test
SOURCES := \
	unit_tests/*.cpp \

TGT_DEFS :=

CPPFLAGS := -g -O0 -Wall -Wextra -iquote .
TGT_CFLAGS := $(CPPFLAGS) -std=gnu11
TGT_CXXFLAGS := $(CPPFLAGS) -std=gnu++17 -pthread -Wold-style-cast

TGT_LDLIBS := -lgtest -lgtest_main -lpthread -lz

.PHONY: tests
tests: $(TARGET_DIR)/$(TARGET)

.PHONY: check
check: $(TARGET_DIR)/$(TARGET)
	$<
	python3 unit_tests/test_encoder.py
