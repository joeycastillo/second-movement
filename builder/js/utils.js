// ============================================================
// UTILITY FUNCTIONS
// ============================================================

/**
 * Escape HTML special characters to prevent XSS
 */
export function escapeHTML(str) {
    return String(str)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#39;');
}

/**
 * Get GitHub token from sessionStorage or input field
 */
export function getToken() {
    return document.getElementById('githubToken').value.trim();
}

/**
 * Debounce function to limit execution frequency
 */
export function debounce(func, wait) {
    let timeout;
    return function executedFunction(...args) {
        const later = () => {
            clearTimeout(timeout);
            func(...args);
        };
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
    };
}

/**
 * Generate unique build ID (timestamp-based)
 */
export function genBuildId() {
    return 'build-' + Date.now() + '-' + Math.random().toString(36).substr(2, 9);
}

/**
 * Copy current URL to clipboard
 */
export function copyLink(updateHashFn) {
    updateHashFn();
    const url = window.location.href;
    navigator.clipboard.writeText(url).then(() => {
        const btn = document.getElementById('copyLinkBtn');
        const orig = btn.textContent;
        btn.textContent = '[copied!]';
        setTimeout(() => { btn.textContent = orig; }, 2000);
    }).catch(() => {
        // fallback for older browsers
        const ta = document.createElement('textarea');
        ta.value = url;
        ta.style.position = 'fixed';
        ta.style.opacity = '0';
        document.body.appendChild(ta);
        ta.select();
        document.execCommand('copy');
        document.body.removeChild(ta);
        const btn = document.getElementById('copyLinkBtn');
        btn.textContent = '[copied!]';
        setTimeout(() => { btn.textContent = '[copy link]'; }, 2000);
    });
}
