// ============================================================
// CONSTANTS
// ============================================================

export const REPO = 'dlorp/second-movement';
export const WORKFLOW_ID = 'custom-build.yml';
export const COOLDOWN_MS = 90 * 1000;
export const POLL_INTERVAL_MS = 5000;

export const BOARDS = [
    { id: 'red', name: 'RED' },
    { id: 'green', name: 'GRN' },
    { id: 'blue', name: 'BLU' },
    { id: 'pro', name: 'PRO' }
];

export const DISPLAYS = [
    { id: 'classic', name: 'CLASSIC' },
    { id: 'custom', name: 'CUSTOM' }
];

export const BRANCHES = [
    { id: 'main', name: 'MAIN', description: 'Stable release branch' },
    { id: 'phase4bc-playlist-dispatch', name: 'PHASE4BC', description: 'Dogfooding: Phase 4E/4F + LED fade' }
];

export const VALID_BOARDS = BOARDS.map(b => b.id);
export const VALID_DISPLAYS = DISPLAYS.map(d => d.id);
export const VALID_BRANCHES = BRANCHES.map(b => b.id);

export const VALID_SIGNAL_TUNES = [
    'SIGNAL_TUNE_DEFAULT',
    'SIGNAL_TUNE_ZELDA_SECRET',
    'SIGNAL_TUNE_MARIO_THEME',
    'SIGNAL_TUNE_MGS_CODEC',
    'SIGNAL_TUNE_KIM_POSSIBLE',
    'SIGNAL_TUNE_POWER_RANGERS',
    'SIGNAL_TUNE_LAYLA',
    'SIGNAL_TUNE_HARRY_POTTER_SHORT',
    'SIGNAL_TUNE_HARRY_POTTER_LONG',
    'SIGNAL_TUNE_JURASSIC_PARK',
    'SIGNAL_TUNE_EVANGELION',
    'SIGNAL_TUNE_NOKIA',
    'SIGNAL_TUNE_TETRIS',
    'SIGNAL_TUNE_IMPERIAL',
    'SIGNAL_TUNE_POKEMON',
    'SIGNAL_TUNE_VICTORY',
    'SIGNAL_TUNE_TRITONE',
    'SIGNAL_TUNE_SOSUMI',
    'SIGNAL_TUNE_OLDPHONE',
    'SIGNAL_TUNE_MGS_ALERT',
    'SIGNAL_TUNE_SONIC',
    'SIGNAL_TUNE_PACMAN',
    'SIGNAL_TUNE_WINERROR'
];

// Note frequencies (Hz) — from BUZZER_NOTE_* in watch_tcc.h
export const NOTE_FREQUENCIES = {
    A1:55, Bb1:58.27, B1:61.74,
    C2:65.41, Db2:69.30, D2:73.42, Eb2:77.78, E2:82.41, F2:87.31, Gb2:92.50, G2:98, Ab2:103.83,
    A2:110, Bb2:116.54, B2:123.47,
    C3:130.81, Db3:138.59, D3:146.83, Eb3:155.56, E3:164.81, F3:174.61, Gb3:185, G3:196, Ab3:207.65,
    A3:220, Bb3:233.08, B3:246.94,
    C4:261.63, Db4:277.18, D4:293.66, Eb4:311.13, E4:329.63, F4:349.23, Gb4:369.99, G4:392, Ab4:415.30,
    A4:440, Bb4:466.16, B4:493.88,
    C5:523.25, Db5:554.37, D5:587.33, Eb5:622.25, E5:659.25, F5:698.46, Gb5:739.99, G5:783.99, Ab5:830.61,
    A5:880, Bb5:932.33, B5:987.77,
    C6:1046.50, Db6:1108.73, D6:1174.66, Eb6:1244.51, E6:1318.51, F6:1396.91, Gb6:1479.98, G6:1567.98, Ab6:1661.22,
    A6:1760, Bb6:1864.66, B6:1975.53,
    C7:2093, Db7:2217.46, D7:2349.32, Eb7:2489.02, E7:2637.02, F7:2793.83, Gb7:2959.96, G7:3135.96, Ab7:3322.44,
    A7:3520, Bb7:3729.31, B7:3951.07,
    C8:4186.01, Db8:4434.92, D8:4698.63, Eb8:4978.03, E8:5274.04, F8:5587.65, Gb8:5919.91, G8:6271.93, Ab8:6644.88,
    A8:7040, Bb8:7458.62, B8:7902.13,
    REST:0,
    // X#Y enharmonic aliases used in movement_custom_signal_tunes.h
    'F#G5':739.99, 'D#E5':622.25, 'G#A4':415.30, 'G#A5':830.61,
    'F#G6':1479.98, 'A#B5':932.33, 'D#E6':1244.51, 'A#B7':3729.31,
    'F#G4':369.99, 'G#A6':1661.22, 'A#B6':1864.66, 'D#E4':311.13,
    'A#B4':466.16, 'F#G3':185, 'G#A3':207.65,
    'C#D5':554.37, 'C#D4':277.18, 'C#D6':1108.73
};

