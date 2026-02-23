// ============================================================
// FACE CARD 3D - Individual 3D face card representation
// Phase 3: Low-poly text-based cards with amber wireframe
// ============================================================

import * as THREE from 'three';
import { escapeHTML } from './utils.js';

/**
 * FaceCard3D - Individual 3D representation of a watch face
 * Text-based (no thumbnails), amber wireframe aesthetic
 */
export class FaceCard3D {
    constructor(faceData, position = new THREE.Vector3(0, 0, 0)) {
        this.faceData = faceData;
        this.position = position;
        this.group = new THREE.Group();
        this.isHovered = false;
        this.isSelected = false;
        
        // Card dimensions
        this.width = 2.5;
        this.height = 3.5;
        this.depth = 0.1;
        
        // Visual elements
        this.cardMesh = null;
        this.textCanvas = null;
        this.textTexture = null;
        this.glowMaterial = null;
        
        // Hover animation state
        this.hoverProgress = 0;
        this.targetHoverProgress = 0;
        
        this.init();
    }

    /**
     * Initialize card geometry and materials
     */
    init() {
        // Card geometry - low-poly box
        const cardGeometry = new THREE.BoxGeometry(this.width, this.height, this.depth);
        
        // Create text texture
        this.createTextTexture();
        
        // Card material - amber wireframe base + text overlay
        this.glowMaterial = new THREE.MeshBasicMaterial({
            color: 0xFFB000, // Amber bright
            wireframe: true,
            transparent: true,
            opacity: 0.6
        });
        
        this.cardMesh = new THREE.Mesh(cardGeometry, this.glowMaterial);
        this.group.add(this.cardMesh);
        
        // Add text plane (front face)
        if (this.textTexture) {
            const textGeometry = new THREE.PlaneGeometry(this.width - 0.2, this.height - 0.2);
            const textMaterial = new THREE.MeshBasicMaterial({
                map: this.textTexture,
                transparent: true,
                side: THREE.DoubleSide
            });
            
            const textMesh = new THREE.Mesh(textGeometry, textMaterial);
            textMesh.position.z = this.depth / 2 + 0.01; // Slightly in front
            this.group.add(textMesh);
            
            // Clone for back side
            const textMeshBack = textMesh.clone();
            textMeshBack.position.z = -this.depth / 2 - 0.01;
            textMeshBack.rotation.y = Math.PI;
            this.group.add(textMeshBack);
        }
        
        // Position the group
        this.group.position.copy(this.position);
        
        // Store face data reference for raycasting
        this.group.userData = {
            faceCard: this,
            faceId: this.faceData.id,
            faceName: this.faceData.name,
            category: this.faceData.category
        };
    }

    /**
     * Create text texture using Canvas 2D API
     */
    createTextTexture() {
        const canvas = document.createElement('canvas');
        canvas.width = 512;
        canvas.height = 712; // Aspect ratio matches card (3.5/2.5 * 512)
        
        const ctx = canvas.getContext('2d');
        if (!ctx) return;
        
        // Background (semi-transparent dark)
        ctx.fillStyle = 'rgba(11, 9, 0, 0.85)';
        ctx.fillRect(0, 0, canvas.width, canvas.height);
        
        // Text styling - amber on dark
        ctx.fillStyle = '#FFD060'; // Amber hot
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        
        // Face name (large)
        ctx.font = 'bold 48px "Share Tech Mono", monospace';
        const name = escapeHTML(this.faceData.name || 'Unknown');
        this.wrapText(ctx, name, canvas.width / 2, 40, canvas.width - 80, 60);
        
        // Category badge
        ctx.fillStyle = '#CC8800'; // Amber mid
        ctx.font = '28px "Share Tech Mono", monospace';
        const category = escapeHTML(this.faceData.category || 'misc').toUpperCase();
        ctx.fillText(category, canvas.width / 2, 160);
        
        // Separator line
        ctx.strokeStyle = '#7A5200'; // Amber dim
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(80, 220);
        ctx.lineTo(canvas.width - 80, 220);
        ctx.stroke();
        
        // Description (wrapped, smaller text)
        ctx.fillStyle = '#FFB000'; // Amber bright
        ctx.font = '22px "Share Tech Mono", monospace';
        ctx.textAlign = 'left';
        const description = escapeHTML(this.faceData.description || 'No description');
        this.wrapText(ctx, description, 40, 260, canvas.width - 80, 30);
        
        // Flash size (if available)
        if (this.faceData.flash_bytes) {
            const kb = (this.faceData.flash_bytes / 1024).toFixed(1);
            ctx.fillStyle = '#FFD060'; // Amber hot
            ctx.font = 'bold 32px "Share Tech Mono", monospace';
            ctx.textAlign = 'center';
            ctx.fillText(`${kb} KB`, canvas.width / 2, canvas.height - 80);
            
            // Flash usage bar
            const barWidth = canvas.width - 120;
            const barHeight = 20;
            const barX = 60;
            const barY = canvas.height - 40;
            
            // Background bar
            ctx.fillStyle = '#3D2900'; // Amber trace
            ctx.fillRect(barX, barY, barWidth, barHeight);
            
            // Usage bar (assume 230KB limit)
            const usage = Math.min(this.faceData.flash_bytes / (230 * 1024), 1);
            const fillWidth = barWidth * usage;
            const fillColor = usage > 0.8 ? '#FF6B00' : '#FFB000';
            ctx.fillStyle = fillColor;
            ctx.fillRect(barX, barY, fillWidth, barHeight);
            
            // Border
            ctx.strokeStyle = '#7A5200';
            ctx.lineWidth = 2;
            ctx.strokeRect(barX, barY, barWidth, barHeight);
        }
        
        // Create texture
        this.textCanvas = canvas;
        this.textTexture = new THREE.CanvasTexture(canvas);
        this.textTexture.minFilter = THREE.LinearFilter;
        this.textTexture.magFilter = THREE.LinearFilter;
    }

