# Flash Size Audit - 2026-03-04

## Build Configurations

### With Phase Engine Enabled
```
   text	   data	    bss	    dec	    hex	filename
 153532	   2812	   7028	 163372	  27e2c	build/firmware.elf
```

**Flash usage (text + data):** 156,344 bytes (152.7 KB)  
**RAM usage (data + bss):** 9,840 bytes (9.6 KB)

### Without Phase Engine
```
   text	   data	    bss	    dec	    hex	filename
 138316	   2804	   5188	 146308	  23b84	build/firmware.elf
```

**Flash usage (text + data):** 141,120 bytes (137.8 KB)  
**RAM usage (data + bss):** 7,992 bytes (7.8 KB)

## Phase Engine Cost

**Flash overhead:** 15,224 bytes (~14.9 KB) - 10.8% increase  
**RAM overhead:** 1,848 bytes (~1.8 KB) - 23.1% increase

## Flash Usage Analysis

- **Total available:** 256 KB (262,144 bytes)
- **Current usage (with Phase Engine):** 156,344 bytes = **59.6%**
- **Headroom:** 105,800 bytes = **40.4% (~103 KB)**

## Status: ✅ COMFORTABLE

Flash usage is well under 60%, providing plenty of room for:
- Future features and face additions
- Debug builds (typically 10-20% larger)
- Experimental features

## Recommendations

1. **Flash headroom is healthy** - No optimization needed at this time
2. **Phase Engine overhead is acceptable** - ~15KB for full circadian tracking
3. **Safe to proceed with flash** - 40%+ headroom provides good safety margin
4. **Monitor on future additions** - Track flash usage when adding new faces

## Build Configuration

- **Board:** sensorwatch_blue
- **Display:** classic
- **Compiler:** arm-none-eabi-gcc (Homebrew GCC 10.3-2021.10)
- **Optimization:** Release build (-Os)

---

*Generated 2026-03-04*
