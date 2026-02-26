// ============================================================
// BUILD SYSTEM - GitHub Actions API Integration
// ============================================================

import { state, REPO, WORKFLOW_ID, COOLDOWN_MS, POLL_INTERVAL_MS } from './state.js';
import { getToken, genBuildId } from './utils.js';

let buildId = null;
let pollTimer = null;
let cooldownTimer = null;

/**
 * Validate GitHub Personal Access Token
 */
export async function validateToken(token) {
    if (!token || token.length === 0) {
        return { valid: false, message: 'NO TOKEN' };
    }
    try {
        const resp = await fetch('https://api.github.com/user', {
            headers: { 'Authorization': 'Bearer ' + token }
        });
        if (resp.ok) {
            return { valid: true, message: 'TOKEN OK' };
        } else if (resp.status === 401) {
            return { valid: false, message: 'INVALID TOKEN' };
        } else {
            return { valid: false, message: 'CHECK FAILED: ' + resp.status };
        }
    } catch (e) {
        return { valid: false, message: 'NETWORK ERROR' };
    }
}

/**
 * Update token status UI
 */
export function updateTokenStatus(result) {
    const statusDiv = document.getElementById('tokenStatus');
    const indicator = document.getElementById('tokenIndicator');
    const label = document.getElementById('tokenLabel');

    statusDiv.style.display = 'block';
    indicator.textContent = result.valid ? '●' : '✗';
    indicator.style.color = result.valid ? 'var(--color-amber-bright)' : 'var(--color-critical)';
    label.textContent = result.message;
    label.style.color = result.valid ? 'var(--color-amber-mid)' : 'var(--color-critical)';
}

/**
 * Trigger custom firmware build via GitHub Actions workflow dispatch
 */
export async function triggerBuild() {
    const token = getToken();
    if (!token) {
        showStatus('[ERR] ENTER GITHUB PAT IN TOKEN FIELD ABOVE', 'error');
        return;
    }

    const faces = state.activeFaces.filter(id => id !== '__secondary__');
    if (faces.length === 0) {
        showStatus('[ERR] ADD AT LEAST ONE WATCH FACE', 'error');
        return;
    }

    const now = Date.now();
    const prevCooldownEnd = parseInt(localStorage.getItem('sm_cooldown_end') || '0', 10);
    if (now < prevCooldownEnd) {
        const remaining = Math.ceil((prevCooldownEnd - now) / 1000);
        showStatus('[WAIT] COOLDOWN: ' + remaining + 's REMAINING', 'error');
        return;
    }

    buildId = genBuildId();
    localStorage.setItem('sm_build_id', buildId);
    localStorage.removeItem('sm_build_result_' + buildId);

    const cooldownEnd = now + COOLDOWN_MS;
    localStorage.setItem('sm_cooldown_end', cooldownEnd.toString());
    startCooldownUI(COOLDOWN_MS);

    const secIdx = state.activeFaces.indexOf('__secondary__');
    const secondaryIndex = secIdx >= 0
        ? state.activeFaces.slice(0, secIdx).filter(id => id !== '__secondary__').length
        : faces.length;

    // Security: clamp zone inputs
    const clampZone = (val, def, min, max) => {
        const n = parseInt(val, 10);
        return (Number.isFinite(n) && n >= min && n <= max) ? n : def;
    };
    const zem = clampZone(state.zoneEmergenceMax || 25, 25, 15, 35);
    const zmm = clampZone(state.zoneMomentumMax || 50, 50, 40, 60);
    const zam = clampZone(state.zoneActiveMax || 75, 75, 65, 85);

    const inputs = {
        faces: faces.join(','),
        board: state.board,
        display: state.display,
        secondary_index: String(secondaryIndex),
        clock_24h: String(state.clock24h),
        led_red: String(state.ledRed),
        led_green: String(state.ledGreen),
        led_blue: String(state.ledBlue),
        smooth_led_fade: String(state.smoothLEDFade),
        button_sound: String(state.btnSound),
        hourly_chime_tune: state.hourlyChimeTune,
        alarm_tune: state.alarmTune,
        smart_alarm_tune: state.smartAlarmTune,
        active_hours_enabled: String(state.activeHoursEnabled),
        active_hours_start: String(state.activeHoursStart),
        active_hours_end: String(state.activeHoursEnd),
        build_id: buildId
    };

    if (state.homebaseLat !== null) {
        inputs.homebase_lat = String(state.homebaseLat);
    }
    if (state.homebaseLon !== null) {
        inputs.homebase_lon = String(state.homebaseLon);
    }
    if (state.homebaseTz !== null) {
        inputs.homebase_tz = String(state.homebaseTz);
    }

    if (zem !== 25) inputs.zone_emergence_max = String(zem);
    if (zmm !== 50) inputs.zone_momentum_max = String(zmm);
    if (zam !== 75) inputs.zone_active_max = String(zam);

    showStatus('DISPATCHING BUILD ' + buildId + '...', '');
    document.getElementById('buildBtn').disabled = true;
    document.getElementById('downloadBtn').classList.remove('active');

    try {
        const resp = await fetch(
            'https://api.github.com/repos/' + REPO + '/actions/workflows/' + WORKFLOW_ID + '/dispatches',
            {
                method: 'POST',
                headers: {
                    'Authorization': 'Bearer ' + token,
                    'Accept': 'application/vnd.github+json',
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ ref: state.branch, inputs })
            }
        );

        if (resp.status === 204) {
            showStatus('BUILD QUEUED. WAITING FOR RUNNER...', '');
            setTimeout(pollBuild, 4000);
        } else {
            const body = await resp.text();
            showStatus('[ERR] DISPATCH FAILED: ' + resp.status + ' ' + body.slice(0, 120), 'error');
        }
    } catch (e) {
        showStatus('[ERR] NETWORK ERROR: ' + e.message, 'error');
    }
}

