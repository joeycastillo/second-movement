// ============================================================
// UI UTILITIES & RENDERING
// ============================================================

import { state, getAudioContext, BOARDS, DISPLAYS, BRANCHES, SIGNAL_TUNES, NOTE_FREQUENCIES, FADE_IN_STEPS, FADE_OUT_STEPS, STEP_MS, setLedAnimationFrame, ledAnimationFrame, registry } from './state.js';
import { escapeHTML } from './utils.js';
import { updateHash } from './template.js';
import { updateFlashUsage } from './faces.js';

/**
 * Clock display (system bar)
 */
export function updateClock() {
    const now = new Date();
    const h = now.getHours().toString().padStart(2, '0');
    const m = now.getMinutes().toString().padStart(2, '0');
    const statusEl = document.getElementById('sysStatus');
    if (statusEl) {
        statusEl.textContent = h + ':' + m + ' | READY';
    }
}

/**
 * Wire slider input to value display and callback
 */
export function wireSlider(id, valId, cb) {
    const slider = document.getElementById(id);
    const valEl = document.getElementById(valId);
    slider.addEventListener('input', e => {
        const v = e.target.value;
        valEl.textContent = v;
        cb(v);
    });
}

/**
 * Set toggle button state (on/off)
 */
export function setToggle(trackId, labelId, value) {
    const track = document.getElementById(trackId);
    const label = document.getElementById(labelId);
    if (value) {
        track.classList.add('on');
        track.setAttribute('aria-checked', 'true');
        label.textContent = '▸ ON';
    } else {
        track.classList.remove('on');
        track.setAttribute('aria-checked', 'false');
        label.textContent = '▸ OFF';
    }
}

/**
 * Update LED color preview
 */
export function updateLEDPreview() {
    const red = parseInt(document.getElementById('ledRed')?.value || '0', 10);
    const green = parseInt(document.getElementById('ledGreen')?.value || '0', 10);
    const blue = parseInt(document.getElementById('ledBlue')?.value || '0', 10);

    const r = red * 17;
    const g = green * 17;
    const b = blue * 17;

    const color = `rgb(${r}, ${g}, ${b})`;
    const glowEl = document.getElementById('led-glow');
    if (glowEl) {
        glowEl.style.backgroundColor = color;
        glowEl.style.color = color;
    }

    updateLEDFadePreview();
}

/**
 * Quadratic gamma curve for LED brightness (matches firmware)
 */
export function calcBrightness(step, maxSteps, target) {
    const stepSq = step * step;
    const maxSq = maxSteps * maxSteps;
    return (stepSq * target) / maxSq;
}

/**
 * Draw LED on canvas with glow effect
 */
export function drawLED(ctx, r, g, b, brightness) {
    const centerX = 50;
    const centerY = 50;
    const radius = 30;

    const br = Math.floor(r * brightness);
    const bg = Math.floor(g * brightness);
    const bb = Math.floor(b * brightness);

    const gradient = ctx.createRadialGradient(centerX, centerY, 0, centerX, centerY, radius);
    gradient.addColorStop(0, `rgba(${br}, ${bg}, ${bb}, 1)`);
    gradient.addColorStop(0.7, `rgba(${br}, ${bg}, ${bb}, 0.5)`);
    gradient.addColorStop(1, `rgba(${br}, ${bg}, ${bb}, 0)`);

    ctx.fillStyle = gradient;
    ctx.fillRect(0, 0, 100, 100);

    ctx.beginPath();
    ctx.arc(centerX, centerY, radius * 0.7, 0, Math.PI * 2);
    ctx.fillStyle = `rgb(${br}, ${bg}, ${bb})`;
    ctx.fill();
}

/**
 * Start LED fade animation preview
 */