    /**
     * Wrap text to fit within width
     */
    wrapText(ctx, text, x, y, maxWidth, lineHeight) {
        const words = text.split(' ');
        let line = '';
        let currentY = y;
        
        for (let i = 0; i < words.length; i++) {
            const testLine = line + words[i] + ' ';
            const metrics = ctx.measureText(testLine);
            const testWidth = metrics.width;
            
            if (testWidth > maxWidth && i > 0) {
                ctx.fillText(line, x, currentY);
                line = words[i] + ' ';
                currentY += lineHeight;
            } else {
                line = testLine;
            }
        }
        ctx.fillText(line, x, currentY);
    }

    /**
     * Set hover state (triggers animation)
     */
    setHover(hovered) {
        this.isHovered = hovered;
        this.targetHoverProgress = hovered ? 1 : 0;
    }

    /**
     * Set selected state (visual distinction)
     */
    setSelected(selected) {
        this.isSelected = selected;
        if (this.glowMaterial) {
            if (selected) {
                this.glowMaterial.color.setHex(0xFFD060); // Amber hot
                this.glowMaterial.opacity = 0.9;
            } else {
                this.glowMaterial.color.setHex(0xFFB000); // Amber bright
                this.glowMaterial.opacity = this.isHovered ? 0.8 : 0.6;
            }
        }
    }

    /**
     * Update animation (call every frame)
     */
    update(deltaTime = 0.016) {
        // Smooth hover animation
        const lerpSpeed = 8;
        this.hoverProgress += (this.targetHoverProgress - this.hoverProgress) * lerpSpeed * deltaTime;
        
        // Apply hover effects
        if (this.glowMaterial && !this.isSelected) {
            const baseOpacity = 0.6;
            const hoverOpacity = 0.8;
            this.glowMaterial.opacity = baseOpacity + (hoverOpacity - baseOpacity) * this.hoverProgress;
        }
        
        // Subtle float animation
        const floatSpeed = 0.5;
        const floatAmount = 0.05;
        this.group.position.y = this.position.y + Math.sin(Date.now() * 0.001 * floatSpeed) * floatAmount;
        
        // Hover lift
        const hoverLift = 0.2;
        this.group.position.y += hoverLift * this.hoverProgress;
    }

    /**
     * Get the Three.js group (add to scene)
     */
    getObject3D() {
        return this.group;
    }

    /**
     * Cleanup / dispose
     */
    dispose() {
        if (this.textTexture) {
            this.textTexture.dispose();
        }
        
        if (this.cardMesh) {
            this.cardMesh.geometry.dispose();
            this.cardMesh.material.dispose();
        }
        
        // Dispose all children
        this.group.traverse((child) => {
            if (child.geometry) child.geometry.dispose();
            if (child.material) {
                if (Array.isArray(child.material)) {
                    child.material.forEach(mat => mat.dispose());
                } else {
                    child.material.dispose();
                }
            }
        });
    }

    /**
     * Show/hide card (category filtering)
     */
    setVisible(visible) {
        this.group.visible = visible;
    }

    /**
     * Get face ID
     */
    getFaceId() {
        return this.faceData.id;
    }

    /**
     * Get category
     */
    getCategory() {
        return this.faceData.category || 'misc';
    }
}
