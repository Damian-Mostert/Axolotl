# Wolf Demo

## Files

- `wolf_demo.axo` - Simple rotating wolf with color (WORKS)
- `wolf_simple.axo` - Wolf with texture attempt (textures not fully implemented)
- `wolf_animated.axo` - Wolf with bone animations
- `wolf_keyframe.axo` - Wolf with keyframe animations

## Current Limitation: Textures

**Textures are not fully implemented yet.** The `setTexture()` function exists but doesn't render properly because:

1. OBJ loader doesn't parse UV texture coordinates (`vt` lines)
2. Renderer doesn't enable GL texturing
3. No texture coordinate binding in render loop

## Working Demo

Use `wolf_demo.axo` which renders the wolf with a solid brown color:

```bash
cd env
axolotl wolf_demo.axo
```

This will:
- Load the Wolf OBJ model
- Apply a brown color (#8B7355)
- Rotate it slowly
- Render with proper lighting

## To Add Texture Support

The following changes are needed in `canvas_3d_functions.cpp`:

1. **Parse UV coordinates in LoadOBJBuiltin:**
   ```cpp
   std::vector<Vec2> uvs;  // Add to Mesh struct
   // Parse "vt u v" lines
   ```

2. **Enable texturing in RenderSceneBuiltin:**
   ```cpp
   if (mesh.textureId) {
       glEnable(GL_TEXTURE_2D);
       glBindTexture(GL_TEXTURE_2D, mesh.textureId);
   }
   ```

3. **Apply UV coordinates:**
   ```cpp
   glTexCoord2f(uv.x, uv.y);
   glVertex3f(v.x, v.y, v.z);
   ```

## Alternative: Use Colors

For now, use `setColor()` to give your models different appearances:

```axolotl
wolf.setColor("#8B7355");  // Brown
wolf.setColor("#CCCCCC");  // Gray
wolf.setColor("#FFFFFF");  // White
```

## Bone Animations

Bone animations work! See `wolf_animated.axo` for examples of:
- Idle animation
- Walk cycle
- Howl animation

Run with:
```bash
axolotl wolf_animated.axo
```
