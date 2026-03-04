# Second-Movement Emulator Build Report
**Date:** 2026-02-18  
**Branch:** main (commit a0fb81f)  
**Board:** sensorwatch_blue  
**Display:** classic  

## ✅ Build Status: SUCCESS

The emulator build completed successfully using Docker + Emscripten.

### Build Artifacts
- **Location:** `build-sim/`
- **Files generated:**
  - `firmware.html` (160 KB) - Main emulator page
  - `firmware.wasm` (460 KB) - WebAssembly binary
  - `firmware.js` (100 KB) - JavaScript loader
  - `firmware.elf` (83 KB) - ELF debug symbols

### Running the Emulator
To run the emulator locally:
```bash
cd ~/repos/second-movement/build-sim
python3 -m http.server 8000
# Open http://localhost:8000/firmware.html in browser
```

## 🎯 Key Faces Verification

### ✅ Successfully Compiled:
- **sleep_tracker_face** ✓
- **circadian_score_face** ✓
- **comms_rx** (optical time sync) ✓
- **smart_alarm_face** ✓

### ❌ oracle_face: NOT ON MAIN YET
- PR #29 exists but **has not been merged to main**
- The face is available on branch `feature/oracle-face`
- Commit 708015b shows the merge but is not an ancestor of main
- Main branch latest: a0fb81f (PR #30 - pages workflow fix)

## ⚠️ Build Warnings Summary

### Minor Format Warnings (non-critical):
- **filesystem.c**: 4 format warnings (`long` vs `int32_t`)
- **fast_stopwatch_face.c**: 3 format warnings
- **days_since_face.c**: 2 format warnings
- **totp_face.c**: 1 format warning
- **totp_lfs_face.c**: 2 format warnings
- **peek_memory_face.c**: 1 format warning
- **rtccount_face.c**: 4 format warnings
- **kitchen_conversions_face.c**: 2 format warnings
- **ke_decimal_time_face.c**: 1 format warning

### Implicit Conversion Warnings:
- **temperature_display_face.c**: `0xFFFFFFFF` to float conversion
- **temperature_logging_face.c**: `0xFFFFFFFF` to float conversion
- **nanosec_face.c**: `0xFFFFFFFF` to float conversion

### Library Warnings:
- **TinyUSB**: `TUP_DCD_ENDPOINT_MAX` undefined (appears 4 times)
- **Emscripten**: JS library `$printErr` deprecated

## 📊 Build Statistics
- **Total warnings:** ~30 (all non-critical)
- **Compilation errors:** 0
- **Faces compiled:** 60+ watch faces
- **Build time:** ~90 seconds (including Docker image pull)

## 🔧 Build Method
```bash
docker run --rm \
  -v $(pwd):/src \
  -w /src \
  emscripten/emsdk \
  emmake make BOARD=sensorwatch_blue DISPLAY=classic
```

## 📝 Notes
- ARM toolchain (`arm-none-eabi-gcc`) not available on this Mac → hardware build not tested
- Emulator build is the primary testing method for this environment
- All requested faces (except oracle_face) successfully compiled and are included in the emulator
- Format warnings are cosmetic (printf format specifiers) and do not affect functionality
