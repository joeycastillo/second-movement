# Keep this first line.
GOSSAMER_PATH=gossamer

# If your firmware targets a specific board, specify it here,
# or omit it and provide it on the command line (make BOARD=foo).
BOARD=sensorwatch_green

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

# Add your source files here.
SRCS += \
  ./app.c \

# Finally, leave this line at the bottom of the file.
include $(GOSSAMER_PATH)/rules.mk
