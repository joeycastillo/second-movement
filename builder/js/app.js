// ============================================================
// SECOND MOVEMENT BUILDER - Main Application Entry Point
// Phase 1: Modularized architecture with Three.js foundation
// ============================================================

import { state, registry, setRegistry, getAudioContext } from './state.js';
import { debounce, copyLink } from './utils.js';
import { parseHash, updateHash, PRESET_TEMPLATES, loadUserTemplates, saveCurrentAsTemplate, deleteUserTemplate, saveUserTemplates } from './template.js';
import { loadRegistry, addFace, removeFace, updateFlashUsage } from './faces.js';
import { triggerBuild, pollBuild, validateToken, updateTokenStatus, startCooldownUI, trackRecentBuild, updateRecentFacesList, clearRecentFaces } from './build.js';
import {
    updateClock, wireSlider, setToggle, updateLEDPreview, updateLEDFadePreview,
    buildTimeOptions, updateAhTimeVisibility, updateAhVisualBar,
    updateZoneConfigVisibility, updateZonePreview, playSignalTune,
    renderBoards, renderDisplays, renderBranches
} from './ui.js';
import { SceneManager } from './scene-manager.js';
import { escapeHTML } from './utils.js';

// Global scene manager instance
let sceneManager = null;

/**
 * Main initialization function
 */
async function init() {
//     console.log('🚀 Second Movement Builder - Phase 1: Foundation');

    // Start clock
    updateClock();
    setInterval(updateClock, 30000);

    // Load face registry
    await loadRegistry();

    // Restore GitHub token from sessionStorage
    const savedToken = sessionStorage.getItem('sm_github_token');
    if (savedToken) {
        document.getElementById('githubToken').value = savedToken;
        const result = await validateToken(savedToken.trim());
        updateTokenStatus(result);
    }

    // Parse hash URL or apply defaults
    const hashState = parseHash(registry);
    if (hashState) {
        Object.assign(state, hashState);
    } else {
        // Default active faces from registry
        state.activeFaces = registry.faces
            .filter(f => f.default)
            .map(f => f.id);

        // Insert secondary divider
        const settingsIdx = state.activeFaces.findIndex(id => {
            const f = registry.faces.find(r => r.id === id);
            return f && f.category === 'settings';
        });
        if (settingsIdx > 0) {
            state.activeFaces.splice(settingsIdx, 0, '__secondary__');
        } else {
            state.activeFaces.push('__secondary__');
        }
    }

    // Restore UI state from state object
    document.getElementById('ledRed').value = state.ledRed;
    document.getElementById('ledRedVal').textContent = state.ledRed;
    document.getElementById('ledGreen').value = state.ledGreen;
    document.getElementById('ledGreenVal').textContent = state.ledGreen;
    document.getElementById('ledBlue').value = state.ledBlue;
    document.getElementById('ledBlueVal').textContent = state.ledBlue;
    document.getElementById('hourlyChimeTune').value = state.hourlyChimeTune;
    document.getElementById('alarmTune').value = state.alarmTune;
    document.getElementById('smartAlarmTune').value = state.smartAlarmTune;

    setToggle('toggle24h', 'label24h', state.clock24h);
    setToggle('toggleBtn', 'labelBtn', state.btnSound);
    setToggle('toggleSmoothLED', 'labelSmoothLED', state.smoothLEDFade);

    updateLEDPreview();
    updateLEDFadePreview();

    setToggle('toggleAhEn', 'labelAhEn', state.activeHoursEnabled);
    document.getElementById('ahStart').innerHTML = buildTimeOptions(state.activeHoursStart);
    document.getElementById('ahEnd').innerHTML = buildTimeOptions(state.activeHoursEnd);
    updateAhTimeVisibility();

    // Restore homebase configuration
    if (state.homebaseLat !== null) {
        document.getElementById('homebase-lat').value = state.homebaseLat;
    }
    if (state.homebaseLon !== null) {
        document.getElementById('homebase-lon').value = state.homebaseLon;
    }
    if (state.homebaseTz !== null) {
        document.getElementById('homebase-tz').value = state.homebaseTz;
    }

    // Restore zone thresholds
    document.getElementById('zone-emergence-max').value = state.zoneEmergenceMax;
    document.getElementById('zone-emergence-val').textContent = state.zoneEmergenceMax;
    document.getElementById('zone-momentum-max').value = state.zoneMomentumMax;
    document.getElementById('zone-momentum-val').textContent = state.zoneMomentumMax;
    document.getElementById('zone-active-max').value = state.zoneActiveMax;
    document.getElementById('zone-active-val').textContent = state.zoneActiveMax;
    updateZonePreview();

    // Render UI
    renderBoards();
    renderDisplays();
    renderBranches();
    renderActiveFaces();
    renderAvailFaces();
    renderTemplates();
    updateRecentFacesList();

    // Check cooldown
    const cooldownStored = parseInt(localStorage.getItem('sm_cooldown_end') || '0', 10);
    if (cooldownStored > Date.now()) {
        startCooldownUI(cooldownStored - Date.now());
    }

    // Check if we're resuming a build
    const storedBuildId = localStorage.getItem('sm_build_id');
    if (storedBuildId) {
        const result = localStorage.getItem('sm_build_result_' + storedBuildId);
        if (!result) {
            // Build might still be in progress
            pollBuild();
        }
    }

    // Wire event listeners
    wireEventListeners();

    // Initialize Three.js scene manager (Phase 2: Active 3D rendering)
    const canvas = document.getElementById('scene-canvas');
    if (canvas) {
        sceneManager = new SceneManager(canvas);
        sceneManager.init();
//         console.log('✅ Three.js scene initialized (Phase 2: Active rendering with cube + grid)');
    }

    // Clean disposal on page unload
    window.addEventListener('beforeunload', () => {
        if (sceneManager) {
            sceneManager.dispose();
        }
    });

//     console.log('✅ Builder initialized successfully');
}

