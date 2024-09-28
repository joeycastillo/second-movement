# Keep this first line.
GOSSAMER_PATH=gossamer

# If your firmware targets a specific board, specify it here,
# or omit it and provide it on the command line (make BOARD=foo).
BOARD=sensorwatch_pro

TINYUSB_CDC=1

# Leave this line here.
include $(GOSSAMER_PATH)/make.mk

# this is a hack and does not currently work; we need to integrate a bit of
# emscripten support into gossamer to actually build for the simulator
ifdef EMSCRIPTEN
BUILD = ./build-sim
CC = emcc
all: $(BUILD)/$(BIN).html
$(BUILD)/$(BIN).html: $(OBJS)
	@echo HTML $@
	@$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@ \
		-s ASYNCIFY=1 \
		-s EXPORTED_RUNTIME_METHODS=lengthBytesUTF8,printErr \
		-s EXPORTED_FUNCTIONS=_main \
		--shell-file=$(TOP)/watch-library/simulator/shell.html
endif

# Add your include directories here.
INCLUDES += \
  -I./ \
  -I./tinyusb/src \
  -I./littlefs \
  -I./utz \
  -I./filesystem \
  -I./shell \
  -I./watch-library/shared/watch \
  -I./watch-library/hardware/watch \
  -I./watch-faces/clock \
  -I./watch-faces/complication \
  -I./watch-faces/settings \

# Add your source files here.
SRCS += \
  ./littlefs/lfs.c \
  ./littlefs/lfs_util.c \
  ./filesystem/filesystem.c \
  ./utz/utz.c \
  ./utz/zones.c \
  ./shell/shell.c \
  ./shell/shell_cmd_list.c \
  ./watch-library/shared/watch/watch_common_buzzer.c \
  ./watch-library/shared/watch/watch_common_display.c \
  ./watch-library/shared/watch/watch_utility.c \
  ./watch-library/hardware/watch/watch.c \
  ./watch-library/hardware/watch/watch_adc.c \
  ./watch-library/hardware/watch/watch_deepsleep.c \
  ./watch-library/hardware/watch/watch_extint.c \
  ./watch-library/hardware/watch/watch_gpio.c \
  ./watch-library/hardware/watch/watch_private.c \
  ./watch-library/hardware/watch/watch_rtc.c \
  ./watch-library/hardware/watch/watch_slcd.c \
  ./watch-library/hardware/watch/watch_storage.c \
  ./watch-library/hardware/watch/watch_tcc.c \
  ./watch-library/hardware/watch/watch_usb_descriptors.c \
  ./watch-library/hardware/watch/watch_usb_cdc.c \

include watch-faces.mk

SRCS += \
  ./movement.c \

# Finally, leave this line at the bottom of the file.
include $(GOSSAMER_PATH)/rules.mk