export function startLEDPreview() {
    const canvas = document.getElementById('ledCanvas');
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    const r = (state.ledRed << 4) | state.ledRed;
    const g = (state.ledGreen << 4) | state.ledGreen;
    const b = (state.ledBlue << 4) | state.ledBlue;

    let phase = 'fade-in';
    let step = 0;
    let startTime = Date.now();

    function animate() {
        const elapsed = Date.now() - startTime;

        ctx.fillStyle = '#0B0900';
        ctx.fillRect(0, 0, 100, 100);

        let brightness = 0;

        if (phase === 'fade-in') {
            step = Math.min(FADE_IN_STEPS, Math.floor(elapsed / STEP_MS));
            brightness = calcBrightness(step, FADE_IN_STEPS, 1.0);

            if (step >= FADE_IN_STEPS) {
                phase = 'hold-on';
                startTime = Date.now();
            }
        } else if (phase === 'hold-on') {
            brightness = 1.0;

            if (elapsed > 1000) {
                phase = 'fade-out';
                step = 0;
                startTime = Date.now();
            }
        } else if (phase === 'fade-out') {
            step = Math.min(FADE_OUT_STEPS, Math.floor(elapsed / STEP_MS));
            const remaining = FADE_OUT_STEPS - step;
            brightness = calcBrightness(remaining, FADE_OUT_STEPS, 1.0);

            if (step >= FADE_OUT_STEPS) {
                phase = 'hold-off';
                startTime = Date.now();
            }
        } else if (phase === 'hold-off') {
            brightness = 0;

            if (elapsed > 1000) {
                phase = 'fade-in';
                step = 0;
                startTime = Date.now();
            }
        }

        drawLED(ctx, r, g, b, brightness);

        setLedAnimationFrame(requestAnimationFrame(animate));
    }

    animate();
}

/**
 * Stop LED fade animation preview
 */
export function stopLEDPreview() {
    if (ledAnimationFrame) {
        cancelAnimationFrame(ledAnimationFrame);
        setLedAnimationFrame(null);
    }
}

/**
 * Update LED fade preview visibility
 */
export function updateLEDFadePreview() {
    const previewEl = document.getElementById('ledFadePreview');
    if (!previewEl) return;

    if (state.smoothLEDFade) {
        previewEl.style.display = 'flex';
        previewEl.classList.add('active');
        stopLEDPreview();
        startLEDPreview();
    } else {
        previewEl.style.display = 'none';
        previewEl.classList.remove('active');
        stopLEDPreview();
    }
}

/**
 * Build time options for active hours dropdown
 */
export function buildTimeOptions(selectedVal) {
    let html = '';
    for (let i = 0; i < 96; i++) {
        const h = String(Math.floor(i / 4)).padStart(2, '0');
        const m = String((i % 4) * 15).padStart(2, '0');
        const sel = i === selectedVal ? ' selected' : '';
        html += '<option value="' + i + '"' + sel + '>' + h + ':' + m + '</option>';
    }
    return html;
}

/**
 * Update active hours time picker visibility
 */
export function updateAhTimeVisibility() {
    const startRow = document.getElementById('ahStartRow');
    const endRow = document.getElementById('ahEndRow');
    const visualContainer = document.getElementById('ahVisual');
    
    if (state.activeHoursEnabled) {
        if (startRow) startRow.style.display = 'flex';
        if (endRow) endRow.style.display = 'flex';
        if (visualContainer) visualContainer.classList.add('active');
        updateAhVisualBar();
    } else {
        if (startRow) startRow.style.display = 'none';
        if (endRow) endRow.style.display = 'none';
        if (visualContainer) visualContainer.classList.remove('active');
    }
}

/**
 * Update active hours visual bar
 */
export function updateAhVisualBar() {
    const bar = document.getElementById('ahActiveBar');
    if (!bar) return;

    const total = 96;
    const start = state.activeHoursStart;
    const end = state.activeHoursEnd;

    const leftPct = (start / total) * 100;
    const widthPct = ((end - start) / total) * 100;

    bar.style.left = leftPct + '%';
    bar.style.width = widthPct + '%';
}

/**
 * Update zone configuration visibility (Phase 3)
 */
export function updateZoneConfigVisibility() {
    const homebaseValid = state.homebaseLat !== null && state.homebaseLon !== null;
    const zonePanel = document.getElementById('zoneConfigPanel');
    if (zonePanel) {
        zonePanel.style.display = homebaseValid ? 'block' : 'none';
    }
}

/**
 * Update zone preview indicator (Phase 3)
 */
export function updateZonePreview() {
    const previewScore = parseInt(document.getElementById('zone-preview-score')?.value || '50', 10);
    const indicator = document.getElementById('zone-indicator');
    const zoneName = document.getElementById('zone-name');

    if (!indicator || !zoneName) return;

    let name, color;
    if (previewScore < state.zoneEmergenceMax) {
        name = 'EMERGENCE';
        color = '#4A90E2';
    } else if (previewScore < state.zoneMomentumMax) {
        name = 'MOMENTUM';
        color = '#50C878';
    } else if (previewScore < state.zoneActiveMax) {
        name = 'ACTIVE';
        color = '#F5A623';
    } else {
        name = 'DESCENT';
        color = '#9B59B6';
    }

    indicator.style.left = previewScore + '%';
    zoneName.textContent = name;
    zoneName.style.color = color;
}