/**
 * Wire all event listeners
 */
function wireEventListeners() {
    // Build button
    document.getElementById('buildBtn').addEventListener('click', triggerBuild);

    // Copy link button
    document.getElementById('copyLinkBtn').addEventListener('click', () => copyLink(updateHash));

    // LED sliders
    wireSlider('ledRed', 'ledRedVal', v => { state.ledRed = +v; updateLEDPreview(); updateHash(); });
    wireSlider('ledGreen', 'ledGreenVal', v => { state.ledGreen = +v; updateLEDPreview(); updateHash(); });
    wireSlider('ledBlue', 'ledBlueVal', v => { state.ledBlue = +v; updateLEDPreview(); updateHash(); });

    // LED preview button
    document.getElementById('ledPreviewBtn').addEventListener('click', () => {
        if (state.smoothLEDFade) {
            // Show fade animation
            import('./ui.js').then(({ startLEDPreview, stopLEDPreview }) => {
                stopLEDPreview();
                startLEDPreview();
            });
        } else {
            // Flash instant preview
            const glowEl = document.getElementById('led-glow');
            if (glowEl) {
                glowEl.style.opacity = '0';
                setTimeout(() => { glowEl.style.opacity = '1'; }, 50);
            }
        }
    });

    // Alarm tune dropdowns
    document.getElementById('hourlyChimeTune').addEventListener('change', e => {
        state.hourlyChimeTune = e.target.value;
        updateHash();
    });
    document.getElementById('alarmTune').addEventListener('change', e => {
        state.alarmTune = e.target.value;
        updateHash();
    });
    document.getElementById('smartAlarmTune').addEventListener('change', e => {
        state.smartAlarmTune = e.target.value;
        updateHash();
    });

    // Debounced token validation
    const debouncedTokenValidation = debounce(async (token) => {
        if (token.length === 0) {
            document.getElementById('tokenStatus').style.display = 'none';
            return;
        }
        const result = await validateToken(token);
        updateTokenStatus(result);
    }, 500);

    document.getElementById('githubToken').addEventListener('input', e => {
        const token = e.target.value.trim();
        sessionStorage.setItem('sm_github_token', e.target.value);
        debouncedTokenValidation(token);
    });

    // Toggle buttons
    document.getElementById('toggle24h').addEventListener('click', () => {
        state.clock24h = !state.clock24h;
        setToggle('toggle24h', 'label24h', state.clock24h);
        updateHash();
    });

    document.getElementById('toggleBtn').addEventListener('click', () => {
        state.btnSound = !state.btnSound;
        setToggle('toggleBtn', 'labelBtn', state.btnSound);
        updateHash();
    });

    document.getElementById('toggleSmoothLED').addEventListener('click', () => {
        state.smoothLEDFade = !state.smoothLEDFade;
        setToggle('toggleSmoothLED', 'labelSmoothLED', state.smoothLEDFade);
        updateLEDFadePreview();
        updateHash();
    });

    document.getElementById('toggleAhEn').addEventListener('click', () => {
        state.activeHoursEnabled = !state.activeHoursEnabled;
        setToggle('toggleAhEn', 'labelAhEn', state.activeHoursEnabled);
        updateAhTimeVisibility();
        updateHash();
    });

    // Active hours time pickers
    document.getElementById('ahStart').addEventListener('change', e => {
        const v = parseInt(e.target.value, 10);
        state.activeHoursStart = (Number.isFinite(v) && v >= 0 && v <= 95) ? v : 16;
        updateAhVisualBar();
        updateHash();
    });

    document.getElementById('ahEnd').addEventListener('change', e => {
        const v = parseInt(e.target.value, 10);
        state.activeHoursEnd = (Number.isFinite(v) && v >= 0 && v <= 95) ? v : 92;
        updateAhVisualBar();
        updateHash();
    });

    // Homebase location inputs
    document.getElementById('homebase-lat').addEventListener('input', e => {
        const lat = parseFloat(e.target.value);
        state.homebaseLat = (Number.isFinite(lat) && lat >= -90 && lat <= 90) ? lat : null;
        updateHash();
    });

    document.getElementById('homebase-lon').addEventListener('input', e => {
        const lon = parseFloat(e.target.value);
        state.homebaseLon = (Number.isFinite(lon) && lon >= -180 && lon <= 180) ? lon : null;
        updateHash();
    });

    document.getElementById('homebase-tz').addEventListener('input', e => {
        const tz = e.target.value.trim();
        const tzRegex = /^([A-Z]{3,4}|UTC[+-]?\d{1,2}|[+-]?\d{1,4})$/i;
        if (tz === '' || tzRegex.test(tz)) {
            state.homebaseTz = tz || null;
            e.target.style.borderColor = 'var(--color-border)';
            updateHash();
        } else {
            e.target.style.borderColor = 'var(--color-critical)';
        }
    });

    // Zone configuration sliders
    document.getElementById('zone-emergence-max').addEventListener('input', e => {
        let val = parseInt(e.target.value, 10);
        if (isNaN(val) || val < 15 || val > 35) val = 25;
        state.zoneEmergenceMax = val;
        document.getElementById('zone-emergence-val').textContent = val;
        updateZonePreview();
        updateHash();
    });

    document.getElementById('zone-momentum-max').addEventListener('input', e => {
        let val = parseInt(e.target.value, 10);
        if (isNaN(val) || val < 40 || val > 60) val = 50;
        state.zoneMomentumMax = val;
        document.getElementById('zone-momentum-val').textContent = val;
        updateZonePreview();
        updateHash();
    });

    document.getElementById('zone-active-max').addEventListener('input', e => {
        let val = parseInt(e.target.value, 10);
        if (isNaN(val) || val < 65 || val > 85) val = 75;
        state.zoneActiveMax = val;
        document.getElementById('zone-active-val').textContent = val;
        updateZonePreview();
        updateHash();
    });

    document.getElementById('zone-preview-score').addEventListener('input', e => {
        let score = parseInt(e.target.value, 10);
        if (isNaN(score) || score < 0 || score > 100) score = 50;
        e.target.value = score;
        updateZonePreview();
    });

    document.getElementById('zone-reset').addEventListener('click', () => {
        state.zoneEmergenceMax = 25;
        state.zoneMomentumMax = 50;
        state.zoneActiveMax = 75;
        document.getElementById('zone-emergence-max').value = 25;
        document.getElementById('zone-momentum-max').value = 50;
        document.getElementById('zone-active-max').value = 75;
        document.getElementById('zone-emergence-val').textContent = '25';
        document.getElementById('zone-momentum-val').textContent = '50';
        document.getElementById('zone-active-val').textContent = '75';
        updateZonePreview();
        updateHash();
    });

    // Alarm tune preview buttons
    document.getElementById('hourlyChimePreviewBtn').addEventListener('click', () => {
        playSignalTune(state.hourlyChimeTune, 'hourlyChimePreviewBtn');
    });
    document.getElementById('alarmTunePreviewBtn').addEventListener('click', () => {
        playSignalTune(state.alarmTune, 'alarmTunePreviewBtn');
    });
    document.getElementById('smartAlarmTunePreviewBtn').addEventListener('click', () => {
        playSignalTune(state.smartAlarmTune, 'smartAlarmTunePreviewBtn');
    });

    // Templates panel toggle
    document.getElementById('templatesPanelBtn').addEventListener('click', function() {
        const body = document.getElementById('templatesPanelBody');
        const chevron = document.getElementById('templatesChevron');
        const expanded = this.getAttribute('aria-expanded') === 'true';
        this.setAttribute('aria-expanded', String(!expanded));
        body.style.display = expanded ? 'none' : 'block';
        chevron.textContent = expanded ? '+' : '-';
    });

    // Save template button
    document.getElementById('saveTemplateBtn').addEventListener('click', function() {
        const input = document.getElementById('templateNameInput');
        if (saveCurrentAsTemplate(input.value)) {
            input.value = '';
            renderTemplates();
        }
    });

    // Recent faces panel toggle
    document.getElementById('recentFacesPanelBtn').addEventListener('click', function() {
        const body = document.getElementById('recentFacesPanelBody');
        const chevron = document.getElementById('recentFacesChevron');
        const expanded = this.getAttribute('aria-expanded') === 'true';
        this.setAttribute('aria-expanded', String(!expanded));
        body.style.display = expanded ? 'none' : 'block';
        chevron.textContent = expanded ? '+' : '-';
    });

    // Clear recent faces button
    document.getElementById('clearRecentBtn').addEventListener('click', () => {
        if (confirm('Clear recent build history?')) {
            clearRecentFaces();
        }
    });

    // Listen for loadRecentBuild events
    window.addEventListener('loadRecentBuild', (e) => {
        // Load the faces configuration
        state.activeFaces = [...e.detail.faces];
        renderActiveFaces();
        updateHash();
        // Scroll to watch faces section
        document.querySelector('#config-panel').scrollTop = document.querySelector('.section-title').offsetTop;
    });

    // CRT Effects panel toggle
    document.getElementById('crtEffectsPanelBtn').addEventListener('click', function() {
        const body = document.getElementById('crtEffectsPanelBody');
        const chevron = document.getElementById('crtEffectsChevron');
        const expanded = this.getAttribute('aria-expanded') === 'true';
        this.setAttribute('aria-expanded', String(!expanded));
        body.style.display = expanded ? 'none' : 'block';
        chevron.textContent = expanded ? '+' : '-';
    });

    // Alarm tunes panel toggle
    document.getElementById('alarmTunesPanelBtn').addEventListener('click', function() {
        const body = document.getElementById('alarmTunesPanelBody');
        const chevron = document.getElementById('alarmTunesChevron');
        const expanded = this.getAttribute('aria-expanded') === 'true';
        this.setAttribute('aria-expanded', String(!expanded));
        body.style.display = expanded ? 'none' : 'block';
        chevron.textContent = expanded ? '+' : '-';
    });

    // Mobile audio unlock
    const isMobile = /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent)
        || (navigator.platform === 'MacIntel' && navigator.maxTouchPoints > 1);
    if (isMobile) {
        document.getElementById('audioUnlockNotice').style.display = 'block';
    }

    document.getElementById('audioUnlockBtn').addEventListener('click', () => {
        const ctx = getAudioContext();
        if (ctx) {
            const osc = ctx.createOscillator();
            const gain = ctx.createGain();
            gain.gain.value = 0;
            osc.connect(gain);
            gain.connect(ctx.destination);
            osc.start();
            osc.stop(ctx.currentTime + 0.01);
        }
        document.getElementById('audioUnlockNotice').style.display = 'none';
    });

    // Phase 3: Scene controls
    wireSceneControls();

