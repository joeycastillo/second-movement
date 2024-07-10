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

# Finally, leave this line at the bottom of the file.
include $(GOSSAMER_PATH)/rules.mk
