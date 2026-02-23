// ============================================================
// TEMPLATE MANAGEMENT
// ============================================================

import { state, VALID_BOARDS, VALID_DISPLAYS, VALID_BRANCHES, VALID_SIGNAL_TUNES } from './state.js';
import { escapeHTML } from './utils.js';

export const TEMPLATES_KEY = 'sm_templates';

export const PRESET_TEMPLATES = [
    {
        id: 'preset_phase_engine',
        name: 'Phase Engine (Default)',
        description: 'Phase 4D dogfooding: circadian tracking with zone playlist',
        preset: true,
        state: {
            board: 'pro',
            display: 'classic',
            activeFaces: [
                'wyoscan_face', 'clock_face', 'emergence_face', 'momentum_face',
                'active_face', 'descent_face', 'timer_face', 'fast_stopwatch_face',
                'advanced_alarm_face', 'sleep_tracker_face', 'circadian_score_face',
                'world_clock_face', 'moon_phase_face', 'sunrise_sunset_face',
                '__secondary__', 'comms_face', 'lis2dw_monitor_face',
                'light_sensor_face', 'voltage_face', 'settings_face'
            ],
            clock24h: false,
            ledRed: 0, ledGreen: 15, ledBlue: 0,
            btnSound: true,
            hourlyChimeTune: 'SIGNAL_TUNE_DEFAULT',
            alarmTune: 'SIGNAL_TUNE_DEFAULT',
            smartAlarmTune: 'SIGNAL_TUNE_DEFAULT'
        }
    },
    {
        id: 'preset_standard',
        name: 'Standard Edition',
        description: 'Classic setup: clock, alarm, stopwatch, tools',
        preset: true,
        state: {
            board: 'red',
            display: 'classic',
            activeFaces: [
                'clock_face', 'alarm_face', 'stopwatch_face', 'countdown_face',
                '__secondary__', 'sunrise_sunset_face', 'moon_phase_face',
                'temperature_display_face', 'settings_face', 'set_time_face'
            ],
            clock24h: false,
            ledRed: 0, ledGreen: 15, ledBlue: 0,
            btnSound: false,
            hourlyChimeTune: 'SIGNAL_TUNE_DEFAULT',
            alarmTune: 'SIGNAL_TUNE_DEFAULT',
            smartAlarmTune: 'SIGNAL_TUNE_DEFAULT'
        }
    },
    {
        id: 'preset_activity',
        name: 'Activity Explorer',
        description: 'Sleep + activity tracking with sensors',
        preset: true,
        state: {
            board: 'red',
            display: 'classic',
            activeFaces: [
                'clock_face', 'alarm_face', '__secondary__',
                'temperature_display_face', 'activity_logging_face',
                'accelerometer_status_face', 'settings_face', 'set_time_face'
            ],
            clock24h: false,
            ledRed: 0, ledGreen: 15, ledBlue: 0,
            btnSound: false,
            hourlyChimeTune: 'SIGNAL_TUNE_DEFAULT',
            alarmTune: 'SIGNAL_TUNE_DEFAULT',
            smartAlarmTune: 'SIGNAL_TUNE_DEFAULT'
        }
    },
    {
        id: 'preset_minimal',
        name: 'Minimal',
        description: 'Just the essentials: clock + settings',
        preset: true,
        state: {
            board: 'red',
            display: 'classic',
            activeFaces: ['clock_face', '__secondary__', 'settings_face', 'set_time_face'],
            clock24h: false,
            ledRed: 0, ledGreen: 15, ledBlue: 0,
            btnSound: false,
            hourlyChimeTune: 'SIGNAL_TUNE_DEFAULT',
            alarmTune: 'SIGNAL_TUNE_DEFAULT',
            smartAlarmTune: 'SIGNAL_TUNE_DEFAULT'
        }
    }
];

export function loadUserTemplates() {
    try {
        return JSON.parse(localStorage.getItem(TEMPLATES_KEY) || '[]');
    } catch (e) {
        return [];
    }
}

export function saveUserTemplates(templates) {
    localStorage.setItem(TEMPLATES_KEY, JSON.stringify(templates));
}

export function saveCurrentAsTemplate(name) {
    if (!name || !name.trim()) return false;
    const templates = loadUserTemplates();
    const tmpl = {
        id: 'user_' + Date.now(),
        name: name.trim(),
        createdAt: Date.now(),
        state: JSON.parse(JSON.stringify(state))
    };
    templates.unshift(tmpl);
    saveUserTemplates(templates);
    return true;
}

export function deleteUserTemplate(id) {
    const templates = loadUserTemplates().filter(t => t.id !== id);
    saveUserTemplates(templates);
}

/**
 * Parse hash URL into state object
 * Requires registry to be loaded first for face validation
 */
