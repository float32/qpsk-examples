# qpsk-examples

Implementation examples, unit tests, and simulation for
[qpsk](https://github.com/float32/qpsk).

---

## Implementation examples

There's currently a single implementation example, written for the
[STM32F4 Discovery](https://www.st.com/en/evaluation-tools/stm32f4discovery.html)
board. See `example/README.md`.


## Unit tests

A suite of unit tests for the decoder and encoder can be found in the
`unit_tests` directory. They depend on
[googletest](https://github.com/google/googletest) and
[zlib](https://www.zlib.net/). Run the tests using these commands:

    make check
    make py-check


## Simulation

There's a decoder simulation under the `sim` directory. Run it with the
command:

    make run-sim

You can look at traces of the decoder's internal signals using
[GTKWave](http://gtkwave.sourceforge.net/), for which a project file is
provided.


## Licensing

This project contains a few libraries with varying licenses.

- [**qpsk**](https://github.com/float32/qpsk)
  is licensed MIT, copyright Émilie Gillet and Tyler Coy.
- [**boilermake**](https://github.com/float32/boilermake)
  is licensed GPL-3.0, copyright Dan Moulding, Alan T. DeKok
- [**vcd-writer**](https://github.com/favorart/vcd-writer)
  is licensed MIT, copyright Kirill Golikov.
- [**CMSIS**](https://github.com/ARM-software/CMSIS_5)
  is licensed Apache-2.0, copyright Arm Limited.
- [**STM32F4 HAL**](https://github.com/STMicroelectronics/STM32CubeF4)
  is licensed BSD-3-Clause, copyright STMicroelectronics.
- Everything else is licensed MIT, copyright Tyler Coy.

---

Copyright 2021 Tyler Coy

https://www.alrightdevices.com

https://github.com/float32