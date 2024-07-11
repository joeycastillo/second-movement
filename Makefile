# Keep this first line.
GOSSAMER_PATH=gossamer

# If your firmware targets a specific board, specify it here,
# or omit it and provide it on the command line (make BOARD=foo).
BOARD=sensorwatch_green

# Leave this line here.
include $(GOSSAMER_PATH)/make.mk

# Add your source files here.
SRCS += \
  ./app.c \
  ./watch-library/hardware/watch/watch.c \
  ./watch-library/hardware/watch/watch_buzzer.c \
  ./watch-library/hardware/watch/watch_led.c \
  ./watch-library/hardware/watch/watch_private.c \

INCLUDES += \
  -I./watch-library/shared/watch \

# Finally, leave this line at the bottom of the file.
include $(GOSSAMER_PATH)/rules.mk