/**
 * Poll GitHub Actions API for build status
 */
export async function pollBuild() {
    if (!buildId) return;
    clearTimeout(pollTimer);

    const token = getToken();
    const headers = {
        'Authorization': token ? 'Bearer ' + token : '',
        'Accept': 'application/vnd.github+json'
    };

    try {
        const resp = await fetch(
            'https://api.github.com/repos/' + REPO + '/actions/runs?event=workflow_dispatch&per_page=5',
            { headers }
        );
        const data = await resp.json();
        const runs = data.workflow_runs || [];

        let run = runs.find(r => r.name && r.name.includes(buildId));
        if (!run && runs.length > 0) run = runs[0];

        if (!run) {
            showStatus('WAITING FOR RUNNER... (BUILD ' + buildId + ')', '');
            pollTimer = setTimeout(pollBuild, POLL_INTERVAL_MS);
            return;
        }

        const status = run.status;
        const conclusion = run.conclusion;

        let elapsed = '';
        if (run.created_at) {
            const created = new Date(run.created_at).getTime();
            const secs = Math.round((Date.now() - created) / 1000);
            elapsed = ' ' + secs + 's';
        }

        if (status === 'completed') {
            if (conclusion === 'success') {
                const buildName = state.board + '-' + state.display + '-' + buildId;
                const uf2Url = 'https://github.com/' + REPO + '/releases/download/custom-builds/' + buildName + '.uf2';
                localStorage.setItem('sm_build_result_' + buildId, 'success:' + uf2Url);
                showStatus('[OK] BUILD COMPLETE' + elapsed + ' - READY TO DOWNLOAD', 'success');
                const dlBtn = document.getElementById('downloadBtn');
                dlBtn.href = uf2Url;
                dlBtn.classList.add('active');
            } else {
                localStorage.setItem('sm_build_result_' + buildId, 'fail:' + conclusion);
                showStatus('[ERR] BUILD ' + (conclusion || 'FAILED').toUpperCase() + elapsed, 'error');
            }
            document.getElementById('buildBtn').disabled = false;
            buildId = null;
        } else {
            const statusLabel = status === 'in_progress' ? 'BUILDING' : 'QUEUED';
            showStatus(statusLabel + '...' + elapsed, '');
            pollTimer = setTimeout(pollBuild, POLL_INTERVAL_MS);
        }
    } catch (e) {
        showStatus('[ERR] POLL ERROR: ' + e.message, 'error');
        pollTimer = setTimeout(pollBuild, POLL_INTERVAL_MS);
    }
}

/**
 * Show build status message
 */
export function showStatus(msg, type) {
    const el = document.getElementById('buildStatus');
    el.className = 'build-status active' + (type ? ' ' + type : '');
    el.textContent = msg;
}

/**
 * Display cooldown timer UI
 */
export function startCooldownUI(durationMs) {
    const bar = document.getElementById('cooldownBar');
    const fill = document.getElementById('cooldownFill');
    bar.classList.add('active');
    fill.style.transition = 'none';
    fill.style.width = '100%';

    void fill.offsetWidth; // force reflow

    fill.style.transition = 'width ' + (durationMs / 1000) + 's linear';
    fill.style.width = '0%';

    clearTimeout(cooldownTimer);
    cooldownTimer = setTimeout(() => {
        bar.classList.remove('active');
        document.getElementById('buildBtn').disabled = false;
    }, durationMs);
}
