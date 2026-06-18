# Bone Visualization Quick Reference

## Setup (3 lines)

```axolotl
enableDevMode(camera);    // Enable dev camera controls
showBones(1);             // Show bone joints (red) and connections (green)
showVertices(1);          // Show vertices colored by bone assignment
```

## Visual Legend

| Color/Shape | Meaning |
|-------------|---------|
| 🔴 Red dots | Bone joint positions |
| 🟢 Green lines | Parent-child bone connections |
| 🌈 Colored dots | Vertices (color = which bone they're assigned to) |
| ⚪ Gray dots | Unassigned vertices |
| 🟡 Yellow dots | Vertices on meshes without bones |

## Dev Camera Controls

| Input | Action |
|-------|--------|
| Mouse drag | Rotate camera |
| Mouse wheel | Zoom in/out |
| W/A/S/D | Pan camera |
| Q/E | Move up/down |
| Shift | Fast movement (5x) |
| Ctrl | Very fast movement (25x) |

## Render Loop Template

```axolotl
var running: int = 1;
while (running == 1) {
    player.updateAnimCallback(0.016);  // Update animation
    updateDevCamera();                  // Update camera from input
    running = handleDevInput();         // Process events
    canvas.render(scene, camera);       // Render with visualization
}
```

## Common Patterns

### Toggle visualization on/off
```axolotl
showBones(0);      // Hide
showBones(1);      // Show
```

### Debug bone placement
```axolotl
showBones(1);      // Show bones only
showVertices(0);   // Hide vertices
```

### Debug vertex assignments
```axolotl
showBones(1);      // Show bones for reference
showVertices(1);   // Show which vertices belong to which bone
```

### Performance mode (disable during final render)
```axolotl
showBones(0);
showVertices(0);
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Nothing visible | Call `enableDevMode(camera)` first |
| All vertices gray | Need to call `assignVertexToBone()` |
| Wrong vertex colors | Adjust Y/X ranges in `assignVertexToBone()` |
| Can't see model | Use mouse wheel to zoom, WASD to pan |

## Full Example

```axolotl
// Setup
var canvas: object = createCanvas(1280, 720);
var scene: object = createScene();
var camera: object = PerspectiveCamera(60.0);
camera.setPosition(0.0, 15.0, 30.0);
camera.lookAt(0.0, 10.0, 0.0);

// Enable visualization
enableDevMode(camera);
showBones(1);
showVertices(1);

// Load and setup model
var player: object = loadOBJ("assets/player.obj");
scene.add(player);

// Create bones
var pelvis: int = player.createBone("pelvis", -1);
var spine: int = player.createBone("spine", pelvis);

// Assign vertices
player.assignVertexToBone(pelvis, 9.0, 10.5, -1000.0, 1000.0);
player.assignVertexToBone(spine, 10.5, 12.0, -1000.0, 1000.0);

// Create animation
player.addAnimation("idle", func(state: object) -> void {
    player.setBonePose(pelvis, 0.0, 10.0, 0.0, 0.0, 0.0, 0.0);
    player.setBonePose(spine, 0.0, 11.0, 0.0, 0.0, 0.0, 0.0);
});
player.playAnimCallback("idle");

// Lighting
var light: object = PointLight("#FFFFFF", 2.0);
light.setPosition(10.0, 20.0, 10.0);
scene.add(light);

// Render loop
var running: int = 1;
while (running == 1) {
    player.updateAnimCallback(0.016);
    updateDevCamera();
    running = handleDevInput();
    canvas.render(scene, camera);
}
```

## See Also

- [BONE_VISUALIZATION.md](BONE_VISUALIZATION.md) - Complete documentation
- [BONE_ANIMATION.md](BONE_ANIMATION.md) - Bone animation API
- [bone_visualization_demo.axo](../tests/bone_visualization_demo.axo) - Working example
