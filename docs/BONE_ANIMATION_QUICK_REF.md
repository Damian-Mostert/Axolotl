# Bone Animation Quick Reference

## Basic Setup

```axolotl
// Load OBJ model
var model: object = loadOBJ("character.obj");
scene.add(model);

// Create bones (returns bone index)
var spine: int = model.createBone("spine", -1);        // Root bone
var head: int = model.createBone("head", spine);       // Child of spine
var arm: int = model.createBone("arm", spine);         // Child of spine
```

## Define Animation

```axolotl
model.addAnimation("animName", func(state: object) -> void {
    var t: float = state.time;  // Current animation time
    
    // Set bone poses: setBonePose(boneIndex, px, py, pz, rx, ry, rz)
    model.setBonePose(spine, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    model.setBonePose(arm, 0.5, 0.5, 0.0, sin(t * 2.0), 0.0, 0.0);
});
```

## Play & Update

```axolotl
// Start animation
model.playAnimCallback("animName");

// Update in loop (deltaTime in seconds)
while (running == 1) {
    model.updateAnimCallback(0.016);  // ~60fps
    canvas.render(scene, camera);
}
```

## Common Patterns

### Looping Motion
```axolotl
var angle: float = sin(t * speed) * amplitude;
model.setBonePose(bone, x, y, z, angle, 0.0, 0.0);
```

### Alternating (Walk Cycle)
```axolotl
model.setBonePose(leftLeg, -0.2, -0.5, 0.0, sin(t * 3.0) * 0.8, 0.0, 0.0);
model.setBonePose(rightLeg, 0.2, -0.5, 0.0, sin(t * 3.0 + 3.14) * 0.8, 0.0, 0.0);
```

### Timed Sequence
```axolotl
var cycle: float = t % duration;
if (cycle < 1.0) {
    // First action
} else {
    // Second action
}
```

### Jump Arc
```axolotl
var jumpCycle: float = t % 2.0;
var height: float = 0.0;
if (jumpCycle < 1.0) {
    height = sin(jumpCycle * 3.14) * maxHeight;
}
model.setBonePose(spine, 0.0, height, 0.0, 0.0, 0.0, 0.0);
```

## Multiple Animations

```axolotl
// Define multiple
model.addAnimation("idle", func(state: object) -> void { ... });
model.addAnimation("walk", func(state: object) -> void { ... });
model.addAnimation("run", func(state: object) -> void { ... });

// Switch between them
var current: string = "idle";
model.playAnimCallback(current);

// Later...
current = "walk";
model.playAnimCallback(current);
```

## Rotation Reference

- **0.0** = 0°
- **1.57** = 90° (π/2)
- **3.14** = 180° (π)
- **4.71** = 270° (3π/2)
- **6.28** = 360° (2π)

## Tips

✓ Use `sin()` and `cos()` for smooth loops  
✓ Add π (3.14) to offset by 180°  
✓ Multiply time by speed: `t * 2.0` = 2x faster  
✓ Multiply result by amplitude: `sin(t) * 0.5` = half range  
✓ Keep deltaTime consistent (0.016 for 60fps)  
✓ Child bones inherit parent transforms  

## Full Example

```axolotl
var model: object = loadOBJ("char.obj");
var spine: int = model.createBone("spine", -1);
var arm: int = model.createBone("arm", spine);

model.addAnimation("wave", func(state: object) -> void {
    var t: float = state.time;
    model.setBonePose(spine, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    model.setBonePose(arm, 0.5, 0.5, 0.0, -1.57 + sin(t * 5.0) * 0.5, 0.0, 0.0);
});

model.playAnimCallback("wave");

while (running == 1) {
    model.updateAnimCallback(0.016);
    canvas.render(scene, camera);
}
```

See `docs/BONE_ANIMATION.md` for complete documentation.
