# MIT License
#
# Copyright 2021 Tyler Coy
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

STACK_SIZE := 256
BOOTLOADER_SIZE := 0x4000
SAMPLE_RATE := 48000
SYMBOL_RATE := 8000
PACKET_SIZE := 256
BLOCK_SIZE := 0x4000
CRC_SEED := 420

TARGET := example.elf
SOURCES := example/*.cpp example/hal/*.c
LD_SCRIPT := example/app.ld

TGT_CC := arm-none-eabi-gcc
TGT_CXX := arm-none-eabi-g++

TGT_DEFS := \
	STM32F407xx \
	HSE_VALUE=8000000 \
	USE_FULL_ASSERT \
	USE_FULL_LL_DRIVER \
	STACK_SIZE=$(STACK_SIZE) \
	BOOTLOADER_SIZE=$(BOOTLOADER_SIZE) \
	SAMPLE_RATE=$(SAMPLE_RATE) \
	SYMBOL_RATE=$(SYMBOL_RATE) \
	PACKET_SIZE=$(PACKET_SIZE) \
	BLOCK_SIZE=$(BLOCK_SIZE) \
	CRC_SEED=$(CRC_SEED) \

ARCHFLAGS := \
	-mthumb \
	-mthumb-interwork \
	-mcpu=cortex-m4 \
	-mfpu=fpv4-sp-d16 \
	-mfloat-abi=hard \
	-mfp16-format=alternative \

WARNFLAGS := \
	-Wall \
	-Wextra \
	-Wundef \

OPTFLAGS := \
	-ffunction-sections \
	-fdata-sections \
	-fshort-enums \
	-finline-functions-called-once \
	-ffast-math \
	-fsingle-precision-constant \
	-finline-functions \

TGT_INCDIRS  := . example example/hal
TGT_CFLAGS   := -Os -g $(ARCHFLAGS) $(OPTFLAGS) $(WARNFLAGS) -std=c11
TGT_CXXFLAGS := -Os -g $(ARCHFLAGS) $(OPTFLAGS) $(WARNFLAGS) -std=c++17 \
	-fno-exceptions -fno-rtti -Wno-register
TGT_LDLIBS   := -lm -lc -lgcc
TGT_LDFLAGS  := $(ARCHFLAGS) \
	-Wl,--gc-sections \
	-specs=nosys.specs \
	-specs=nano.specs \
	-T$(LD_SCRIPT) \
	-Wl,--defsym,STACK_SIZE=$(STACK_SIZE) \
	-Wl,--defsym,BOOTLOADER_SIZE=$(BOOTLOADER_SIZE) \

.PHONY: example
example: $(TARGET_DIR)/$(TARGET)

OPENOCD_CMD := openocd -c "debug_level 1" -f board/stm32f4discovery.cfg

.PHONY: load-example
load-example: $(TARGET_DIR)/$(TARGET)
	$(OPENOCD_CMD) -c "program $< verify reset exit"

WAV_FILE := $(TARGET_DIR)/data.wav

.PHONY: wav
wav: example/data.bin | $(TARGET_DIR)
	python3 qpsk/encoder.py \
		-s $(SAMPLE_RATE) -y $(SYMBOL_RATE) -b $(BLOCK_SIZE) \
		-w 410 -f 16K:500:4 64K:1100:1 128K:2000:7 -x 0x08000000 \
		-a +$(BOOTLOADER_SIZE) -p $(PACKET_SIZE) -e $(CRC_SEED) \
		-i $< -o $(WAV_FILE)

TGT_POSTCLEAN := $(RM) $(WAV_FILE)

define TGT_POSTMAKE
@echo 'BINARY SIZE:' \
	$$(arm-none-eabi-size $(TARGET_DIR)/$(TARGET) | tail -n1 | \
	awk '{ printf "0x%05X %d", $$1+$$2, $$1+$$2 }')
@echo 'RAM USAGE:  ' \
	$$(arm-none-eabi-size $(TARGET_DIR)/$(TARGET) | tail -n1 | \
	awk '{ printf "0x%05X %d", $$2+$$3, $$2+$$3 }')
endef

.PHONY: example-sym
example-sym: $(TARGET_DIR)/$(TARGET)
	arm-none-eabi-nm -CnS $< | less

.PHONY: example-top
example-top: $(TARGET_DIR)/$(TARGET)
	arm-none-eabi-nm -CrS --size-sort $< | less

.PHONY: verify
verify:
	$(OPENOCD_CMD) -c init \
		-c "flash verify_bank 0 example/data.bin $(BOOTLOADER_SIZE)" -c exit

.PHONY: erase
erase:
	$(OPENOCD_CMD) -c init -c halt -c "flash erase_sector 0 0 last" \
		-c reset -c exit

.PHONY: openocd
openocd:
	$(OPENOCD_CMD) -c init -c reset -c halt

.PHONY: debug
debug: $(TARGET_DIR)/$(TARGET)
	@if ! nc -z localhost 3333; then \
		echo "\n\t[Error] OpenOCD is not running! \
			Start it with: 'make openocd'\n"; exit 1; \
	else \
		gdb-multiarch -tui -ex "target extended localhost:3333" \
			-ex "monitor arm semihosting enable" \
			-ex "monitor reset halt" \
			-ex "load" \
			-ex "monitor reset init" \
			$(GDBFLAGS) $<; \
	fi
