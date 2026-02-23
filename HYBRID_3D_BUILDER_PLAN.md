# Hybrid 3D Builder Implementation Plan

**Vision:** Transform the sensor-watch builder into a hybrid 3D interface combining practical HTML controls with Three.js visualizations, aligned with the chronomantic instrument aesthetic.

## Architecture Overview

### HTML/CSS Layer (Left Panel - Desktop)
- Configuration forms and inputs
- Face selection controls
- Build triggers and settings
- Traditional DOM for usability

### Three.js Layer (Right Panel - Desktop)
- **3D Face Gallery**: Floating modules in space, camera navigation
- **Build Status Dashboard**: 3D progress visualization, real-time updates
- **Enhanced RetroPass**: Integration with existing 3D watch model

### Aesthetic Direction
- **Chronomantic instrument theme**: Time-aware, mystical tech
- **Demoscene style**: Low-poly, procedural, efficient
- **Amber CRT glow**: Warm phosphor aesthetic, scan lines
- **Constraint-driven**: GameBoy palette influences, 4-color depth

## Phased Implementation (6 PRs, 60-80 hours)

### Phase 1: Foundation & Modularization (12-14 hours)
**Goal:** Extract monolith into manageable modules, prepare for 3D integration

**Tasks:**
- Break 4,303-line `index.html` into ES6 modules:
  - `state.js` - Global state management
  - `faces.js` - Face registry and selection logic
  - `build.js` - Build orchestration
  - `template.js` - Template management
  - `ui.js` - DOM manipulation utilities
- Add Three.js dependencies (CDN or npm)
- Create base scene manager (`scene-manager.js`)
- No UI changes yet (purely architectural)

**Deliverables:**
- Modular JavaScript structure
- Three.js integrated but not rendering
- All existing functionality preserved
- Comprehensive grep audit for broken references

**Risks:**
- Inline script references breaking
- Hash URL compatibility
- Build triggers not firing

**Mitigation:**
- Round-trip testing on every major extraction
- Keep hash format unchanged
- Maintain backward compatibility

---

### Phase 2: Split-Pane Layout System (10-12 hours)
**Goal:** Responsive desktop layout with HTML left, 3D canvas right

**Tasks:**
- CSS Grid split-pane layout (60/40 split)
- Responsive breakpoints:
  - `< 768px`: Single column (mobile), 3D optional/simplified
  - `>= 768px`: Split pane (desktop), full 3D
- Collapsible panels with smooth transitions
- Simple/Expert mode toggle (default: Simple)
- Canvas resize handling (Three.js viewport updates)

**Deliverables:**
- Desktop split-pane layout
- Mobile single-column fallback
- Resizable panels (optional)
- Mode toggle UI

**3D Integration:**
- Basic Three.js scene rendering in right panel
- Camera controls (OrbitControls)
- Ambient scene (placeholder grid, lighting)

**Risks:**
- CRT shader resize issues
- Mobile performance with 3D
- Canvas aspect ratio on window resize

**Mitigation:**
- Resize observer for canvas
- Progressive enhancement (3D disabled on low-end mobile)
- Fixed aspect ratio option

---

### Phase 3: 3D Face Gallery (14-16 hours)
**Goal:** Visualize watch faces as 3D floating modules in space

**Tasks:**
- **3D Face Cards:**
  - Low-poly rectangular cards (plane geometry)
  - Texture mapping (face metadata: name, category, flash size)
  - Ambient lighting with edge glow (amber CRT aesthetic)
  - Hover states (card lift, glow intensity)
- **Spatial Layout:**
  - Grid arrangement in 3D space (z-depth variation)
  - Category-based positioning (clusters)
  - Camera flythrough on selection
- **Interaction:**
  - Raycasting for click detection
  - Select face → camera zooms to card
  - Add to active list → card animates to "dock"
- **Asset Generation:**
  - Procedural face thumbnails (Canvas 2D → texture)
  - Category color coding (4-color GameBoy palette per category)
  - Flash size indicators (bar graph on card)

**Deliverables:**
- 3D face gallery rendering
- Click-to-select interaction
- Category filtering (hide/show clusters)
- Smooth camera transitions

**Performance:**
- Target: 60fps on desktop, 30fps on mobile
- LOD system (simplified geometry at distance)
- Texture atlasing (reduce draw calls)

**Risks:**
- Too many draw calls (100+ faces)
- Raycasting performance
- Mobile GPU limits

**Mitigation:**
- Instancing for repeated geometry
- Spatial culling (only render visible cards)
- Mobile: 2D grid fallback option

---

### Phase 4: Build Status Dashboard 3D (12-14 hours)
**Goal:** Real-time build visualization in 3D space

**Tasks:**
- **Progress Visualization:**
  - Spinning low-poly watch case (use RetroPass model)
  - Build stages as orbital rings filling (compilation → linking → upload)
  - Particle system for active builds (amber sparks)
