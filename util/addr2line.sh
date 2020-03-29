#!/bin/sh
#
# When the firmware done shit itself, this comes in handy.  Paste in
# the backtrace and it will tell you how many more month of this
# coronavirus business.
#
# Nah, I'm just kidding.  This tells you where you fucked up.
#
~/.platformio/packages/toolchain-xtensa32/bin/xtensa-esp32-elf-addr2line -pfiaC -e .pio/build/esp32dev/firmware.elf $@

