# Additional Iconic Alarm Tunes Research

## Target Tunes (Iconic & Recognizable)

### 1. Nokia Ringtone (Grande Valse)
**Why:** THE classic ringtone, instantly recognizable
**Notes:** E5 D5 F#4 G#4, E5 D5 B4 C#5, D5 C5 E4 F#4, G#4 E4 E5 E5
**Difficulty:** Easy - simple melody

### 2. Tetris Theme (Korobeiniki)
**Why:** Iconic game music, upbeat wake-up energy
**Notes:** E5 B4 C5 D5, C5 B4 A4 A4, C5 E5 D5 C5, B4 C5 D5 E5, C5 A4 A4
**Difficulty:** Medium - longer sequence

### 3. Star Wars Imperial March
**Why:** Powerful, commanding wake-up
**Notes:** G4 G4 G4, Eb4 Bb4, G4 Eb4 Bb4 G4
**Difficulty:** Easy - simple rhythm

### 4. Pokémon Center Healing
**Why:** Nostalgic, positive association
**Notes:** C6 E6 G6 C7 E7 G6 E6 C6
**Difficulty:** Easy - ascending/descending

### 5. Final Fantasy Victory Fanfare
**Why:** Achievement sound, motivating
**Notes:** C5 C5 C5 C5, Ab4 Bb4, C5 Bb4 C5
**Difficulty:** Easy - short and punchy

### 6. Nyan Cat
**Why:** Internet culture, quirky
**Notes:** F#5 G#5 D#5 D#5 A#4, D5 D#5 C#5 B4, B4 C#5 D5 D5 C#5 A#4 B4 C#5 B4 etc.
**Difficulty:** Hard - fast, long sequence

### 7. Never Gonna Give You Up (Rickroll)
**Why:** Meme culture icon
**Notes:** D5 E5 F#5 D5 E5, F#5 E5 A4 A4
**Difficulty:** Medium

### 8. Legend of Zelda Main Theme
**Why:** Gaming icon, adventurous
**Notes:** Bb4 F5 Bb4 Bb4, C6 D6 Eb6 F6
**Difficulty:** Medium

### 9. Megalovanial (Undertale)
**Why:** Modern gaming culture
**Notes:** D4 D4 D5 A4 Ab4 G4 F4, D4 F4 G4
**Difficulty:** Medium - recognizable opening

### 10. Halo Theme
**Why:** Epic, dramatic
**Notes:** E5 E5 E5 C5 E5, G5 E5 C5 E5
**Difficulty:** Medium

### 11. iPhone Radar
**Why:** Modern phone culture
**Notes:** G6 C7 E7, E7 D7 C7 G6
**Difficulty:** Easy

### 12. Toreador March (FNAF)
**Why:** Gaming, tense energy
**Notes:** D5 D5 D5 D5 A5 A5, D5 D5 D5 D5 A5
**Difficulty:** Easy - repetitive

## Recommended Priority (Top 5 to add first)

1. **Nokia Ringtone** - Universal classic
2. **Tetris Theme** - Upbeat, gaming culture
3. **Star Wars Imperial March** - Commanding, powerful
4. **Pokémon Center** - Positive, nostalgic
5. **Final Fantasy Victory** - Short, motivating

## Note Format Reference
From existing firmware:
```
{ id:'SIGNAL_TUNE_NAME', name:'Display Name',
  notes:[['NOTE',ticks],['REST',ticks], ...] }
```

Where:
- NOTE = C4, D5, E6, F#G5 (enharmonic), REST, etc.
- ticks = duration (1 tick ≈ 16ms)
- Typical range: 5-20 ticks per note
