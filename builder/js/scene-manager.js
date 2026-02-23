// ============================================================
// THREE.JS SCENE MANAGER - PHASE 3: 3D FACE GALLERY
// Multi-scene system: Face Gallery, Build Status, RetroPass
// Raycasting for mouse interaction
// ============================================================

import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { GalleryManager } from './gallery-manager.js';

export class SceneManager {
    constructor(canvas) {
        this.canvas = canvas;
        this.scene = null;
        this.camera = null;
        this.renderer = null;
        this.controls = null;
        this.animationFrameId = null;
        
        // Phase 3: Gallery Manager
        this.galleryManager = null;
        
        // Phase 2: Legacy objects (Build Status scene)
        this.testCube = null;
        this.gridFloor = null;
        
        // Scene modes
        this.currentScene = 'gallery'; // 'gallery', 'build', 'retropass'
        
        // Raycasting
        this.raycaster = new THREE.Raycaster();
        this.mouse = new THREE.Vector2();
        this.hoveredCard = null;
        this.mouseMoveLast = 0;
        this.mouseMoveThrottle = 50; // ms
        
        // Animation frame limiting (DoS prevention)
        this.frameCount = 0;
        this.frameLimit = 10000; // Reset every 10k frames (~2.7 min at 60fps)
        this.lastFrameTime = 0;
        
        // Rotation speed (Build Status scene)
        this.cubeRotationSpeed = 0.01;
    }

    /**
     * Initialize Three.js scene - Phase 3: Face Gallery + Multi-scene
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
        this.scene.fog = new THREE.Fog(0x0B0900, 20, 60); // Depth fog for gallery

        // Camera
        const aspect = width / height;
        this.camera = new THREE.PerspectiveCamera(75, aspect, 0.1, 1000);
        this.camera.position.set(0, 8, 20); // Gallery overview position
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
        const ambientLight = new THREE.AmbientLight(0xFFD060, 0.4); // Very subtle amber
        this.scene.add(ambientLight);

        const directionalLight = new THREE.DirectionalLight(0xFFB000, 0.6);
        directionalLight.position.set(10, 10, 10);
        this.scene.add(directionalLight);

        // Subtle rim light (amber accent)
        const rimLight = new THREE.DirectionalLight(0xCC8800, 0.3);
        rimLight.position.set(-5, 5, -5);
        this.scene.add(rimLight);

        // Build Status scene objects (hidden initially)
        this.createBuildStatusScene();

        // OrbitControls - Mouse camera control
        this.controls = new OrbitControls(this.camera, this.renderer.domElement);
        this.controls.enableDamping = true; // Smooth camera transitions
        this.controls.dampingFactor = 0.05;
        this.controls.minDistance = 5;
        this.controls.maxDistance = 40;
        this.controls.maxPolarAngle = Math.PI / 2 + 0.3; // Allow some below horizon
        this.controls.target.set(0, 0, 0);

        // Phase 3: Initialize Gallery Manager
        this.galleryManager = new GalleryManager(this.scene, this.camera, this.controls);
        this.galleryManager.init();

        // Setup mouse event handlers
        this.setupMouseHandlers();

        console.log('✅ Three.js Scene Manager initialized (Phase 3 - Face Gallery)');
        console.log('   - Gallery Manager: Face cards loaded');
        console.log('   - Raycasting: Mouse interaction enabled');
        console.log('   - Scene modes: Gallery | Build Status | RetroPass');

        // Setup resize handler
        window.addEventListener('resize', () => this.onWindowResize());

        // Mark container as loaded (remove loading text)
        container.classList.add('loaded');

        // Start in gallery mode
        this.setSceneMode('gallery');

        // Start render loop
        this.animate();
    }

    /**
     * Create Build Status scene objects (Phase 2 legacy)
     */
    createBuildStatusScene() {
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
        this.testCube.visible = false; // Hidden initially
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
        this.gridFloor.visible = false; // Hidden initially
        this.scene.add(this.gridFloor);
    }

    /**
     * Setup mouse event handlers for raycasting
     */
    setupMouseHandlers() {
        // Mouse move (throttled)
        this.canvas.addEventListener('mousemove', (event) => {
            const now = Date.now();
            if (now - this.mouseMoveLast < this.mouseMoveThrottle) return;
            this.mouseMoveLast = now;

            this.onMouseMove(event);
        });

        // Mouse click
        this.canvas.addEventListener('click', (event) => {
            this.onMouseClick(event);
        });

        // Cursor style management
        this.canvas.style.cursor = 'default';
    }

