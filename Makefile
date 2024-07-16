# Keep this first line.
GOSSAMER_PATH=gossamer

# If your firmware targets a specific board, specify it here,
# or omit it and provide it on the command line (make BOARD=foo).
BOARD=sensorwatch_green

# TinyUSB configuration: we want one CDC interface.
TINYUSB_CDC=1

# Leave this line here.
include $(GOSSAMER_PATH)/make.mk

# Add your source files here.
SRCS += \
  ./app.c \
  ./watch-library/hardware/watch/watch.c \
  ./watch-library/hardware/watch/watch_buzzer.c \
  ./watch-library/hardware/watch/watch_led.c \
  ./watch-library/hardware/watch/watch_private.c \
  ./watch-library/hardware/watch/watch_slcd.c \
  ./watch-library/hardware/watch/watch_usb_descriptors.c \
  ./watch-library/hardware/watch/watch_usb_cdc.c \
  ./watch-library/shared/watch/watch_private_display.c \

INCLUDES += \
  -I./tinyusb/src \
  -I./watch-library/hardware/watch \
  -I./watch-library/shared/watch \

# Finally, leave this line at the bottom of the file.
include $(GOSSAMER_PATH)/rules.mk
