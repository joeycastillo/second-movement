Second Movement
===============

This is a work-in-progress refactor of the Movement firmware for [Sensor Watch](https://www.sensorwatch.net).


Getting dependencies
-------------------------
You will need to install [the GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads/) to build projects for the watch. If you're using Debian or Ubuntu, it should be sufficient to `apt install gcc-arm-none-eabi`.

You will need to fetch the git submodules for this repository too, with `git submodule update --init --recursive` 


Building Second Movement
----------------------------
You can build the default watch firmware with:

```
make BOARD=board_type DISPLAY=display_type
```

where `board_type` is any of:
- sensorwatch_pro
- sensorwatch_green  
- sensorwatch_red (also known as Sensor Watch Lite)
- sensorwatch_blue

and `display_type` is any of:
- classic
- custom

Optionally you can set the watch time when building the firmware using `TIMESET=minute`. 

`TIMESET` can be defined as:
- `year` = Sets the year to the PC's
- `day` = Sets the default time down to the day (year, month, day)
- `minute` = Sets the default time down to the minute (year, month, day, hour, minute)


If you'd like to modify which faces are built and included in the firmware, edit `movement_config.h`. You will get a compilation error if you enable more faces than the watch can store.

Installing firmware to the watch
----------------------------
To install the firmware onto your Sensor Watch board, plug the watch into your USB port and double tap the tiny Reset button on the back of the board. You should see the LED light up red and begin pulsing. (If it does not, make sure you didn’t plug the board in upside down). Once you see the `WATCHBOOT` drive appear on your desktop, type `make install`. This will convert your compiled program to a UF2 file, and copy it over to the watch.

If you want to do this step manually, copy `/build/firmware.uf2` to your watch. 


Emulating the firmware
----------------------------
You may want to test out changes in the emulator first. To do this, you'll need to install [emscripten](https://emscripten.org/), then run:

```
emmake make BOARD=sensorwatch_red DISPLAY=classic
python3 -m http.server -d build-sim
```

Finally, visit [watch.html](http://localhost:8000/watch.html) to see your work.