    /**
     * Handle mouse move (hover detection)
     */
    onMouseMove(event) {
        if (this.currentScene !== 'gallery') return;

        // Calculate mouse position in normalized device coordinates
        const rect = this.canvas.getBoundingClientRect();
        this.mouse.x = ((event.clientX - rect.left) / rect.width) * 2 - 1;
        this.mouse.y = -((event.clientY - rect.top) / rect.height) * 2 + 1;

        // Update raycaster
        this.raycaster.setFromCamera(this.mouse, this.camera);

        // Check for card intersections
        const intersectedCard = this.galleryManager.raycast(this.raycaster);

        // Update hover state
        if (intersectedCard !== this.hoveredCard) {
            // Clear previous hover
            if (this.hoveredCard) {
                this.hoveredCard.setHover(false);
            }

            // Set new hover
            this.hoveredCard = intersectedCard;
            if (this.hoveredCard) {
                this.hoveredCard.setHover(true);
                this.canvas.style.cursor = 'pointer';
            } else {
                this.canvas.style.cursor = 'default';
            }
        }
    }

    /**
     * Handle mouse click (card selection)
     */
    onMouseClick(event) {
        if (this.currentScene !== 'gallery') return;

        // Use current mouse position
        this.raycaster.setFromCamera(this.mouse, this.camera);

        // Check for card intersections
        const clickedCard = this.galleryManager.raycast(this.raycaster);

        if (clickedCard) {
            this.galleryManager.selectCard(clickedCard);
        }
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

        // Frame limiting (DoS prevention)
        this.frameCount++;
        if (this.frameCount >= this.frameLimit) {
            console.log('⚠️  Frame limit reached, resetting counter');
            this.frameCount = 0;
        }

        // Calculate delta time
        const now = performance.now();
        const deltaTime = this.lastFrameTime ? (now - this.lastFrameTime) / 1000 : 0.016;
        this.lastFrameTime = now;

        // Update current scene
        if (this.currentScene === 'gallery' && this.galleryManager) {
            this.galleryManager.update(deltaTime);
        } else if (this.currentScene === 'build') {
            // Rotate test cube (Build Status scene)
            if (this.testCube && this.testCube.visible) {
                this.testCube.rotation.x += this.cubeRotationSpeed;
                this.testCube.rotation.y += this.cubeRotationSpeed * 1.5;
            }
        }
        // RetroPass scene update would go here (Phase 5)

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
     * Switch scene mode (Gallery / Build Status / RetroPass)
     */
    setSceneMode(mode) {
        if (this.currentScene === mode) return;

        console.log(`🎬 Switching scene: ${this.currentScene} → ${mode}`);
        this.currentScene = mode;

        // Hide all scene objects
        if (this.testCube) this.testCube.visible = false;
        if (this.gridFloor) this.gridFloor.visible = false;

        // Show relevant objects
        if (mode === 'gallery') {
            // Gallery is always visible (managed by GalleryManager)
            // Reset camera
            if (this.galleryManager) {
                this.galleryManager.resetCamera();
            }
        } else if (mode === 'build') {
            if (this.testCube) this.testCube.visible = true;
            if (this.gridFloor) this.gridFloor.visible = true;

            // Move camera to build view
            this.camera.position.set(3, 3, 5);
            this.controls.target.set(0, 1, 0);
            this.controls.update();
        } else if (mode === 'retropass') {
            // RetroPass scene (Phase 5)
            console.log('⏰ RetroPass scene not yet implemented');
        }

        // Clear hover state when switching
        if (this.hoveredCard) {
            this.hoveredCard.setHover(false);
            this.hoveredCard = null;
        }
        this.canvas.style.cursor = 'default';
    }

    /**
     * Get current scene mode
     */
    getSceneMode() {
        return this.currentScene;
    }

    /**
     * Reset camera to current scene's default view
     */
    resetCamera() {
        if (this.currentScene === 'gallery' && this.galleryManager) {
            this.galleryManager.resetCamera();
        } else if (this.currentScene === 'build') {
            this.camera.position.set(3, 3, 5);
            this.controls.target.set(0, 1, 0);
            this.controls.update();
        }
    }

    /**
     * Set category filter (Gallery mode only)
     */
    setCategoryVisible(category, visible) {
        if (this.galleryManager) {
            this.galleryManager.setCategoryVisible(category, visible);
        }
    }

    /**
     * Get all categories (Gallery mode)
     */
    getCategories() {
        return this.galleryManager ? this.galleryManager.getCategories() : [];
    }

    /**
     * Cleanup / dispose
     */
    dispose() {
        if (this.animationFrameId) {
            cancelAnimationFrame(this.animationFrameId);
        }

        // Dispose gallery manager
        if (this.galleryManager) {
            this.galleryManager.dispose();
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

        // Remove event listeners
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
