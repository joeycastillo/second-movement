Second Movement
===============

This is a work-in-progress refactor of the Movement firmware for [Sensor Watch](https://www.sensorwatch.net).


Getting dependencies
-------------------------
You will need to install [the GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads/) to build projects for the watch. If you're using Debian or Ubuntu, it should be sufficient to `apt install gcc-arm-none-eabi`.

You will need to fetch the git submodules for this repository too, with `git submodule update --init --recursive` 

If you want your IDE's LSP to find its way around this repo, you might need to create the files `compile_commands.json` and `.clangd`:

For `compile_commands.json`:
```
apt install bear
bear -- make BOARD=sensorwatch_pro DISPLAY=classic # choose your desired board and display
```

For `.clangd` create a file at the root of this repo with this content:
```
CompileFlags:
  Add:
    - --target=arm-none-eabi
    - -isystem
    - /usr/include/newlib
```

Note: The path to newlib might vary on your system. Above example is for Debian.
Fedora should have it in package `arm-none-eabi-newlib` in `/usr/arm-none-eabi/include`

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


If you'd like to customize the settings of your watch, e.g. the included faces, create a (gitignored) file named `movement_config_local.h`
and add to it `#define`'s from `movement_config.h`. Example file: 

```
#include "movement_faces.h"

#define WATCH_FACES
const watch_face_t watch_faces[] = {
    clock_face,
    alarm_face,
    fast_stopwatch_face,
    totp_face,
    settings_face,
};

#define MOVEMENT_SECONDARY_FACE_INDEX (MOVEMENT_NUM_FACES 0)

#define TOTP_CREDS \
    CREDENTIAL(CM, "CHANGEME", SHA1, 30),
```

Note: You will get a compilation error if you enable more faces than the watch can store.

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

Finally, visit [firmware.html](http://localhost:8000/firmware.html) to see your work.