- **Status Display:**
  - 3D text labels (build step names, timestamps)
  - Flash/RAM bars as 3D columns (height = usage %)
  - Error states (red glow, shake animation)
- **Build History:**
  - Previous builds as faded "ghosts" in background
  - Click history item → restore that config
- **Log Integration:**
  - Terminal-style log panel (HTML overlay on 3D canvas)
  - Syntax highlighting for errors/warnings
  - Auto-scroll during active build

**Deliverables:**
- 3D build progress animation
- Real-time status updates
- Build history visualization
- Log panel integration

**Performance:**
- Particle count limit (< 1000 particles)
- Text rendering optimization (bitmap fonts or CSS3D)
- Throttled updates (avoid 60fps log spam)

**Risks:**
- Particle system overhead
- Text rendering in 3D (performance)
- Build state sync (HTML ↔ 3D)

**Mitigation:**
- GPU particle system (vertex shader positioning)
- CSS3DRenderer for text (DOM-based, hardware accelerated)
- Debounced state updates

---

### Phase 5: Enhanced RetroPass Integration (8-10 hours)
**Goal:** Connect existing RetroPass 3D with new scenes

**Tasks:**
- **Scene Switching:**
  - Tab/button to switch between Face Gallery, Build Status, RetroPass
  - Smooth camera transitions between scenes
  - Shared lighting/post-processing
- **Active Faces Preview:**
  - RetroPass shows currently selected faces
  - Rotate through face list (auto or manual)
  - Real-time face configuration reflection (if possible)
- **Unified Aesthetic:**
  - Apply amber CRT glow to all scenes
  - Scan line shader (post-processing)
  - Vignette effect
  - Film grain (subtle)

**Deliverables:**
- Scene navigation system
- RetroPass ↔ Gallery/Status transitions
- Unified post-processing pipeline
- CRT shader controls (intensity, scan line density)

**Risks:**
- Post-processing performance hit
- Scene memory management (dispose old geometries)
- Shader complexity

**Mitigation:**
- Configurable post-processing (disable on low-end)
- Proper Three.js disposal on scene switch
- Pre-compiled shaders

---

### Phase 6: Keyboard Shortcuts & Polish (6-8 hours)
**Goal:** Power-user features and final refinements

**Tasks:**
- **Keyboard Shortcuts:**
  - `Ctrl+B`: Trigger build
  - `Ctrl+S`: Save template
  - `Ctrl+F`: Focus search (face filter)
  - `Ctrl+1/2/3`: Switch scenes (Gallery/Status/RetroPass)
  - `Space`: Pause/resume auto-rotation
  - `?`: Show help overlay
- **Accessibility:**
  - Keyboard navigation for 3D (arrow keys = camera movement)
  - Screen reader labels for all controls
  - Focus indicators on 3D objects (outline shader)
  - Alt text for generated textures
- **Polish:**
  - Loading screen (3D logo spin)
  - Smooth idle animations (camera drift, card float)
  - Sound effects (optional, muted by default)
  - Easter eggs (Konami code → special shader mode?)

**Deliverables:**
- Full keyboard shortcut system
- Accessibility pass (WCAG 2.1 AA)
- Help overlay
- Final visual polish

**Risks:**
- Keyboard conflicts with browser shortcuts
- Screen reader compatibility with WebGL
- Over-animated (distracting)

**Mitigation:**
- Configurable shortcuts (modal dialog to change)
- ARIA labels on all interactive elements
- Animation toggle (Settings panel)

---

## Asset Requirements

### 3D Models
- **Face cards**: Procedurally generated (Three.js BoxGeometry + textures)
- **Watch case**: Existing RetroPass model (already in builder)
- **Particles**: Point sprites (no models needed)

### Textures
- **Face thumbnails**: Generated from face metadata (Canvas 2D API)
- **Category badges**: SVG → rasterized to texture
- **CRT overlay**: Scan line pattern (1px × screen height, repeating)
- **Film grain**: Noise texture (512×512, tileable)

### Shaders
- **CRT glow**: Fragment shader (bloom + chromatic aberration)
- **Scan lines**: Overlay shader (UV-based)
- **Edge glow**: Fresnel-based rim lighting
- **Particle**: Vertex shader (CPU-free animation)

### Fonts
- **Monospace terminal**: For logs and build output
- **Display font**: For 3D labels (bitmap font atlas or CSS3D)

**Total asset size estimate:** < 5 MB (mostly procedural, minimal external assets)

---

## Performance Targets

### Desktop (>= 1080p, discrete GPU)
- **60 fps** sustained in all scenes
- **< 200 MB** memory usage
- **< 2s** initial load time

### Laptop (integrated GPU, 720p)
- **30-60 fps** (adaptive quality)
- **< 150 MB** memory
- **< 3s** load time

### Mobile (iOS/Android, modern devices)
- **30 fps** minimum (simplified 3D or 2D fallback)
- **< 100 MB** memory
- **< 5s** load time
- **Optional 3D** (can disable entirely)

