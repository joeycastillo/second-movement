// ============================================================
// THREE.JS SCENE MANAGER
// Foundation for Phase 1 - Basic scene setup, no rendering yet
// ============================================================

import * as THREE from 'three';

export class SceneManager {
    constructor(canvas) {
        this.canvas = canvas;
        this.scene = null;
        this.camera = null;
        this.renderer = null;
        this.animationFrameId = null;
    }

    /**
     * Initialize Three.js scene
     * Phase 1: Basic setup only - black canvas placeholder
     */
    init() {
        // Scene
        this.scene = new THREE.Scene();
        this.scene.background = new THREE.Color(0x0B0900); // --color-bg

        // Camera
        const aspect = window.innerWidth / window.innerHeight;
        this.camera = new THREE.PerspectiveCamera(75, aspect, 0.1, 1000);
        this.camera.position.z = 5;

        // Renderer
        this.renderer = new THREE.WebGLRenderer({
            canvas: this.canvas,
            antialias: true,
            alpha: false
        });
        this.renderer.setSize(window.innerWidth, window.innerHeight);
        this.renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));

        // Lighting (basic ambient for future use)
        const ambientLight = new THREE.AmbientLight(0xFFD060, 0.5); // Amber
        this.scene.add(ambientLight);

        console.log('✅ Three.js Scene Manager initialized (Phase 1 - placeholder)');

        // Setup resize handler
        window.addEventListener('resize', () => this.onWindowResize());

        // Start render loop
        this.animate();
    }

    /**
     * Handle window resize
     */
    onWindowResize() {
        if (!this.camera || !this.renderer) return;

        const aspect = window.innerWidth / window.innerHeight;
        this.camera.aspect = aspect;
        this.camera.updateProjectionMatrix();

        this.renderer.setSize(window.innerWidth, window.innerHeight);
    }

    /**
     * Animation loop
     */
    animate() {
        this.animationFrameId = requestAnimationFrame(() => this.animate());

        // Phase 1: Just render the empty scene (black canvas)
        if (this.renderer && this.scene && this.camera) {
            this.renderer.render(this.scene, this.camera);
        }
    }

    /**
     * Cleanup / dispose
     */
    dispose() {
        if (this.animationFrameId) {
            cancelAnimationFrame(this.animationFrameId);
        }

        if (this.renderer) {
            this.renderer.dispose();
        }

        window.removeEventListener('resize', () => this.onWindowResize());
        console.log('Scene Manager disposed');
    }
}

/**
 * Detect device tier for performance optimization
 * (For future phases - not used in Phase 1)
 */
export function getDeviceTier() {
    const ua = navigator.userAgent;

    const isMobile = /iPhone|iPod|Android(?!.*Tablet)/i.test(ua);
    const isTablet = /iPad|Android.*Tablet/i.test(ua);

    if (isMobile) {
        console.log('🔍 Device: Mobile → LOW tier');
        return 'low';
    }

    if (isTablet) {
        console.log('🔍 Device: Tablet → MEDIUM tier');
        return 'medium';
    }

    const memory = navigator.deviceMemory || 4;
    const cores = navigator.hardwareConcurrency || 4;

    if (memory < 4 || cores < 4) {
        console.log('🔍 Device: Desktop (low-spec) → MEDIUM tier');
        return 'medium';
    }

    console.log('🔍 Device: Desktop (high-spec) → HIGH tier');
    return 'high';
}
