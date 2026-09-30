# Keep this first line.
GOSSAMER_PATH=gossamer

# Which board are we building for? Commented out to force a choice when building.
# Options are:
# - sensorwatch_pro
# - sensorwatch_green
# - sensorwatch_red (also known as Sensor Watch Lite)
# - sensorwatch_blue
# BOARD=sensorwatch_pro

# Set this to the type of display in your watch: classic or custom. Commented out to force a choice when building.
# DISPLAY=classic

# End of user configurable options.

# Support USB features?
TINYUSB_CDC=1

# Now we're all set to include gossamer's make rules.
include $(GOSSAMER_PATH)/make.mk

# Don't add gossamer's rtc.c since we are using our own rtc32.c
SRCS := $(filter-out $(GOSSAMER_PATH)/peripherals/rtc.c,$(SRCS))

CFLAGS+=-D_POSIX_C_SOURCE=200112L

define n


endef

# Don't require BOARD or DISPLAY for `make clean` or `make install`
ifeq (,$(filter clean,$(MAKECMDGOALS)))
  ifeq (,$(filter install,$(MAKECMDGOALS)))
    ifndef BOARD
      $(error Build failed: BOARD not defined. Use one of the four options below, depending on your hardware:$n$n    make BOARD=sensorwatch_red DISPLAY=display_type$n    make BOARD=sensorwatch_blue DISPLAY=display_type$n    make BOARD=sensorwatch_pro DISPLAY=display_type$n$n)
    endif
  endif

  ifeq (,$(filter install,$(MAKECMDGOALS)))
    ifndef DISPLAY
      $(error Build failed: DISPLAY not defined. Use one of the options below, depending on your hardware:$n$n    make BOARD=board_type DISPLAY=classic$n    make BOARD=board_type DISPLAY=custom$n$n)
    else
      ifeq ($(DISPLAY), custom)
        DEFINES += -DFORCE_CUSTOM_LCD_TYPE
      else ifeq ($(DISPLAY), classic)
        DEFINES += -DFORCE_CLASSIC_LCD_TYPE
      else ifeq ($(DISPLAY), autodetect)
        $(warning WARNING: LCD autodetection is experimental and not reliable! We suggest specifying DISPLAY=classic or DISPLAY=custom for reliable operation.)
      else
        $(error Build failed: invalid DISPLAY type. Use one of the options below, depending on your hardware:$n$n    make BOARD=board_type DISPLAY=classic$n    make BOARD=board_type DISPLAY=custom$n$n)
      endif
    endif
  endif
endif

ifdef NOSLEEP
    DEFINES += -DMOVEMENT_LOW_ENERGY_MODE_FORBIDDEN
endif

# Emscripten targets are now handled in rules.mk in gossamer

# Add your include directories here.
INCLUDES += \
  -I./ \
  -I. \
  -I./tinyusb/src \
  -I./littlefs \
  -I./utz \
  -I./filesystem \
  -I./shell \
  -I./lib/sunriset \
  -I./lib/sha1 \
  -I./lib/sha256 \
  -I./lib/sha512 \
  -I./lib/base32 \
  -I./lib/TOTP \
  -I./lib/chirpy_tx \
  -I./lib/base64 \
  -I./lib/embedded_pedometer \
  -I./watch-library/shared/watch \
  -I./watch-library/shared/driver \
  -I./watch-faces/clock \
  -I./watch-faces/complication \
  -I./watch-faces/demo \
  -I./watch-faces/sensor \
  -I./watch-faces/settings \
  -I./watch-faces/io \

# Add your source files here.
SRCS += \
  ./dummy.c \
  ./littlefs/lfs.c \
  ./littlefs/lfs_util.c \
  ./filesystem/filesystem.c \
  ./utz/utz.c \
  ./utz/zones.c \
  ./shell/shell.c \
  ./shell/shell_cmd_list.c \
  ./lib/sunriset/sunriset.c \
  ./lib/base32/base32.c \
  ./lib/TOTP/sha1.c \
  ./lib/TOTP/sha256.c \
  ./lib/TOTP/sha512.c \
  ./lib/TOTP/TOTP.c \
  ./lib/chirpy_tx/chirpy_tx.c \
  ./lib/base64/base64.c \
  ./lib/embedded_pedometer/count_steps.c \
  ./watch-library/shared/driver/thermistor_driver.c \
  ./watch-library/shared/watch/watch_common_buzzer.c \
  ./watch-library/shared/watch/watch_common_display.c \
  ./watch-library/shared/watch/watch_utility.c \


SRCS += ./watch-library/shared/driver/lis2dw.c

ifdef EMSCRIPTEN

INCLUDES += \
  -I./watch-library/simulator/watch \

SRCS += \
  ./watch-library/simulator/watch/watch.c \
  ./watch-library/simulator/watch/watch_adc.c \
  ./watch-library/simulator/watch/watch_deepsleep.c \
  ./watch-library/simulator/watch/watch_extint.c \
  ./watch-library/simulator/watch/watch_gpio.c \
  ./watch-library/simulator/watch/watch_i2c.c \
  ./watch-library/simulator/watch/watch_private.c \
  ./watch-library/simulator/watch/watch_rtc.c \
  ./watch-library/simulator/watch/watch_slcd.c \
  ./watch-library/simulator/watch/watch_spi.c \
  ./watch-library/simulator/watch/watch_storage.c \
  ./watch-library/simulator/watch/watch_tcc.c \
  ./watch-library/simulator/watch/watch_uart.c \

else

INCLUDES += \
  -I./watch-library/hardware/watch \

SRCS += \
  ./watch-library/hardware/watch/rtc32.c \
  ./watch-library/hardware/watch/watch.c \
  ./watch-library/hardware/watch/watch_adc.c \
  ./watch-library/hardware/watch/watch_deepsleep.c \
  ./watch-library/hardware/watch/watch_extint.c \
  ./watch-library/hardware/watch/watch_gpio.c \
  ./watch-library/hardware/watch/watch_i2c.c \
  ./watch-library/hardware/watch/watch_private.c \
  ./watch-library/hardware/watch/watch_rtc.c \
  ./watch-library/hardware/watch/watch_slcd.c \
  ./watch-library/hardware/watch/watch_spi.c \
  ./watch-library/hardware/watch/watch_storage.c \
  ./watch-library/hardware/watch/watch_tcc.c \
  ./watch-library/hardware/watch/watch_uart.c \
  ./watch-library/hardware/watch/watch_usb_descriptors.c \
  ./watch-library/hardware/watch/watch_usb_cdc.c \

endif

include watch-faces.mk

SRCS += \
  ./movement.c \

# Finally, leave this line at the bottom of the file.
include $(GOSSAMER_PATH)/rules.mk