// Signal tune data — note sequences from movement_custom_signal_tunes.h
export const SIGNAL_TUNES = [
    { id:'SIGNAL_TUNE_DEFAULT',          name:'Default',
      notes:[['C8',5],['REST',6],['C8',5]] },
    { id:'SIGNAL_TUNE_ZELDA_SECRET',     name:'Zelda Secret',
      notes:[['G5',8],['F#G5',8],['D#E5',8],['A4',8],['G#A4',8],['E5',8],['G#A5',8],['C6',20]] },
    { id:'SIGNAL_TUNE_MARIO_THEME',      name:'Mario Theme',
      notes:[['E6',7],['REST',2],['E6',7],['REST',10],['E6',7],['REST',11],['C6',7],['REST',1],['E6',7],['REST',10],['G6',8],['REST',30],['G5',8]] },
    { id:'SIGNAL_TUNE_MGS_CODEC',        name:'MGS Codec',
      notes:[['G#A5',1],['C6',1],['G#A5',1],['C6',1],['G#A5',1],['C6',1],['G#A5',1],['C6',1],['G#A5',1],['C6',1],['G#A5',1],['C6',1],['G#A5',1],['C6',1],['G#A5',1],['C6',1]] },
    { id:'SIGNAL_TUNE_KIM_POSSIBLE',     name:'Kim Possible',
      notes:[['G7',6],['G4',2],['REST',5],['G7',6],['G4',2],['REST',5],['A#B7',6],['REST',2],['G7',6],['G4',2]] },
    { id:'SIGNAL_TUNE_POWER_RANGERS',    name:'Power Rangers',
      notes:[['D8',6],['REST',8],['D8',6],['REST',8],['C8',6],['REST',2],['D8',6],['REST',8],['F8',6],['REST',8],['D8',6]] },
    { id:'SIGNAL_TUNE_LAYLA',            name:'Layla (Derek & Dominos)',
      notes:[['A6',5],['REST',1],['C7',5],['REST',1],['D7',5],['REST',1],['F7',5],['REST',1],['D7',5],['REST',1],['C7',5],['REST',1],['D7',20]] },
    { id:'SIGNAL_TUNE_HARRY_POTTER_SHORT', name:'Harry Potter (Short)',
      notes:[['B5',12],['REST',1],['E6',12],['REST',1],['G6',6],['REST',1],['F#G6',6],['REST',1],['E6',16],['REST',1],['B6',8],['REST',1],['A6',24],['REST',1],['F#G6',24]] },
    { id:'SIGNAL_TUNE_HARRY_POTTER_LONG',  name:'Harry Potter (Long)',
      notes:[['B5',12],['REST',1],['E6',12],['REST',1],['G6',6],['REST',1],['F#G6',6],['REST',1],['E6',16],['REST',1],['B6',8],['REST',1],['A6',24],['REST',1],['F#G6',24],['REST',1],['E6',12],['REST',1],['G6',6],['REST',1],['F#G6',6],['REST',1],['D#E6',16],['REST',1],['F6',8],['REST',1],['B5',24]] },
    { id:'SIGNAL_TUNE_JURASSIC_PARK',    name:'Jurassic Park',
      notes:[['B5',7],['REST',7],['A#B5',7],['REST',7],['B5',13],['REST',13],['F#G5',13],['REST',13],['E5',13],['REST',13]] },
    { id:'SIGNAL_TUNE_EVANGELION',       name:"A Cruel Angel's Thesis (NGE)",
      notes:[['G5',8],['A5',8],['C6',8],['REST',4],['A5',4],['C6',8],['D6',8],['REST',8],['G5',8],['A5',8],['C6',8],['REST',4],['A5',4],['E6',16]] },
    { id:'SIGNAL_TUNE_NOKIA',            name:'Nokia (Grande Valse)',
      notes:[['E5',8],['D5',8],['F#G4',16],['G#A4',16],['REST',4],['C#D5',8],['B4',8],['D4',16],['E4',16],['REST',4],['B4',8],['A4',8],['C#D4',16],['E4',16],['REST',4],['A4',20]] },
    { id:'SIGNAL_TUNE_TETRIS',           name:'Tetris (Korobeiniki)',
      notes:[['E5',12],['B4',6],['C5',6],['D5',12],['C5',6],['B4',6],['A4',12],['REST',2],['A4',6],['C5',6],['E5',12],['D5',6],['C5',6],['B4',12],['REST',2],['C5',6],['D5',12],['E5',12],['C5',12],['A4',12],['A4',16]] },
    { id:'SIGNAL_TUNE_IMPERIAL',         name:'Imperial March (Star Wars)',
      notes:[['G4',12],['G4',12],['G4',12],['REST',2],['D#E4',10],['A#B4',4],['G4',12],['REST',2],['D#E4',10],['A#B4',4],['G4',20]] },
    { id:'SIGNAL_TUNE_POKEMON',          name:'Pokémon Center',
      notes:[['C6',6],['E6',6],['G6',6],['C7',6],['E7',6],['REST',4],['G6',6],['E6',6],['C6',10]] },
    { id:'SIGNAL_TUNE_VICTORY',          name:'Final Fantasy Victory',
      notes:[['C5',6],['C5',6],['C5',6],['C5',12],['REST',4],['G#A4',6],['A#B4',6],['C5',6],['REST',2],['A#B4',6],['C5',20]] },
    { id:'SIGNAL_TUNE_TRITONE',          name:'iPhone Tri-Tone',
      notes:[['E6',8],['C#D6',8],['F#G5',16],['REST',4],['E6',8],['C#D6',8],['F#G5',16],['REST',4],['E6',8],['C#D6',8],['A5',16]] },
    { id:'SIGNAL_TUNE_SOSUMI',           name:'Mac Sosumi',
      notes:[['C6',4],['E6',4],['G6',4],['C7',8]] },
    { id:'SIGNAL_TUNE_OLDPHONE',         name:'Old Phone Ring',
      notes:[['E5',8],['G5',8],['E5',8],['G5',8],['REST',8],['E5',8],['G5',8],['E5',8],['G5',8]] },
    { id:'SIGNAL_TUNE_MGS_ALERT',        name:'MGS Alert (!)',
      notes:[['G4',2],['G#A4',2],['A4',2],['A#B4',2],['B4',2],['C5',2],['C#D5',2],['D5',2],['D#E5',2],['E5',8]] },
    { id:'SIGNAL_TUNE_SONIC',            name:'Sonic Ring',
      notes:[['E6',3],['G6',3],['E7',6]] },
    { id:'SIGNAL_TUNE_PACMAN',           name:'Pac-Man Death',
      notes:[['B5',6],['A#B5',6],['A5',6],['G#A5',6],['G5',6],['F#G5',6],['F5',6],['E5',6],['D#E5',6],['D5',6],['C#D5',6],['C5',10]] },
    { id:'SIGNAL_TUNE_WINERROR',         name:'Windows Error',
      notes:[['C4',10],['G3',10],['C4',10]] }
];

