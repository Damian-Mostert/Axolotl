# Bone Animation System for OBJ Files

## Overview

Axolotl now supports skeletal bone animation for OBJ files using a callback-based system. This allows you to create predetermined bone actions and animations that can be played back dynamically.

## API Reference

### Creating Bones

```axolotl
var boneIndex: int = mesh.createBone(name, parentBoneIndex)
```

- **name**: String identifier for the bone
- **parentBoneIndex**: Index of parent bone, or -1 for root bones
- **Returns**: Integer index of the created bone

**Example:**
```axolotl
var model: object = loadOBJ("character.obj");
var spine: int = model.createBone("spine", -1);
var head: int = model.createBone("head", spine);
var leftArm: int = model.createBone("leftArm", spine);
```

### Setting Bone Pose

```axolotl
mesh.setBonePose(boneIndex, px, py, pz, rx, ry, rz)
```

- **boneIndex**: Index of the bone to modify
- **px, py, pz**: Position offset (x, y, z)
- **rx, ry, rz**: Rotation in radians (x, y, z)

**Example:**
```axolotl
// Rotate arm 45 degrees on X axis
model.setBonePose(leftArm, 0.0, 0.5, 0.0, 0.785, 0.0, 0.0);
```

### Adding Animations with Callbacks

```axolotl
mesh.addAnimation(name, callback)
```

- **name**: String identifier for the animation
- **callback**: Function that receives a state object with `time` field

**Example:**
```axolotl
model.addAnimation("wave", func(state: object) -> void {
    var t: float = state.time;
    
    // Animate arm waving
    model.setBonePose(rightArm, 0.3, 0.5, 0.0, 
                      -1.57 + sin(t * 5.0) * 0.3, 
                      0.0, 
                      sin(t * 5.0) * 0.5);
});
```

### Playing Animations

```axolotl
mesh.playAnimCallback(name)
```

- **name**: Name of the animation to play

**Example:**
```axolotl
model.playAnimCallback("wave");
```

### Updating Animations

```axolotl
mesh.updateAnimCallback(deltaTime)
```

- **deltaTime**: Time elapsed since last update (in seconds)

**Example:**
```axolotl
// Update at 60fps
while (running == 1) {
    model.updateAnimCallback(0.016);
    canvas.render(scene, camera);
}
```

## Complete Example

```axolotl
// Load model
var character: object = loadOBJ("character.obj");
character.setPosition(0.0, 0.0, 0.0);
scene.add(character);

// Create skeleton
var spine: int = character.createBone("spine", -1);
var head: int = character.createBone("head", spine);
var leftArm: int = character.createBone("leftArm", spine);
var rightArm: int = character.createBone("rightArm", spine);

// Define walk animation
character.addAnimation("walk", func(state: object) -> void {
    var t: float = state.time;
    
    // Spine bob
    character.setBonePose(spine, 0.0, sin(t * 4.0) * 0.05, 0.0, 0.0, 0.0, 0.0);
    
    // Arms swing
    character.setBonePose(leftArm, -0.3, 0.3, 0.0, sin(t * 3.0) * 0.5, 0.0, 0.0);
    character.setBonePose(rightArm, 0.3, 0.3, 0.0, sin(t * 3.0 + 3.14) * 0.5, 0.0, 0.0);
});

// Define jump animation
character.addAnimation("jump", func(state: object) -> void {
    var t: float = state.time;
    var jumpCycle: float = t % 2.0;
    var height: float = 0.0;
    
    if (jumpCycle < 1.0) {
        height = sin(jumpCycle * 3.14) * 0.8;
    }
    
    character.setBonePose(spine, 0.0, height, 0.0, 0.0, 0.0, 0.0);
    character.setBonePose(leftArm, -0.3, 0.3 + height, 0.0, -height * 1.5, 0.0, 0.0);
    character.setBonePose(rightArm, 0.3, 0.3 + height, 0.0, -height * 1.5, 0.0, 0.0);
});

// Play animation
character.playAnimCallback("walk");

// Animation loop
while (running == 1) {
    character.updateAnimCallback(0.016);
    canvas.render(scene, camera);
}
```

## Animation Patterns

### Looping Animations

Use `sin()` and `cos()` for smooth loops:

```axolotl
model.addAnimation("idle", func(state: object) -> void {
    var t: float = state.time;
    // Gentle breathing motion
    model.setBonePose(spine, 0.0, sin(t * 2.0) * 0.02, 0.0, 0.0, 0.0, 0.0);
});
```

### Timed Sequences

Use modulo for repeating sequences:

```axolotl
model.addAnimation("patrol", func(state: object) -> void {
    var t: float = state.time;
    var cycle: float = t % 4.0;
    
    if (cycle < 2.0) {
        // Walk forward
        model.setBonePose(leftLeg, -0.2, -0.5, 0.0, sin(t * 3.0) * 0.8, 0.0, 0.0);
    } else {
        // Stand still
        model.setBonePose(leftLeg, -0.2, -0.5, 0.0, 0.0, 0.0, 0.0);
    }
});
```

### Switching Animations

```axolotl
var currentAnim: string = "walk";

// Switch animation
if (playerJumped == 1) {
    currentAnim = "jump";
    model.playAnimCallback(currentAnim);
}
```

## Tips

1. **Delta Time**: Always use consistent deltaTime (e.g., 0.016 for 60fps)
2. **Bone Hierarchy**: Child bones inherit parent transformations
3. **Radians**: Rotation uses radians (π ≈ 3.14159)
4. **Performance**: Keep bone count reasonable (<50 bones per mesh)
5. **Smooth Transitions**: Use interpolation between animation states

## Legacy API (Keyframe-based)

The system also supports traditional keyframe animations:

```axolotl
// Create animation
var animIdx: int = model.createAnimation("walk", 2.0);

// Add keyframes
model.addAnimKey(animIdx, "leftArm", 0.0, 0.0, 0.0, 0.0, 0);
model.addAnimKey(animIdx, "leftArm", 1.0, 0.5, 0.0, 0.0, 0);

// Play
model.playAnimation(animIdx);
model.updateAnimation(0.016);
```

## See Also

- `tests/bone_animation_demo.axo` - Full featured demo
- `tests/simple_bone_anim.axo` - Minimal example
- `docs/GAME_FEATURES.md` - 3D graphics features
