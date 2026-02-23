// ============================================================
// FACE REGISTRY & MANAGEMENT
// ============================================================

import { state, registry, setRegistry } from './state.js';
import { escapeHTML } from './utils.js';
import { updateHash } from './template.js';

const REGISTRY_MOCK = {
    faces: [
        { id: "clock_face", name: "Clock", category: "clock", description: "Main clock display", default: true },
        { id: "wyoscan_face", name: "Wyoscan", category: "clock", description: "Wyoming scanner clock", default: true },
        { id: "alarm_face", name: "Alarm", category: "complication", description: "Simple alarm", default: true },
        { id: "temperature_display_face", name: "Temperature", category: "sensor", description: "Thermistor readout", default: false },
        { id: "movement_settings_face", name: "Settings", category: "settings", description: "Firmware settings", default: true }
    ],
    categories: [
        { id: "clock", name: "Clock" },
        { id: "complication", name: "Complication" },
        { id: "sensor", name: "Sensor" },
        { id: "demo", name: "Demo" },
        { id: "io", name: "I/O" },
        { id: "settings", name: "Settings" }
    ]
};

/**
 * Load face registry from JSON file
 */
export async function loadRegistry() {
    try {
        const resp = await fetch('./face_registry.json');
        if (!resp.ok) throw new Error('HTTP ' + resp.status);
        const data = await resp.json();
        setRegistry(data);
    } catch (e) {
        console.warn('Failed to load face registry, using mock:', e);
        setRegistry(REGISTRY_MOCK);
    }
}

/**
 * Add face to active list
 */
export function addFace(id) {
    const secIdx = state.activeFaces.indexOf('__secondary__');
    if (secIdx < 0) {
        state.activeFaces.push(id);
    } else {
        state.activeFaces.splice(secIdx, 0, id);
    }
    updateHash();
}

/**
 * Remove face from active list by index
 */
export function removeFace(idx) {
    if (idx >= 0 && idx < state.activeFaces.length && state.activeFaces[idx] !== '__secondary__') {
        state.activeFaces.splice(idx, 1);
        updateHash();
    }
}

/**
 * Calculate and display flash memory usage
 */
export function updateFlashUsage() {
    if (!registry) return;
    
    const activeFaceIds = state.activeFaces.filter(id => id !== '__secondary__');
    let totalBytes = 0;
    const unresolvedFaces = [];

    activeFaceIds.forEach(id => {
        const face = registry.faces.find(f => f.id === id);
        if (face && face.flash_bytes) {
            totalBytes += face.flash_bytes;
        } else {
            unresolvedFaces.push(id);
        }
    });

    const usageEl = document.getElementById('flashUsage');
    const warningEl = document.getElementById('flashWarning');
    
    if (unresolvedFaces.length > 0) {
        if (usageEl) usageEl.textContent = 'UNKNOWN';
        if (warningEl) warningEl.style.display = 'none';
        return;
    }

    const kb = (totalBytes / 1024).toFixed(1);
    if (usageEl) usageEl.textContent = kb + ' KB';

    const FLASH_LIMIT = 230 * 1024; // 230 KB safe limit
    if (warningEl) {
        if (totalBytes > FLASH_LIMIT) {
            warningEl.style.display = 'block';
        } else {
            warningEl.style.display = 'none';
        }
    }
}