/**
 * Play signal tune preview (alarm sounds)
 */
export function playSignalTune(tuneId, buttonId) {
    const tune = SIGNAL_TUNES.find(t => t.id === tuneId);
    if (!tune) {
        console.error('Tune not found:', tuneId);
        return;
    }

    const audioCtx = getAudioContext();
    if (!audioCtx) {
        console.error('Web Audio API not supported');
        return;
    }

    const gainNode = audioCtx.createGain();
    gainNode.gain.value = 0.7;
    gainNode.connect(audioCtx.destination);

    const TICK_SEC = 0.016;
    let t = audioCtx.currentTime;

    tune.notes.forEach(([note, ticks]) => {
        const freq = NOTE_FREQUENCIES[note] ?? 0;
        const dur = ticks * TICK_SEC;
        if (freq > 0) {
            const osc = audioCtx.createOscillator();
            osc.type = 'square';
            osc.frequency.value = freq;
            osc.connect(gainNode);
            osc.start(t);
            osc.stop(t + dur);
        }
        t += dur;
    });

    const btn = buttonId ? document.getElementById(buttonId) : null;
    const totalMs = tune.notes.reduce((sum, [, ticks]) => sum + ticks * 16, 0);

    if (btn) {
        btn.classList.add('playing');
        btn.disabled = true;
        setTimeout(() => {
            btn.classList.remove('playing');
            btn.disabled = false;
        }, totalMs + 200);
    }
}

/**
 * Render board selection radio buttons
 */
export function renderBoards() {
    const el = document.getElementById('boardRadio');
    if (!el) return;
    
    el.innerHTML = '';
    BOARDS.forEach(b => {
        const btn = document.createElement('button');
        btn.className = 'radio-btn' + (state.board === b.id ? ' active' : '');
        btn.setAttribute('role', 'radio');
        btn.setAttribute('aria-checked', state.board === b.id ? 'true' : 'false');
        btn.innerHTML = '<span class="dot">' + (state.board === b.id ? '*' : ' ') + '</span> ' + b.name;
        btn.onclick = () => {
            state.board = b.id;
            renderBoards();
            updateHash();
        };
        el.appendChild(btn);
    });
}

/**
 * Render display selection radio buttons
 */
export function renderDisplays() {
    const el = document.getElementById('displayRadio');
    if (!el) return;
    
    el.innerHTML = '';
    DISPLAYS.forEach(d => {
        const btn = document.createElement('button');
        btn.className = 'radio-btn' + (state.display === d.id ? ' active' : '');
        btn.setAttribute('role', 'radio');
        btn.setAttribute('aria-checked', state.display === d.id ? 'true' : 'false');
        btn.innerHTML = '<span class="dot">' + (state.display === d.id ? '*' : ' ') + '</span> ' + d.name;
        btn.onclick = () => {
            state.display = d.id;
            renderDisplays();
            updateHash();
        };
        el.appendChild(btn);
    });
}

/**
 * Render branch selection radio buttons
 */
export function renderBranches() {
    const el = document.getElementById('branchRadio');
    if (!el) return;
    
    el.innerHTML = '';
    BRANCHES.forEach(b => {
        const btn = document.createElement('button');
        btn.className = 'radio-btn' + (state.branch === b.id ? ' active' : '');
        btn.setAttribute('role', 'radio');
        btn.setAttribute('aria-checked', state.branch === b.id ? 'true' : 'false');
        btn.setAttribute('title', b.description);
        btn.innerHTML = '<span class="dot">' + (state.branch === b.id ? '*' : ' ') + '</span> ' + b.name;
        btn.onclick = () => {
            state.branch = b.id;
            renderBranches();
            updateHash();
        };
        el.appendChild(btn);
    });
}

/**
 * Render active faces list (simplified - full version in app.js)
 */
export function renderActiveFaces() {
    updateFlashUsage();
}

/**
 * Render available faces list (simplified - full version in app.js)
 */
export function renderAvailFaces() {
    // Placeholder - full implementation requires Sortable.js integration
}
