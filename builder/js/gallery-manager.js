// ============================================================
// GALLERY MANAGER - 3D Face Gallery orchestration
// Phase 3: Spatial layout, category clustering, camera management
// ============================================================

import * as THREE from 'three';
import { FaceCard3D } from './face-card-3d.js';
import { registry } from './state.js';
import { addFace } from './faces.js';

/**
 * GalleryManager - Manages 3D face card gallery
 * Handles spatial layout, category clustering, filtering, camera flythrough
 */
export class GalleryManager {
    constructor(scene, camera, controls) {
        this.scene = scene;
        this.camera = camera;
        this.controls = controls;
        
        this.faceCards = [];
        this.categories = new Map(); // category -> cards[]
        this.selectedCard = null;
        
        // Layout configuration
        this.cardsPerRow = 5;
        this.cardSpacing = 4; // Units between cards
        this.categorySpacing = 8; // Extra space between categories
        this.zDepthVariation = 2; // Random z-depth for visual interest
        
        // Camera positions
        this.defaultCameraPosition = new THREE.Vector3(0, 8, 20);
        this.defaultCameraTarget = new THREE.Vector3(0, 0, 0);
        
        // Animation state
        this.cameraAnimating = false;
        this.cameraAnimationProgress = 0;
        this.cameraStartPos = new THREE.Vector3();
        this.cameraEndPos = new THREE.Vector3();
        this.cameraStartTarget = new THREE.Vector3();
        this.cameraEndTarget = new THREE.Vector3();
        
        // Category visibility
        this.visibleCategories = new Set();
    }

    /**
     * Initialize gallery from face registry
     */
    init() {
        if (!registry || !registry.faces) {
            console.error('❌ Gallery Manager: No face registry available');
            return;
        }

//         console.log('🎨 Gallery Manager: Initializing with', registry.faces.length, 'faces');

        // Group faces by category
        const facesByCategory = this.groupByCategory(registry.faces);
        
        // Generate category order (consistent with UI)
        const categoryOrder = this.getCategoryOrder(facesByCategory);
        
        // Initialize visible categories (all visible by default)
        categoryOrder.forEach(cat => this.visibleCategories.add(cat));
        
        // Create 3D cards with spatial layout
        this.createGalleryLayout(facesByCategory, categoryOrder);
        
//         console.log('✅ Gallery Manager: Created', this.faceCards.length, 'face cards');
//         console.log('   - Categories:', categoryOrder.join(', '));
    }

    /**
     * Group faces by category
     */
    groupByCategory(faces) {
        const grouped = new Map();
        
        faces.forEach(face => {
            const category = face.category || 'misc';
            if (!grouped.has(category)) {
                grouped.set(category, []);
            }
            grouped.get(category).push(face);
        });
        
        return grouped;
    }

    /**
     * Get category order (sorted by name, with priority categories first)
     */
    getCategoryOrder(facesByCategory) {
        const priorityOrder = ['clock', 'complication', 'sensor', 'settings'];
        const categories = Array.from(facesByCategory.keys());
        
        // Separate priority and other categories
        const priority = categories.filter(cat => priorityOrder.includes(cat));
        const others = categories.filter(cat => !priorityOrder.includes(cat));
        
        // Sort priority by defined order
        priority.sort((a, b) => priorityOrder.indexOf(a) - priorityOrder.indexOf(b));
        
        // Sort others alphabetically
        others.sort();
        
        return [...priority, ...others];
    }

    /**
     * Create 3D gallery layout with category clustering
     */
    createGalleryLayout(facesByCategory, categoryOrder) {
        let currentX = 0;
        let currentY = 0;
        let currentRow = 0;
        
        categoryOrder.forEach((category, categoryIndex) => {
            const faces = facesByCategory.get(category);
            if (!faces || faces.length === 0) return;
            
            // Create cards for this category
            const categoryCards = [];
            
            faces.forEach((face, faceIndex) => {
                // Calculate position
                const col = currentRow % this.cardsPerRow;
                const row = Math.floor(currentRow / this.cardsPerRow);
                
                const x = currentX + col * this.cardSpacing;
                const y = currentY - row * this.cardSpacing;
                
                // Random z-depth for visual interest
                const z = (Math.random() - 0.5) * this.zDepthVariation;
                
                const position = new THREE.Vector3(x, y, z);
                
                // Create card
                const card = new FaceCard3D(face, position);
                this.scene.add(card.getObject3D());
                
                this.faceCards.push(card);
                categoryCards.push(card);
                
                currentRow++;
            });
            
            // Store category mapping
            this.categories.set(category, categoryCards);
            
            // Add category spacing (move to next row, with extra vertical gap)
            if (categoryIndex < categoryOrder.length - 1) {
                const lastRow = Math.floor(currentRow / this.cardsPerRow);
                currentY -= (lastRow + 1) * this.cardSpacing + this.categorySpacing;
                currentRow = 0;
            }
        });
        
        // Center the entire gallery
        this.centerGallery();
    }

    /**
     * Center gallery around origin
     */
    centerGallery() {
        if (this.faceCards.length === 0) return;
        
        // Calculate bounding box
        const box = new THREE.Box3();
        this.faceCards.forEach(card => {
            const obj = card.getObject3D();
            box.expandByObject(obj);
        });
        
        const center = box.getCenter(new THREE.Vector3());
        
        // Offset all cards
        this.faceCards.forEach(card => {
            const obj = card.getObject3D();
            obj.position.x -= center.x;
            obj.position.y -= center.y;
            // Don't center z (preserve depth variation)
        });
        
        // Update default camera target
        this.defaultCameraTarget.set(0, 0, 0);
    }

