TARGET := test
SOURCES := \
	unit_tests/*.cpp \

TGT_DEFS :=
CPPFLAGS := -g -O0 -Wall -Wextra -iquote .
TGT_CFLAGS := $(CPPFLAGS) -std=gnu11
TGT_CXXFLAGS := $(CPPFLAGS) -std=gnu++17 -pthread -Wold-style-cast
TGT_LDLIBS := -lgtest -lgtest_main -lpthread -lz

TEST_WAV_DIR := $(TARGET_DIR)/test-wav
# [[[cog
# from unit_tests.data.encodings import GenerateEncodings
# cog.outl('TEST_WAV_FILES := \\')
# targets = []
# for e in GenerateEncodings():
#     file = '$(TEST_WAV_DIR)/decode-{}-{}-{}.wav' \
#         .format(e.carrier_frequency, e.packet_size, e.page_size)
#     cog.outl('\t' + file + '\\')
#     targets.append((file, e))
# cog.outl()
# ]]]
TEST_WAV_FILES := \
	$(TEST_WAV_DIR)/decode-8000-52-52.wav\
	$(TEST_WAV_DIR)/decode-8000-52-208.wav\
	$(TEST_WAV_DIR)/decode-8000-52-676.wav\
	$(TEST_WAV_DIR)/decode-8000-256-256.wav\
	$(TEST_WAV_DIR)/decode-8000-256-1024.wav\
	$(TEST_WAV_DIR)/decode-8000-256-3328.wav\
	$(TEST_WAV_DIR)/decode-8000-1024-1024.wav\
	$(TEST_WAV_DIR)/decode-8000-1024-4096.wav\
	$(TEST_WAV_DIR)/decode-8000-1024-13312.wav\
	$(TEST_WAV_DIR)/decode-6000-52-52.wav\
	$(TEST_WAV_DIR)/decode-6000-52-208.wav\
	$(TEST_WAV_DIR)/decode-6000-52-676.wav\
	$(TEST_WAV_DIR)/decode-6000-256-256.wav\
	$(TEST_WAV_DIR)/decode-6000-256-1024.wav\
	$(TEST_WAV_DIR)/decode-6000-256-3328.wav\
	$(TEST_WAV_DIR)/decode-6000-1024-1024.wav\
	$(TEST_WAV_DIR)/decode-6000-1024-4096.wav\
	$(TEST_WAV_DIR)/decode-6000-1024-13312.wav\
	$(TEST_WAV_DIR)/decode-4000-52-52.wav\
	$(TEST_WAV_DIR)/decode-4000-52-208.wav\
	$(TEST_WAV_DIR)/decode-4000-52-676.wav\
	$(TEST_WAV_DIR)/decode-4000-256-256.wav\
	$(TEST_WAV_DIR)/decode-4000-256-1024.wav\
	$(TEST_WAV_DIR)/decode-4000-256-3328.wav\
	$(TEST_WAV_DIR)/decode-4000-1024-1024.wav\
	$(TEST_WAV_DIR)/decode-4000-1024-4096.wav\
	$(TEST_WAV_DIR)/decode-4000-1024-13312.wav\
	$(TEST_WAV_DIR)/decode-3000-52-52.wav\
	$(TEST_WAV_DIR)/decode-3000-52-208.wav\
	$(TEST_WAV_DIR)/decode-3000-52-676.wav\
	$(TEST_WAV_DIR)/decode-3000-256-256.wav\
	$(TEST_WAV_DIR)/decode-3000-256-1024.wav\
	$(TEST_WAV_DIR)/decode-3000-256-3328.wav\
	$(TEST_WAV_DIR)/decode-3000-1024-1024.wav\
	$(TEST_WAV_DIR)/decode-3000-1024-4096.wav\
	$(TEST_WAV_DIR)/decode-3000-1024-13312.wav\

# [[[end]]]

.PHONY: tests
tests: $(TARGET_DIR)/$(TARGET)

.PHONY: check
check: $(TARGET_DIR)/$(TARGET) $(TEST_WAV_FILES)
	$<
	python3 unit_tests/test_encoder.py -v

TEST_WAV_ENCODE = \
	python3 encoder.py -s 48000 -w 0.05 -t bin -i unit_tests/data/data.bin

$(TEST_WAV_DIR):
	mkdir -p $@

# [[[cog
# for (file, e) in targets:
#     cog.outl(file + ': | $(TEST_WAV_DIR)')
#     cog.outl('\t$(TEST_WAV_ENCODE) -c {} -p {} -f {} -o $@'
#         .format(e.carrier_frequency, e.packet_size, e.page_size))
# ]]]
$(TEST_WAV_DIR)/decode-8000-52-52.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 8000 -p 52 -f 52 -o $@
$(TEST_WAV_DIR)/decode-8000-52-208.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 8000 -p 52 -f 208 -o $@
$(TEST_WAV_DIR)/decode-8000-52-676.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 8000 -p 52 -f 676 -o $@
$(TEST_WAV_DIR)/decode-8000-256-256.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 8000 -p 256 -f 256 -o $@
$(TEST_WAV_DIR)/decode-8000-256-1024.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 8000 -p 256 -f 1024 -o $@
$(TEST_WAV_DIR)/decode-8000-256-3328.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 8000 -p 256 -f 3328 -o $@
$(TEST_WAV_DIR)/decode-8000-1024-1024.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 8000 -p 1024 -f 1024 -o $@
$(TEST_WAV_DIR)/decode-8000-1024-4096.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 8000 -p 1024 -f 4096 -o $@
$(TEST_WAV_DIR)/decode-8000-1024-13312.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 8000 -p 1024 -f 13312 -o $@
$(TEST_WAV_DIR)/decode-6000-52-52.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 6000 -p 52 -f 52 -o $@
$(TEST_WAV_DIR)/decode-6000-52-208.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 6000 -p 52 -f 208 -o $@
$(TEST_WAV_DIR)/decode-6000-52-676.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 6000 -p 52 -f 676 -o $@
$(TEST_WAV_DIR)/decode-6000-256-256.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 6000 -p 256 -f 256 -o $@
$(TEST_WAV_DIR)/decode-6000-256-1024.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 6000 -p 256 -f 1024 -o $@
$(TEST_WAV_DIR)/decode-6000-256-3328.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 6000 -p 256 -f 3328 -o $@
$(TEST_WAV_DIR)/decode-6000-1024-1024.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 6000 -p 1024 -f 1024 -o $@
$(TEST_WAV_DIR)/decode-6000-1024-4096.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 6000 -p 1024 -f 4096 -o $@
$(TEST_WAV_DIR)/decode-6000-1024-13312.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 6000 -p 1024 -f 13312 -o $@
$(TEST_WAV_DIR)/decode-4000-52-52.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 4000 -p 52 -f 52 -o $@
$(TEST_WAV_DIR)/decode-4000-52-208.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 4000 -p 52 -f 208 -o $@
$(TEST_WAV_DIR)/decode-4000-52-676.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 4000 -p 52 -f 676 -o $@
$(TEST_WAV_DIR)/decode-4000-256-256.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 4000 -p 256 -f 256 -o $@
$(TEST_WAV_DIR)/decode-4000-256-1024.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 4000 -p 256 -f 1024 -o $@
$(TEST_WAV_DIR)/decode-4000-256-3328.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 4000 -p 256 -f 3328 -o $@
$(TEST_WAV_DIR)/decode-4000-1024-1024.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 4000 -p 1024 -f 1024 -o $@
$(TEST_WAV_DIR)/decode-4000-1024-4096.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 4000 -p 1024 -f 4096 -o $@
$(TEST_WAV_DIR)/decode-4000-1024-13312.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 4000 -p 1024 -f 13312 -o $@
$(TEST_WAV_DIR)/decode-3000-52-52.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 3000 -p 52 -f 52 -o $@
$(TEST_WAV_DIR)/decode-3000-52-208.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 3000 -p 52 -f 208 -o $@
$(TEST_WAV_DIR)/decode-3000-52-676.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 3000 -p 52 -f 676 -o $@
$(TEST_WAV_DIR)/decode-3000-256-256.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 3000 -p 256 -f 256 -o $@
$(TEST_WAV_DIR)/decode-3000-256-1024.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 3000 -p 256 -f 1024 -o $@
$(TEST_WAV_DIR)/decode-3000-256-3328.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 3000 -p 256 -f 3328 -o $@
$(TEST_WAV_DIR)/decode-3000-1024-1024.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 3000 -p 1024 -f 1024 -o $@
$(TEST_WAV_DIR)/decode-3000-1024-4096.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 3000 -p 1024 -f 4096 -o $@
$(TEST_WAV_DIR)/decode-3000-1024-13312.wav: | $(TEST_WAV_DIR)
	$(TEST_WAV_ENCODE) -c 3000 -p 1024 -f 13312 -o $@
# [[[end]]]

.PHONY: wav
wav: $(TEST_WAV_FILES)

define TGT_POSTCLEAN
	$(RM) -r $(TEST_WAV_DIR)
endef