// LED Animation Constants
export const FADE_IN_STEPS = 20;
export const FADE_OUT_STEPS = 32;
export const STEP_MS = 1000 / 64; // 15.625ms per step @ 64 Hz

// ============================================================
// GLOBAL STATE
// ============================================================

export const state = {
    board: 'red',
    display: 'classic',
    branch: 'main',
    activeFaces: [],
    clock24h: false,
    ledRed: 0,
    ledGreen: 15,
    ledBlue: 0,
    smoothLEDFade: false,
    btnSound: false,
    hourlyChimeTune: 'SIGNAL_TUNE_DEFAULT',
    alarmTune: 'SIGNAL_TUNE_DEFAULT',
    smartAlarmTune: 'SIGNAL_TUNE_DEFAULT',
    activeHoursEnabled: true,
    activeHoursStart: 16,
    activeHoursEnd: 92,
    homebaseLat: null,
    homebaseLon: null,
    homebaseTz: null,
    zoneEmergenceMax: 25,
    zoneMomentumMax: 50,
    zoneActiveMax: 75
};

// Build state
export let buildId = null;
export let pollTimer = null;
export let cooldownTimer = null;
export let cooldownEnd = 0;

// Registry state
export let registry = null;

// LED animation state
export let ledAnimationFrame = null;

// Audio context (lazy initialized)
let audioContext = null;

/**
 * Update build state
 */
export function setBuildId(id) {
    buildId = id;
}

export function setPollTimer(timer) {
    pollTimer = timer;
}

export function setCooldownTimer(timer) {
    cooldownTimer = timer;
}

export function setCooldownEnd(timestamp) {
    cooldownEnd = timestamp;
}

export function setRegistry(reg) {
    registry = reg;
}

export function setLedAnimationFrame(frame) {
    ledAnimationFrame = frame;
}

/**
 * Get or create audio context
 */
export function getAudioContext() {
    if (!audioContext) {
        audioContext = new (window.AudioContext || window.webkitAudioContext)();
    }
    return audioContext;
}