    /**
     * Update gallery (call every frame)
     */
    update(deltaTime = 0.016) {
        // Update all face cards
        this.faceCards.forEach(card => card.update(deltaTime));
        
        // Update camera animation
        if (this.cameraAnimating) {
            this.updateCameraAnimation(deltaTime);
        }
    }

    /**
     * Handle card selection (add to active list + camera flythrough)
     */
    selectCard(card) {
        if (!card) return;
        
//         console.log('🎯 Gallery: Selected face:', card.getFaceId());
        
        // Add to active face list
        addFace(card.getFaceId());
        
        // Visual feedback
        if (this.selectedCard) {
            this.selectedCard.setSelected(false);
        }
        this.selectedCard = card;
        card.setSelected(true);
        
        // Camera flythrough to card
        this.flyToCard(card);
    }

    /**
     * Deselect current card
     */
    deselectCard() {
        if (this.selectedCard) {
            this.selectedCard.setSelected(false);
            this.selectedCard = null;
        }
    }

    /**
     * Fly camera to card (smooth transition)
     */
    flyToCard(card) {
        const cardPos = card.getObject3D().position;
        
        // Calculate target camera position (in front of card)
        const offset = new THREE.Vector3(0, 0, 6);
        const targetPos = cardPos.clone().add(offset);
        
        // Start animation
        this.cameraStartPos.copy(this.camera.position);
        this.cameraEndPos.copy(targetPos);
        this.cameraStartTarget.copy(this.controls.target);
        this.cameraEndTarget.copy(cardPos);
        
        this.cameraAnimating = true;
        this.cameraAnimationProgress = 0;
    }

    /**
     * Return camera to overview position
     */
    resetCamera() {
        this.cameraStartPos.copy(this.camera.position);
        this.cameraEndPos.copy(this.defaultCameraPosition);
        this.cameraStartTarget.copy(this.controls.target);
        this.cameraEndTarget.copy(this.defaultCameraTarget);
        
        this.cameraAnimating = true;
        this.cameraAnimationProgress = 0;
        
        this.deselectCard();
    }

    /**
     * Update camera animation (smooth lerp)
     */
    updateCameraAnimation(deltaTime) {
        const animationSpeed = 2; // Units per second
        this.cameraAnimationProgress += animationSpeed * deltaTime;
        
        if (this.cameraAnimationProgress >= 1) {
            // Animation complete
            this.camera.position.copy(this.cameraEndPos);
            this.controls.target.copy(this.cameraEndTarget);
            this.cameraAnimating = false;
            this.cameraAnimationProgress = 0;
        } else {
            // Smooth easing (ease-in-out)
            const t = this.easeInOutCubic(this.cameraAnimationProgress);
            
            this.camera.position.lerpVectors(this.cameraStartPos, this.cameraEndPos, t);
            this.controls.target.lerpVectors(this.cameraStartTarget, this.cameraEndTarget, t);
        }
        
        this.controls.update();
    }

    /**
     * Easing function (cubic ease-in-out)
     */
    easeInOutCubic(t) {
        return t < 0.5
            ? 4 * t * t * t
            : 1 - Math.pow(-2 * t + 2, 3) / 2;
    }

    /**
     * Filter gallery by category (show/hide clusters)
     */
    setCategoryVisible(category, visible) {
        if (visible) {
            this.visibleCategories.add(category);
        } else {
            this.visibleCategories.delete(category);
        }
        
        // Update card visibility
        const cards = this.categories.get(category);
        if (cards) {
            cards.forEach(card => card.setVisible(visible));
        }
        
//         console.log(`🔍 Gallery: ${category} ${visible ? 'shown' : 'hidden'}`);
    }

    /**
     * Get all category names
     */
    getCategories() {
        return Array.from(this.categories.keys());
    }

    /**
     * Check if category is visible
     */
    isCategoryVisible(category) {
        return this.visibleCategories.has(category);
    }

    /**
     * Raycast check - find intersected card
     * Returns card or null
     */
    raycast(raycaster) {
        const objects = this.faceCards
            .filter(card => card.getObject3D().visible)
            .map(card => card.getObject3D());
        
        const intersects = raycaster.intersectObjects(objects, true);
        
        if (intersects.length > 0) {
            // Find the card from userData
            let obj = intersects[0].object;
            while (obj && !obj.userData.faceCard) {
                obj = obj.parent;
            }
            
            if (obj && obj.userData.faceCard) {
                return obj.userData.faceCard;
            }
        }
        
        return null;
    }

    /**
     * Clear hover states on all cards
     */
    clearHovers() {
        this.faceCards.forEach(card => card.setHover(false));
    }

    /**
     * Dispose gallery
     */
    dispose() {
        this.faceCards.forEach(card => {
            this.scene.remove(card.getObject3D());
            card.dispose();
        });
        
        this.faceCards = [];
        this.categories.clear();
        this.visibleCategories.clear();
        this.selectedCard = null;
        
//         console.log('✅ Gallery Manager disposed');
    }
}
