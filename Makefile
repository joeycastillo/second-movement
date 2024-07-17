# Keep this first line.
GOSSAMER_PATH=gossamer

# If your firmware targets a specific board, specify it here,
# or omit it and provide it on the command line (make BOARD=foo).
BOARD=sensorwatch_pro

# TinyUSB configuration: we want one CDC interface and one MSC interface.
TINYUSB_CDC=1
TINYUSB_MSC=1

# OPTIONAL: enable debug output
# CFLAGS += -DCFG_TUSB_DEBUG=2
# CFLAGS += -DLFS_YES_TRACE

# Leave this line here.
include $(GOSSAMER_PATH)/make.mk

# Add your source files here.
SRCS += \
  ./movement.c \
  ./littlefs/lfs.c \
  ./littlefs/lfs_util.c \
  ./filesystem/filesystem.c \
  ./filesystem/mimic_fat.c \
  ./filesystem/unicode.c \
  ./filesystem/usb_msc_driver.c \
  ./shell/shell.c \
  ./shell/shell_cmd_list.c \
  ./watch-library/hardware/watch/watch.c \
  ./watch-library/hardware/watch/watch_adc.c \
  ./watch-library/hardware/watch/watch_buzzer.c \
  ./watch-library/hardware/watch/watch_deepsleep.c \
  ./watch-library/hardware/watch/watch_extint.c \
  ./watch-library/hardware/watch/watch_led.c \
  ./watch-library/hardware/watch/watch_private.c \
  ./watch-library/hardware/watch/watch_rtc.c \
  ./watch-library/hardware/watch/watch_slcd.c \
  ./watch-library/hardware/watch/watch_storage.c \
  ./watch-library/hardware/watch/watch_usb_descriptors.c \
  ./watch-library/hardware/watch/watch_usb_cdc.c \
  ./watch-library/shared/watch/watch_common_display.c \
  ./watch-library/shared/watch/watch_common_buzzer.c \

INCLUDES += \
  -I./tinyusb/src \
  -I./littlefs \
  -I./filesystem \
  -I./shell \
  -I./watch-library/hardware/watch \
  -I./watch-library/shared/watch \

# Finally, leave this line at the bottom of the file.
include $(GOSSAMER_PATH)/rules.mk