//     console.log('✅ Event listeners wired');
}

/**
 * Wire Phase 3 scene controls (scene switching, category filters)
 */
function wireSceneControls() {
    // Scene selector buttons
    const sceneButtons = document.querySelectorAll('.scene-btn');
    sceneButtons.forEach(btn => {
        btn.addEventListener('click', () => {
            const sceneMode = btn.dataset.scene;
            if (!sceneManager) return;

            // Update active button
            sceneButtons.forEach(b => b.classList.remove('active'));
            btn.classList.add('active');

            // Switch scene
            sceneManager.setSceneMode(sceneMode);

            // Update scene controls data attribute (for CSS)
            const controls = document.getElementById('scene-controls');
            if (controls) {
                controls.dataset.scene = sceneMode;
            }

//             console.log('🎬 Scene switched to:', sceneMode);
        });
    });

    // Camera reset button
    const cameraResetBtn = document.getElementById('camera-reset');
    if (cameraResetBtn) {
        cameraResetBtn.addEventListener('click', () => {
            if (sceneManager) {
                sceneManager.resetCamera();
//                 console.log('📷 Camera reset');
            }
        });
    }

    // Populate category checkboxes (after gallery is initialized)
    setTimeout(() => {
        populateCategoryFilters();
    }, 500);
}

