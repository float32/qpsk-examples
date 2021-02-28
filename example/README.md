This is an example implementation of a bootloader using the QPSK decoder to
decode firmware data from an audio signal. It's intended to run on the
STM32F4 Discovery board.

The audio signal is taken from pin PC4. The board doesn't have an audio input
connector so you will need to wire it manually, taking care to couple the
signal to the [0, Vdd] range and to protect the pin from over/undervoltage.

When a QPSK signal is present, the on-board LEDs display the decoder's
activity. The blue LED toggles after each packet is received. After an
entire block's worth of packets has been received, the orange LED turns on
while the data is written to flash memory. The green LED flashes continuously
after the entire firmware image has been written. The red LED flashes to
indicate an error, at which point the decoder can be reset by pressing the
user button B1, or by resetting the microcontroller with button B2.

By default, this bootloader doesn't actually write to flash memory. It only
simulates the writes using time delays corresponding to the worst-case flash
write durations specified in the STM32F407 datasheet. You can override this
behavior and force real flash writes by holding down the user button B1
at boot.

The STM32F407 is capable of running at 168 MHz, but this bootloader sets the
clock to 32 MHz as a performance demonstration.

The makefile provides recipes to demonstrate the bootloader functionality.
From the top level directory (containing example.mk), use the following
commands.

- Completely erase the microcontroller's flash memory:

      make erase

- Compile the bootloader and download it to the microcontroller:

      make load-example

- Generate a QPSK audio file (encoded from example/data.bin which contains
  random data):

      make wav

  The file is written to build/artifact/data.wav

- After the bootloader has decoded and written the data (and having overridden
  the flash write simulation behavior described above), check the flash
  contents against the data from which the audio file was generated:

      make verify
