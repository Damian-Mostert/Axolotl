# Bone and Vertex Visualization in Dev Mode

When working with skeletal animations in Axolotl, it's crucial to visualize the bone structure and vertex assignments to debug and fine-tune your animations. The dev mode visualization features help you see exactly what's happening with your skeletal rig.

## Enabling Visualization

### Basic Setup

```axolotl
// 1. Create your scene and camera
var canvas: object = createCanvas(1280, 720);
var scene: object = createScene();
var camera: object = PerspectiveCamera(60.0);

// 2. Enable dev mode with the camera
enableDevMode(camera);

// 3. Enable bone visualization (red joints + green connections)
showBones(1);

// 4. Enable vertex visualization (colored by bone assignment)
showVertices(1);
```

### Toggling Visualization

You can toggle visualization on/off at any time:

```axolotl
showBones(0);      // Hide bones
showBones(1);      // Show bones

showVertices(0);   // Hide vertices
showVertices(1);   // Show vertices
```

## What You'll See

### Bone Visualization (`showBones(1)`)

- **Red dots**: Bone joint positions
- **Green lines**: Connections between parent and child bones
- Bones are rendered in world space, accounting for mesh position, rotation, and scale
- The skeleton hierarchy is clearly visible

### Vertex Visualization (`showVertices(1)`)

- **Colored dots**: Each vertex is colored based on which bone it's assigned to
  - Different bones get distinct colors using a golden angle hue distribution
  - This makes it easy to see which vertices belong to which bone
- **Gray dots**: Vertices that aren't assigned to any bone
- **Yellow dots**: Vertices on meshes without skeletal rigs

## Dev Mode Camera Controls

When dev mode is enabled, you get full 3D camera control:

- **Mouse drag**: Rotate camera around target
- **Mouse wheel**: Zoom in/out
- **W/A/S/D**: Pan camera (forward/left/back/right)
- **Q/E**: Move camera up/down
- **Shift**: Fast movement (5x speed)
- **Ctrl**: Very fast movement (25x speed)

## Use Cases

### 1. Debugging Bone Placement

```axolotl
// Create bones
var pelvis: int = player.createBone("pelvis", -1);
var spine: int = player.createBone("spine", pelvis);

// Set bone poses
player.setBonePose(pelvis, 0.0, 10.0, 0.0, 0.0, 0.0, 0.0);
player.setBonePose(spine, 0.0, 11.0, 0.0, 0.0, 0.0, 0.0);

// Enable visualization to see if bones are positioned correctly
showBones(1);
```

### 2. Verifying Vertex Assignments

```axolotl
// Assign vertices to bones
player.assignVertexToBone(leftArm, 6.5, 9.5, -2.0, -1.0);
player.assignVertexToBone(rightArm, 6.5, 9.5, 1.0, 2.0);

// Enable vertex visualization to see which vertices are assigned
showVertices(1);

// Look for:
// - Gray vertices (unassigned - might need adjustment)
// - Vertices with wrong colors (assigned to wrong bone)
// - Gaps in coverage (missing bone assignments)
```

### 3. Testing Animation Deformation

```axolotl
// Create animation
player.addAnimation("wave", func(state: object) -> void {
    var angle = sin(state.time * 3.0) * 0.5;
    player.setBonePose(rightUpperArm, 2.5, 12.25, 0.0, angle, 0.0, 0.0);
});

player.playAnimCallback("wave");

// Enable both visualizations to see how bones move and vertices follow
showBones(1);
showVertices(1);

// In the render loop
while (running == 1) {
    player.updateAnimCallback(0.016);
    updateDevCamera();
    running = handleDevInput();
    canvas.render(scene, camera);
}
```

## Tips for Effective Visualization

### 1. Start with Bones Only

First, verify your bone hierarchy and positions:

```axolotl
showBones(1);
showVertices(0);
```

Check that:
- Bone joints (red dots) are at the correct positions
- Parent-child connections (green lines) form the expected hierarchy
- Bone positions match your model's anatomy

### 2. Then Add Vertices

Once bones look correct, enable vertex visualization:

```axolotl
showBones(1);
showVertices(1);
```

Look for:
- Each bone should have a distinct color
- Vertices should form clear regions matching bone boundaries
- No unexpected gray (unassigned) vertices in important areas

### 3. Adjust Vertex Assignments

If you see issues, adjust the bone ranges:

```axolotl
// Expand bone range if too few vertices are assigned
player.assignVertexToBone(leftArm, 6.5, 9.5, -2.5, -0.5);  // Wider X range

// Narrow range if too many vertices are assigned
player.assignVertexToBone(leftArm, 7.0, 9.0, -2.0, -1.0);  // Tighter Y range
```