/**
 * Populate category filter checkboxes
 */
function populateCategoryFilters() {
    if (!sceneManager) return;

    const categories = sceneManager.getCategories();
    const container = document.getElementById('category-checkboxes');
    if (!container) return;

    container.innerHTML = '';

    categories.forEach(category => {
        const item = document.createElement('div');
        item.className = 'category-item';

        const checkbox = document.createElement('input');
        checkbox.type = 'checkbox';
        const safeCategoryId = category.replace(/[^a-z0-9-]/gi, '_');
        checkbox.id = `category-${safeCategoryId}`;
        checkbox.checked = true; // All visible by default

        const label = document.createElement('label');
        label.htmlFor = `category-${safeCategoryId}`;
        label.textContent = escapeHTML(category);

        // Category filter handler
        checkbox.addEventListener('change', () => {
            if (sceneManager) {
                sceneManager.setCategoryVisible(category, checkbox.checked);
            }
        });

        item.appendChild(checkbox);
        item.appendChild(label);
        container.appendChild(item);
    });

//     console.log('✅ Category filters populated:', categories.join(', '));
}

/**
 * Render templates list (preset + user templates)
 */
function renderTemplates() {
    const presetContainer = document.getElementById('preset-templates');
    const userContainer = document.getElementById('user-templates');
    const noSaved = document.getElementById('no-saved-templates');

    // Render presets
    presetContainer.innerHTML = '';
    PRESET_TEMPLATES.forEach(tmpl => {
        const row = document.createElement('div');
        row.className = 'template-row';
        const nameSpan = document.createElement('span');
        nameSpan.className = 'template-name';
        nameSpan.textContent = tmpl.name;
        const descSpan = document.createElement('span');
        descSpan.className = 'template-desc';
        descSpan.textContent = tmpl.description;
        const loadBtn = document.createElement('button');
        loadBtn.className = 'template-load-btn';
        loadBtn.textContent = '[load]';
        loadBtn.addEventListener('click', () => loadTemplate(tmpl.state));
        row.appendChild(nameSpan);
        row.appendChild(descSpan);
        row.appendChild(loadBtn);
        presetContainer.appendChild(row);
    });

    // Render user templates
    const userTemplates = loadUserTemplates();
    if (userTemplates.length === 0) {
        noSaved.style.display = '';
        userContainer.innerHTML = '';
    } else {
        noSaved.style.display = 'none';
        userContainer.innerHTML = '';
        userTemplates.forEach(tmpl => {
            const row = document.createElement('div');
            row.className = 'template-row';
            const date = new Date(tmpl.createdAt).toLocaleDateString();
            const nameSpan = document.createElement('span');
            nameSpan.className = 'template-name';
            nameSpan.textContent = tmpl.name;
            const descSpan = document.createElement('span');
            descSpan.className = 'template-desc';
            descSpan.textContent = date;
            const loadBtn = document.createElement('button');
            loadBtn.className = 'template-load-btn';
            loadBtn.textContent = '[load]';
            loadBtn.addEventListener('click', () => loadTemplate(tmpl.state));
            const delBtn = document.createElement('button');
            delBtn.className = 'template-del-btn';
            delBtn.textContent = '[del]';
            delBtn.addEventListener('click', () => {
                if (confirm('Delete template "' + escapeHTML(tmpl.name) + '"?')) {
                    deleteUserTemplate(tmpl.id);
                    renderTemplates();
                }
            });
            row.appendChild(nameSpan);
            row.appendChild(descSpan);
            row.appendChild(loadBtn);
            row.appendChild(delBtn);
            userContainer.appendChild(row);
        });
    }
}

