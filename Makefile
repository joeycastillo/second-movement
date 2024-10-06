# Keep this first line.
GOSSAMER_PATH=gossamer

# Which board are we building for?
BOARD=sensorwatch_red

# Which screen are we building for?
DISPLAY=CLASSIC

# Support USB features?
TINYUSB_CDC=1

# Leave this line here.
include $(GOSSAMER_PATH)/make.mk

ifdef EMSCRIPTEN
all: $(BUILD)/$(BIN).elf $(BUILD)/$(BIN).html
$(BUILD)/$(BIN).html: $(OBJS)
	@echo HTML $@
	@$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@ \
		-s ASYNCIFY=1 \
		-s EXPORTED_RUNTIME_METHODS=lengthBytesUTF8,printErr \
		-s EXPORTED_FUNCTIONS=_main \
		--shell-file=./watch-library/simulator/shell.html
endif

# Add your include directories here.
INCLUDES += \
  -I./ \
  -I./tinyusb/src \
  -I./littlefs \
  -I./utz \
  -I./filesystem \
  -I./shell \
  -I./movement/lib/sunriset \
  -I./watch-library/shared/watch \
  -I./watch-faces/clock \
  -I./watch-faces/complication \
  -I./watch-faces/demo \
  -I./watch-faces/sensor \
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
  ./movement/lib/sunriset/sunriset.c \
  ./watch-library/shared/watch/watch_common_buzzer.c \
  ./watch-library/shared/watch/watch_common_display.c \
  ./watch-library/shared/watch/watch_utility.c \

ifdef EMSCRIPTEN

INCLUDES += \
  -I./watch-library/simulator/watch \

SRCS += \
  ./watch-library/simulator/watch/watch.c \
  ./watch-library/simulator/watch/watch_adc.c \
  ./watch-library/simulator/watch/watch_deepsleep.c \
  ./watch-library/simulator/watch/watch_extint.c \
  ./watch-library/simulator/watch/watch_gpio.c \
  ./watch-library/simulator/watch/watch_private.c \
  ./watch-library/simulator/watch/watch_rtc.c \
  ./watch-library/simulator/watch/watch_slcd.c \
  ./watch-library/simulator/watch/watch_storage.c \
  ./watch-library/simulator/watch/watch_tcc.c \

else

INCLUDES += \
  -I./watch-library/hardware/watch \

SRCS += \
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

endif

include watch-faces.mk

SRCS += \
  ./movement.c \

# Finally, leave this line at the bottom of the file.
include $(GOSSAMER_PATH)/rules.mk