### 4. Test with Animation

Always test with animation running to see deformation:

```axolotl
player.playAnimCallback("walk");

while (running == 1) {
    player.updateAnimCallback(0.016);
    // Watch how vertices move with bones
    canvas.render(scene, camera);
}
```

## Common Issues and Solutions

### Issue: Gray vertices everywhere
**Solution**: You haven't assigned vertices to bones yet. Use `assignVertexToBone()` to assign them.

### Issue: Vertices assigned to wrong bone
**Solution**: Adjust the Y and X ranges in `assignVertexToBone()` to be more precise.

### Issue: Bones not visible
**Solution**: 
- Make sure `enableDevMode(camera)` was called
- Verify `showBones(1)` was called
- Check that your mesh has bones created with `createBone()`

### Issue: Vertices all one color
**Solution**: This means they're all assigned to the same bone. Check your `assignVertexToBone()` ranges.

### Issue: Can't see the model clearly
**Solution**: 
- Use mouse wheel to zoom
- Use W/A/S/D to pan around
- Hold Shift or Ctrl for faster movement
- Temporarily disable vertex visualization: `showVertices(0)`

## Example: Complete Workflow

```axolotl
var canvas: object = createCanvas(1280, 720);
var scene: object = createScene();
var camera: object = PerspectiveCamera(60.0);

camera.setPosition(0.0, 15.0, 30.0);
camera.lookAt(0.0, 10.0, 0.0);

// Enable dev mode
enableDevMode(camera);
showBones(1);
showVertices(1);

// Load model
var player: object = loadOBJ("assets/player.obj");
scene.add(player);

// Create skeleton
var pelvis: int = player.createBone("pelvis", -1);
var spine: int = player.createBone("spine", pelvis);
var leftArm: int = player.createBone("leftArm", spine);
var rightArm: int = player.createBone("rightArm", spine);

// Assign vertices
player.assignVertexToBone(pelvis, 9.0, 10.5, -1000.0, 1000.0);
player.assignVertexToBone(spine, 10.5, 12.0, -1000.0, 1000.0);
player.assignVertexToBone(leftArm, 10.0, 14.0, -2.0, -0.5);
player.assignVertexToBone(rightArm, 10.0, 14.0, 0.5, 2.0);

// Create animation
player.addAnimation("idle", func(state: object) -> void {
    player.setBonePose(pelvis, 0.0, 10.0, 0.0, 0.0, 0.0, 0.0);
    player.setBonePose(spine, 0.0, 11.0, 0.0, 0.0, 0.0, 0.0);
    player.setBonePose(leftArm, -1.5, 12.0, 0.0, 0.0, 0.0, 0.0);
    player.setBonePose(rightArm, 1.5, 12.0, 0.0, 0.0, 0.0, 0.0);
});

player.playAnimCallback("idle");

// Add lighting
var light: object = PointLight("#FFFFFF", 2.0);
light.setPosition(10.0, 20.0, 10.0);
scene.add(light);

var ambient: object = AmbientLight("#FFFFFF", 0.6);
scene.add(ambient);

// Render loop
var running: int = 1;
while (running == 1) {
    player.updateAnimCallback(0.016);
    updateDevCamera();
    running = handleDevInput();
    canvas.render(scene, camera);
}
```

## API Reference

### `enableDevMode(camera: object) -> void`
Enables development mode with free camera controls.

### `showBones(enabled: int) -> void`
Toggle bone visualization (joints and connections).
- `1` = show bones
- `0` = hide bones

### `showVertices(enabled: int) -> void`
Toggle vertex visualization (colored by bone assignment).
- `1` = show vertices
- `0` = hide vertices

### `updateDevCamera() -> void`
Updates the dev camera position based on input. Call this in your render loop.

### `handleDevInput() -> int`
Processes input events for dev mode. Returns `0` when window is closed, `1` otherwise.

## Performance Notes

- Visualization adds minimal overhead (a few milliseconds per frame)
- Vertex visualization is more expensive than bone visualization
- For very high-poly models (>100k vertices), consider toggling visualization off when not needed
- Visualization is only rendered when dev mode is enabled

## See Also

- [BONE_ANIMATION.md](BONE_ANIMATION.md) - Complete bone animation API
- [examples/bone_visualization_demo.axo](../tests/bone_visualization_demo.axo) - Working example
- [examples/physics_builtin_test.axo](../tests/physics_builtin_test.axo) - Full character controller with bones
