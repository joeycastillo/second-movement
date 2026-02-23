// ============================================================
// THREE.JS SCENE MANAGER - PHASE 2: ACTIVE 3D RENDERING
// Desktop split-pane 3D scene with cube + grid + OrbitControls
// ============================================================

import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

export class SceneManager {
    constructor(canvas) {
        this.canvas = canvas;
        this.scene = null;
        this.camera = null;
        this.renderer = null;
        this.controls = null;
        this.animationFrameId = null;
        
        // Objects
        this.testCube = null;
        this.gridFloor = null;
        
        // Rotation speed
        this.cubeRotationSpeed = 0.01;
    }

    /**
     * Initialize Three.js scene - Phase 2: Active rendering
     */
    init() {
        const container = this.canvas.parentElement;
        if (!container) {
            console.error('❌ Canvas container not found');
            return;
        }

        const width = container.clientWidth;
        const height = container.clientHeight;

        // Scene
        this.scene = new THREE.Scene();
        this.scene.background = new THREE.Color(0x0B0900); // --color-bg
        this.scene.fog = new THREE.Fog(0x0B0900, 10, 50); // Subtle depth fog

        // Camera
        const aspect = width / height;
        this.camera = new THREE.PerspectiveCamera(75, aspect, 0.1, 1000);
        this.camera.position.set(3, 3, 5);
        this.camera.lookAt(0, 0, 0);

        // Renderer
        this.renderer = new THREE.WebGLRenderer({
            canvas: this.canvas,
            antialias: true,
            alpha: false
        });
        this.renderer.setSize(width, height);
        this.renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));

        // Lighting - Subtle amber glow (per dlorp's preference)
        const ambientLight = new THREE.AmbientLight(0xFFD060, 0.3); // Very subtle amber
        this.scene.add(ambientLight);

        const directionalLight = new THREE.DirectionalLight(0xFFB000, 0.5);
        directionalLight.position.set(5, 5, 5);
        this.scene.add(directionalLight);

        // Subtle rim light (amber accent)
        const rimLight = new THREE.DirectionalLight(0xCC8800, 0.2);
        rimLight.position.set(-3, 2, -3);
        this.scene.add(rimLight);

        // Test cube - Low-poly wireframe aesthetic
        const cubeGeometry = new THREE.BoxGeometry(2, 2, 2);
        const cubeMaterial = new THREE.MeshStandardMaterial({
            color: 0xFFB000, // Amber bright
            wireframe: true,
            emissive: 0xCC8800, // Subtle amber glow
            emissiveIntensity: 0.2
        });
        this.testCube = new THREE.Mesh(cubeGeometry, cubeMaterial);
        this.testCube.position.y = 1; // Lift cube above grid
        this.scene.add(this.testCube);

        // Grid floor - 20x20 subtle amber lines
        const gridSize = 20;
        const gridDivisions = 20;
        const gridColorCenter = new THREE.Color(0x7A5200); // Amber dim
        const gridColorGrid = new THREE.Color(0x3D2900);   // Amber trace
        
        this.gridFloor = new THREE.GridHelper(
            gridSize,
            gridDivisions,
            gridColorCenter,
            gridColorGrid
        );
        this.gridFloor.position.y = 0;
        this.scene.add(this.gridFloor);

        // OrbitControls - Mouse camera control
        this.controls = new OrbitControls(this.camera, this.renderer.domElement);
        this.controls.enableDamping = true; // Smooth camera transitions
        this.controls.dampingFactor = 0.05;
        this.controls.minDistance = 3;
        this.controls.maxDistance = 20;
        this.controls.maxPolarAngle = Math.PI / 2 - 0.1; // Prevent going below floor
        this.controls.target.set(0, 1, 0); // Look at cube center

        console.log('✅ Three.js Scene Manager initialized (Phase 2 - Active Rendering)');
        console.log('   - Test cube: Rotating amber wireframe');
        console.log('   - Grid floor: 20x20 subtle amber lines');
        console.log('   - OrbitControls: Mouse drag, zoom enabled');

        // Setup resize handler
        window.addEventListener('resize', () => this.onWindowResize());

        // Mark container as loaded (remove loading text)
        container.classList.add('loaded');

        // Start render loop
        this.animate();
    }

    /**
     * Handle window resize
     */
    onWindowResize() {
        if (!this.camera || !this.renderer) return;

        const container = this.canvas.parentElement;
        if (!container) return;

        const width = container.clientWidth;
        const height = container.clientHeight;

        this.camera.aspect = width / height;
        this.camera.updateProjectionMatrix();

        this.renderer.setSize(width, height);
    }

    /**
     * Animation loop - 60 FPS target
     */
    animate() {
        this.animationFrameId = requestAnimationFrame(() => this.animate());

        // Rotate test cube
        if (this.testCube) {
            this.testCube.rotation.x += this.cubeRotationSpeed;
            this.testCube.rotation.y += this.cubeRotationSpeed * 1.5;
        }

        // Update controls (damping)
        if (this.controls) {
            this.controls.update();
        }

        // Render scene
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

        if (this.controls) {
            this.controls.dispose();
        }

        if (this.renderer) {
            this.renderer.dispose();
        }

        // Dispose geometries and materials
        if (this.testCube) {
            this.testCube.geometry.dispose();
            this.testCube.material.dispose();
        }

        if (this.gridFloor) {
            this.gridFloor.geometry.dispose();
            this.gridFloor.material.dispose();
        }

        window.removeEventListener('resize', () => this.onWindowResize());
        console.log('✅ Scene Manager disposed');
    }
}

/**
 * Detect device tier for performance optimization
 * Used for quality settings (not implemented in Phase 2 - desktop focus)
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
