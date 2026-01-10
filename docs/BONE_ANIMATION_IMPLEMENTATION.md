# Bone Animation System Implementation Summary

## Overview

Implemented a callback-based bone animation system for OBJ files in Axolotl, allowing developers to create predetermined bone actions and animations that can be played back dynamically.

## What Was Added

### C++ Backend (canvas_3d_functions.cpp)

1. **AddAnimationBuiltin** - Stores animation callbacks by name
   - Syntax: `mesh.addAnimation(name, callback)`
   - Stores function callbacks in mesh object fields
   - Allows multiple named animations per mesh

2. **PlayAnimCallbackBuiltin** - Starts playing a named animation
   - Syntax: `mesh.playAnimCallback(name)`
   - Sets current animation and resets time counter
   - Validates animation exists before playing

3. **UpdateAnimCallbackBuiltin** - Updates animation state
   - Syntax: `mesh.updateAnimCallback(deltaTime)`
   - Increments animation time
   - Calls animation callback with time state
   - Runs every frame in game loop

### Existing Bone System (Already Present)

The following were already implemented:
- `createBone(name, parentIndex)` - Create skeletal bones
- `setBonePose(index, px, py, pz, rx, ry, rz)` - Set bone transforms
- `createAnimation(name, duration)` - Keyframe-based animations
- `addAnimKey(...)` - Add keyframes to animations
- `playAnimation(index)` - Play keyframe animation
- `updateAnimation(deltaTime)` - Update keyframe animation

## API Design

### Simple Callback Pattern

```axolotl
// Define animation with callback
mesh.addAnimation("walk", func(state: object) -> void {
    var t: float = state.time;
    mesh.setBonePose(bone, x, y, z, sin(t) * angle, 0.0, 0.0);
});

// Play animation
mesh.playAnimCallback("walk");

// Update in loop
while (running == 1) {
    mesh.updateAnimCallback(0.016);  // 60fps
    canvas.render(scene, camera);
}
```

### Benefits

1. **Procedural** - Animations defined as code, not data
2. **Dynamic** - Can use math functions (sin, cos) for smooth motion
3. **Flexible** - Easy to modify and extend
4. **Named** - Multiple animations per mesh with string identifiers
5. **Callback-based** - Clean separation of animation logic

## Examples Created

### 1. bone_animation_demo.axo
Full-featured demo with 4 animations:
- Walk cycle with arm/leg swing
- Wave animation with arm motion
- Jump with height arc
- Dance with complex movements

### 2. simple_bone_anim.axo
Minimal example showing basic API usage:
- Single rotating arm animation
- ~50 lines of code

### 3. game_character_anim.axo
Game-ready character controller with 5 animations:
- Idle (breathing, subtle head movement)
- Walk (coordinated limb motion)
- Run (faster, more exaggerated)
- Attack (swing animation with wind-up)
- Jump (height arc with leg tuck)

## Documentation

### 1. BONE_ANIMATION.md
Complete API reference with:
- Function signatures
- Parameter descriptions
- Usage examples
- Animation patterns
- Tips and best practices

### 2. BONE_ANIMATION_QUICK_REF.md
Quick reference cheat sheet with:
- Basic setup code
- Common patterns
- Rotation reference
- Full working example

### 3. README.md Updates
- Added bone animation to features list
- Added demo reference to examples section

## Technical Details

### Data Storage

Animations are stored in mesh object fields:
- `_animations` - Map of animation name → callback function
- `_currentAnim` - Currently playing animation name
- `_animTime` - Current animation time counter

### Callback Signature

```axolotl
func(state: object) -> void
```

Where `state` contains:
- `time: float` - Current animation time in seconds

### Integration with Existing System

The callback system works alongside the existing keyframe system:
- **Callback-based**: `addAnimation()`, `playAnimCallback()`, `updateAnimCallback()`
- **Keyframe-based**: `createAnimation()`, `addAnimKey()`, `playAnimation()`, `updateAnimation()`

Both systems use the same bone infrastructure (`createBone()`, `setBonePose()`).

## Usage Patterns

### Looping Animations
```axolotl
var angle: float = sin(t * speed) * amplitude;
```

### Alternating Motion (Walk)
```axolotl
leftLeg: sin(t * 3.0) * 0.8
rightLeg: sin(t * 3.0 + 3.14) * 0.8  // 180° offset
```

### Timed Sequences
```axolotl
var cycle: float = t % duration;
if (cycle < 1.0) { /* action 1 */ }
else { /* action 2 */ }
```

### Jump Arcs
```axolotl
var height: float = sin(cycle * 3.14) * maxHeight;
```

## Performance Considerations

- Callback functions are called every frame
- Keep bone count reasonable (<50 per mesh)
- Use consistent deltaTime for smooth animation
- Bone transforms are hierarchical (children inherit parent transforms)

## Future Enhancements

Possible additions:
- Animation blending/transitions
- Animation events/triggers
- IK (Inverse Kinematics) support
- Animation state machines
- Export/import animation data
- Animation editor tool

## Files Modified/Created

### Modified
- `src/builtins/canvas_3d_functions.cpp` - Added 3 new builtin functions
- `README.md` - Updated features and examples

### Created
- `tests/bone_animation_demo.axo` - Full demo
- `tests/simple_bone_anim.axo` - Minimal example
- `tests/game_character_anim.axo` - Game character
- `docs/BONE_ANIMATION.md` - Complete documentation
- `docs/BONE_ANIMATION_QUICK_REF.md` - Quick reference
- `docs/BONE_ANIMATION_IMPLEMENTATION.md` - This file

## Conclusion

The bone animation system provides a powerful, flexible way to animate OBJ models in Axolotl. The callback-based approach allows for procedural animations using mathematical functions, making it easy to create smooth, dynamic character movements without requiring external animation tools or data files.