/**
 * Load template (from preset or user template)
 */
function loadTemplate(tmplState) {
    Object.assign(state, tmplState);

    // Sync UI
    document.getElementById('ledRed').value = state.ledRed;
    document.getElementById('ledRedVal').textContent = state.ledRed;
    document.getElementById('ledGreen').value = state.ledGreen;
    document.getElementById('ledGreenVal').textContent = state.ledGreen;
    document.getElementById('ledBlue').value = state.ledBlue;
    document.getElementById('ledBlueVal').textContent = state.ledBlue;
    document.getElementById('hourlyChimeTune').value = state.hourlyChimeTune;
    document.getElementById('alarmTune').value = state.alarmTune;
    document.getElementById('smartAlarmTune').value = state.smartAlarmTune;

    setToggle('toggle24h', 'label24h', state.clock24h);
    setToggle('toggleBtn', 'labelBtn', state.btnSound);
    setToggle('toggleSmoothLED', 'labelSmoothLED', state.smoothLEDFade);

    updateLEDPreview();
    updateLEDFadePreview();

    setToggle('toggleAhEn', 'labelAhEn', state.activeHoursEnabled);
    document.getElementById('ahStart').innerHTML = buildTimeOptions(state.activeHoursStart);
    document.getElementById('ahEnd').innerHTML = buildTimeOptions(state.activeHoursEnd);
    updateAhTimeVisibility();

    renderBoards();
    renderDisplays();
    renderBranches();
    renderActiveFaces();
    renderAvailFaces();
    updateHash();
}

/**
 * Render active faces list (placeholder for now)
 */
function renderActiveFaces() {
    updateFlashUsage();
    // Full implementation requires Sortable.js integration
//     console.log('Active faces:', state.activeFaces);
}

/**
 * Render available faces list (placeholder for now)
 */
function renderAvailFaces() {
    // Full implementation requires face registry iteration and Sortable.js
//     console.log('Available faces loaded from registry');
}

// Bootstrap the application
init().catch(console.error);