### Optimization Strategies
- **LOD (Level of Detail)**: Reduce geometry complexity at distance
- **Frustum culling**: Don't render off-screen objects
- **Texture atlasing**: Combine textures to reduce draw calls
- **Instancing**: Reuse geometry for identical objects (face cards)
- **Web Workers**: Offload heavy computation (future)

---

## Technology Stack

### Core
- **Three.js** r170+ (latest stable)
- **ES6 modules** (native, no bundler initially)
- **Plain CSS** (no framework, full control)

### Three.js Extensions
- **OrbitControls**: Camera manipulation
- **CSS3DRenderer**: DOM-based 3D text (performance)
- **EffectComposer**: Post-processing pipeline
- **GLTFLoader**: Load RetroPass model (if not already embedded)

### Optional (Future)
- **Vite/Rollup**: Bundler for production builds
- **TypeScript**: Type safety (large refactor)
- **Tailwind CSS**: Utility classes (if CSS becomes unwieldy)

---

## Migration Strategy

### Backward Compatibility
- **Hash URLs unchanged**: `#faces=...&template=...` format preserved
- **Simple mode default**: Current behavior (HTML-only) is fallback
- **Progressive enhancement**: 3D only loads if WebGL supported
- **Mobile-first**: Single-column layout works without 3D

### Feature Flags
- `ENABLE_3D`: Toggle Three.js rendering (default: true on desktop)
- `ENABLE_POST_FX`: Toggle post-processing (default: false on mobile)
- `ENABLE_PARTICLES`: Toggle particle effects (default: false on low-end)

### Testing Strategy
- **Manual testing**: Each PR on desktop/mobile/tablet
- **Browser matrix**: Chrome, Firefox, Safari, Edge (latest)
- **Performance profiling**: Chrome DevTools, WebGL Inspector
- **Accessibility audit**: Lighthouse, aXe DevTools

---

## Risk Assessment

### High-Risk Areas
1. **Performance on low-end devices**: Mitigate with quality presets, fallbacks
2. **Three.js learning curve**: Mitigate with reference examples, documentation
3. **Scene memory leaks**: Mitigate with proper disposal, testing
4. **Mobile GPU limits**: Mitigate with simplified 3D or 2D fallback

### Medium-Risk Areas
1. **CRT shader complexity**: Mitigate with pre-tested shader code
2. **Raycasting performance**: Mitigate with spatial indexing, culling
3. **Browser compatibility**: Mitigate with WebGL fallback detection
4. **Asset loading time**: Mitigate with lazy loading, progress indicators

### Low-Risk Areas
1. **Keyboard shortcuts**: Standard browser APIs, well-documented
2. **Responsive layout**: CSS Grid is mature, widely supported
3. **Modularization**: Mechanical refactor, low logic risk

---

## Time Breakdown (Total: 62-74 hours)

| Phase | Hours | Complexity | Risk |
|-------|-------|------------|------|
| 1. Foundation & Modularization | 12-14 | Medium | Medium |
| 2. Split-Pane Layout | 10-12 | Low | Low |
| 3. 3D Face Gallery | 14-16 | High | Medium |
| 4. Build Status 3D | 12-14 | High | Medium |
| 5. RetroPass Integration | 8-10 | Medium | Low |
| 6. Shortcuts & Polish | 6-8 | Low | Low |
| **Total** | **62-74** | — | — |

**Recommended buffer:** +10-15% for debugging, testing, iteration = **70-85 hours realistic**

---

## Next Steps

1. **Decision check-in**:
   - Confirm Three.js approach (vs simpler 2D cards)
   - Confirm 70-85 hour budget acceptable
   - Confirm aesthetic direction (amber CRT glow, demoscene)

2. **Start Phase 1** (Foundation):
   - Create feature branch: `feature/hybrid-3d-builder`
   - Extract modular JS structure
   - Add Three.js dependency
   - Basic scene rendering test

3. **Iterate**:
   - PR per phase
   - Test on hardware after each merge
   - Adjust plan based on learnings

---

## Confirmed Decisions (2026-02-22)

1. **Asset creation**: ✅ No face thumbnails — text-based cards or simple geometric representation
2. **Sound effects**: ✅ Visual-only (no audio system)
3. **Mobile 3D**: ✅ Desktop-focused — no mobile 3D complexity
4. **Shader complexity**: ✅ Very subtle amber glow (adjustable intensity slider)
5. **Scene switching**: ✅ Manual only (tabs/buttons, no auto-rotate)

**Simplified scope benefits:**
- No audio API integration (saves ~4-6 hours)
- No mobile fallbacks (saves ~6-8 hours)
- No thumbnail generation system (saves ~4-5 hours)
- **Revised estimate: 55-65 hours** (down from 70-85)

---

**Phase 1 starting now!** 🚀