export function parseHash(registry) {
    const hash = window.location.hash.replace('#', '');
    if (!hash) return null;

    const params = {};
    hash.split('&').forEach(part => {
        const eq = part.indexOf('=');
        if (eq > 0) {
            params[part.slice(0, eq)] = decodeURIComponent(part.slice(eq + 1));
        }
    });

    if (!params.faces) return null;

    const board = VALID_BOARDS.includes(params.board) ? params.board : 'red';
    const display = VALID_DISPLAYS.includes(params.display) ? params.display : 'classic';
    const branch = VALID_BRANCHES.includes(params.branch) ? params.branch : 'main';
    const hourlyChimeTune = VALID_SIGNAL_TUNES.includes(params.hc_tune) ? params.hc_tune : 'SIGNAL_TUNE_DEFAULT';
    const alarmTune = VALID_SIGNAL_TUNES.includes(params.al_tune) ? params.al_tune : 'SIGNAL_TUNE_DEFAULT';
    const smartAlarmTune = VALID_SIGNAL_TUNES.includes(params.sa_tune) ? params.sa_tune : 'SIGNAL_TUNE_DEFAULT';

    const clampLed = (val, def) => {
        const n = parseInt(val || String(def), 10);
        return (Number.isFinite(n) && n >= 0 && n <= 15) ? n : def;
    };

    const validFaceIds = new Set((registry ? registry.faces : []).map(f => f.id));
    const rawFaceIds = params.faces.split(',').filter(Boolean);
    const faceIds = rawFaceIds.filter(id => validFaceIds.has(id));

    const secondaryAt = params.secondary !== undefined ? parseInt(params.secondary, 10) : faceIds.length;
    const clampedSecondary = (Number.isFinite(secondaryAt) && secondaryAt >= 0) ? secondaryAt : faceIds.length;

    const activeFaces = [];
    faceIds.forEach((id, i) => {
        if (i === clampedSecondary) activeFaces.push('__secondary__');
        activeFaces.push(id);
    });
    if (clampedSecondary >= faceIds.length) activeFaces.push('__secondary__');

    const clampQh = (val, def) => {
        const n = parseInt(val || String(def), 10);
        return (Number.isFinite(n) && n >= 0 && n <= 95) ? n : def;
    };

    let homebaseLat = null;
    let homebaseLon = null;
    let homebaseTz = null;

    if (params.hb_lat !== undefined) {
        const lat = parseFloat(params.hb_lat);
        if (Number.isFinite(lat) && lat >= -90 && lat <= 90) {
            homebaseLat = lat;
        }
    }

    if (params.hb_lon !== undefined) {
        const lon = parseFloat(params.hb_lon);
        if (Number.isFinite(lon) && lon >= -180 && lon <= 180) {
            homebaseLon = lon;
        }
    }

    if (params.hb_tz !== undefined && params.hb_tz.trim().length > 0) {
        const tz = params.hb_tz.trim();
        const tzRegex = /^([A-Z]{3,4}|UTC[+-]?\d{1,2}|[+-]?\d{1,4})$/i;
        if (tzRegex.test(tz)) {
            homebaseTz = tz;
        }
    }

    const clampZone = (val, def, min, max) => {
        const n = parseInt(val, 10);
        return (Number.isFinite(n) && n >= min && n <= max) ? n : def;
    };

    const zoneEmergenceMax = params.zem !== undefined ? clampZone(params.zem, 25, 15, 35) : 25;
    const zoneMomentumMax = params.zmm !== undefined ? clampZone(params.zmm, 50, 40, 60) : 50;
    const zoneActiveMax = params.zam !== undefined ? clampZone(params.zam, 75, 65, 85) : 75;

    return {
        board,
        display,
        branch,
        activeFaces,
        clock24h: params['24h'] === 'true',
        ledRed: clampLed(params.led_r, 0),
        ledGreen: clampLed(params.led_g, 15),
        ledBlue: clampLed(params.led_b, 0),
        smoothLEDFade: params.smooth_fade === 'true',
        btnSound: params.btn === 'true',
        hourlyChimeTune,
        alarmTune,
        smartAlarmTune,
        activeHoursEnabled: params.ah_en !== undefined ? params.ah_en === 'true' : true,
        activeHoursStart: clampQh(params.ah_start, 16),
        activeHoursEnd: clampQh(params.ah_end, 92),
        homebaseLat,
        homebaseLon,
        homebaseTz,
        zoneEmergenceMax,
        zoneMomentumMax,
        zoneActiveMax
    };
}

/**
 * Update hash URL from current state
 */
export function updateHash() {
    const faces = state.activeFaces.filter(id => id !== '__secondary__').join(',');
    const secIdx = state.activeFaces.indexOf('__secondary__');
    const secondaryAt = secIdx >= 0
        ? state.activeFaces.slice(0, secIdx).filter(id => id !== '__secondary__').length
        : state.activeFaces.filter(id => id !== '__secondary__').length;

    const params = [
        'board=' + state.board,
        'display=' + state.display,
        'branch=' + encodeURIComponent(state.branch),
        'faces=' + encodeURIComponent(faces),
        'secondary=' + secondaryAt,
        '24h=' + state.clock24h,
        'led_r=' + state.ledRed,
        'led_g=' + state.ledGreen,
        'led_b=' + state.ledBlue,
        'smooth_fade=' + state.smoothLEDFade,
        'btn=' + state.btnSound,
        'hc_tune=' + encodeURIComponent(state.hourlyChimeTune),
        'al_tune=' + encodeURIComponent(state.alarmTune),
        'sa_tune=' + encodeURIComponent(state.smartAlarmTune),
        'ah_en=' + state.activeHoursEnabled,
        'ah_start=' + state.activeHoursStart,
        'ah_end=' + state.activeHoursEnd
    ];

    if (state.homebaseLat !== null) {
        params.push('hb_lat=' + encodeURIComponent(state.homebaseLat));
    }
    if (state.homebaseLon !== null) {
        params.push('hb_lon=' + encodeURIComponent(state.homebaseLon));
    }
    if (state.homebaseTz !== null) {
        params.push('hb_tz=' + encodeURIComponent(state.homebaseTz));
    }

    if (state.zoneEmergenceMax !== undefined && state.zoneEmergenceMax !== 25) {
        params.push('zem=' + encodeURIComponent(state.zoneEmergenceMax));
    }
    if (state.zoneMomentumMax !== undefined && state.zoneMomentumMax !== 50) {
        params.push('zmm=' + encodeURIComponent(state.zoneMomentumMax));
    }
    if (state.zoneActiveMax !== undefined && state.zoneActiveMax !== 75) {
        params.push('zam=' + encodeURIComponent(state.zoneActiveMax));
    }

    history.replaceState(null, '', '#' + params.join('&'));
}
