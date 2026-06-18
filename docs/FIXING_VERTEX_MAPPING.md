# Fixing Vertex Mapping Issues - Diagnostic Guide

## What the Visualization Shows

When you enable `showBones(1)` and `showVertices(1)`, you see:

- **Red dots** = Bone joint positions
- **Green lines** = Parent-child bone connections  
- **Colored vertices** = Each bone gets a unique color
- **Gray vertices** = NOT assigned to any bone (problem!)

## Common Problems and Solutions

### Problem 1: Multiple Colors on One Body Part

**What you see:** Arms have 3-4 different colors mixed together

**Cause:** X or Y ranges are overlapping between bones

**Solution:** Make boundaries TIGHTER and NON-OVERLAPPING

```axolotl
// BAD - Too wide, overlaps with torso
player.assignVertexToBone(leftArm, 8.0, 13.0, -5.0, -1.0);

// GOOD - Tight boundaries
player.assignVertexToBone(leftUpperArm, 11.0, 13.0, -2.5, -1.5);
player.assignVertexToBone(leftForearm, 8.0, 11.0, -2.5, -1.5);
```

### Problem 2: Gray (Unassigned) Vertices

**What you see:** Gray dots scattered on the model

**Cause:** Gaps in your Y/X ranges - some vertices don't match any bone

**Solution:** Expand ranges slightly to cover all vertices

```axolotl
// BAD - Gap between 9.0 and 9.5
player.assignVertexToBone(leftThigh, 5.5, 9.0, -1.5, -0.2);
player.assignVertexToBone(pelvis, 9.5, 10.5, -1.0, 1.0);

// GOOD - No gap
player.assignVertexToBone(leftThigh, 5.5, 9.0, -1.5, -0.2);
player.assignVertexToBone(pelvis, 9.0, 10.5, -1.0, 1.0);  // Starts at 9.0
```

### Problem 3: Wrong Body Part Gets Wrong Color

**What you see:** Head is blue, but legs are also blue

**Cause:** Bone assignment order matters - first match wins!

**Solution:** Assign specific bones BEFORE general ones

```axolotl
// BAD - Head assigned first with wide range
player.assignVertexToBone(head, 14.0, 25.0, -3.0, 3.0);  // Too wide!
player.assignVertexToBone(neck, 14.0, 16.0, -0.7, 0.7);  // Never matches!

// GOOD - Neck first, then head
player.assignVertexToBone(neck, 14.5, 16.0, -0.7, 0.7);
player.assignVertexToBone(head, 16.0, 25.0, -2.0, 2.0);
```

### Problem 4: Arms Affect Legs (Cross-Contamination)

**What you see:** When arms move, leg vertices also move

**Cause:** X ranges are too wide (e.g., `-5.0 to 5.0` covers entire body)

**Solution:** Use TIGHT X boundaries

```axolotl
// BAD - Arms cover entire width
player.assignVertexToBone(leftArm, 8.0, 13.0, -5.0, -0.5);   // Too wide!
player.assignVertexToBone(rightArm, 8.0, 13.0, 0.5, 5.0);    // Too wide!

// GOOD - Arms stay away from center
player.assignVertexToBone(leftArm, 8.0, 13.0, -2.5, -1.5);   // Tight!
player.assignVertexToBone(rightArm, 8.0, 13.0, 1.5, 2.5);    // Tight!
```

## Step-by-Step Fixing Process

### Step 1: Enable Visualization

```axolotl
enableDevMode(camera);
showBones(1);
showVertices(1);
```

### Step 2: Check Each Body Part

Rotate the camera and look at each part:

1. **Head** - Should be ONE color (no gray)
2. **Neck** - Should be ONE color, different from head
3. **Chest** - Should be ONE color
4. **Arms** - Each arm should be 3 colors (shoulder, upper, forearm)
5. **Pelvis** - Should be ONE color
6. **Legs** - Each leg should be 3 colors (thigh, shin, foot)

### Step 3: Fix From Bottom Up

Start with feet and work upward:

```axolotl
// 1. FEET (easiest - bottom of model)
player.assignVertexToBone(leftFoot, 0.0, 2.0, -1.2, -0.1);
player.assignVertexToBone(rightFoot, 0.0, 2.0, 0.1, 1.2);

// 2. SHINS
player.assignVertexToBone(leftShin, 2.0, 5.5, -1.2, -0.1);
player.assignVertexToBone(rightShin, 2.0, 5.5, 0.1, 1.2);

// 3. THIGHS
player.assignVertexToBone(leftThigh, 5.5, 9.0, -1.5, -0.2);
player.assignVertexToBone(rightThigh, 5.5, 9.0, 0.2, 1.5);

// 4. PELVIS
player.assignVertexToBone(pelvis, 9.0, 10.5, -1.0, 1.0);

// 5. SPINE
player.assignVertexToBone(spine, 10.5, 11.5, -0.9, 0.9);

// 6. CHEST (exclude arms!)
player.assignVertexToBone(chest, 11.5, 13.5, -0.8, 0.8);

// 7. SHOULDERS
player.assignVertexToBone(leftShoulder, 13.0, 14.5, -2.0, -0.8);
player.assignVertexToBone(rightShoulder, 13.0, 14.5, 0.8, 2.0);

// 8. UPPER ARMS
player.assignVertexToBone(leftUpperArm, 11.0, 13.0, -2.5, -1.5);
player.assignVertexToBone(rightUpperArm, 11.0, 13.0, 1.5, 2.5);

// 9. FOREARMS
player.assignVertexToBone(leftForearm, 8.0, 11.0, -2.5, -1.5);
player.assignVertexToBone(rightForearm, 8.0, 11.0, 1.5, 2.5);

// 10. NECK
player.assignVertexToBone(neck, 14.5, 16.0, -0.7, 0.7);

// 11. HEAD (last - catches everything above)
player.assignVertexToBone(head, 16.0, 25.0, -2.0, 2.0);
```

### Step 4: Test Each Fix

After each assignment, check the visualization:

1. Does that body part have ONE solid color now?
2. Are there any gray vertices left?
3. Did fixing this part break another part?

### Step 5: Fine-Tune Boundaries

If you see issues, adjust by small amounts:

```axolotl
// If left arm has some gray vertices
player.assignVertexToBone(leftUpperArm, 11.0, 13.0, -2.6, -1.4);  // Wider by 0.1

// If left arm bleeds into torso
player.assignVertexToBone(leftUpperArm, 11.0, 13.0, -2.4, -1.6);  // Narrower by 0.1
```

## Quick Reference: Typical Human Model Ranges

For a standing human model (Y: 0-20):

| Body Part | Y Range | X Range (Left) | X Range (Right) |
|-----------|---------|----------------|-----------------|
| Feet | 0.0 - 2.0 | -1.2 to -0.1 | 0.1 to 1.2 |
| Shins | 2.0 - 5.5 | -1.2 to -0.1 | 0.1 to 1.2 |
| Thighs | 5.5 - 9.0 | -1.5 to -0.2 | 0.2 to 1.5 |
| Pelvis | 9.0 - 10.5 | -1.0 to 1.0 | (center) |
| Spine | 10.5 - 11.5 | -0.9 to 0.9 | (center) |
| Chest | 11.5 - 13.5 | -0.8 to 0.8 | (center) |
| Shoulders | 13.0 - 14.5 | -2.0 to -0.8 | 0.8 to 2.0 |
| Upper Arms | 11.0 - 13.0 | -2.5 to -1.5 | 1.5 to 2.5 |
| Forearms | 8.0 - 11.0 | -2.5 to -1.5 | 1.5 to 2.5 |
| Neck | 14.5 - 16.0 | -0.7 to 0.7 | (center) |
| Head | 16.0 - 25.0 | -2.0 to 2.0 | (center) |

## Key Principles

1. **No Overlaps** - Each vertex should belong to exactly ONE bone
2. **No Gaps** - Every vertex should belong to SOME bone
3. **Tight Boundaries** - Use the smallest ranges that cover the body part
4. **Order Matters** - Assign specific parts before general parts
5. **Test Incrementally** - Fix one part at a time

## Debugging Commands

```axolotl
// Show only bones (hide vertices)
showBones(1);
showVertices(0);

// Show only vertices (hide bones)
showBones(0);
showVertices(1);

// Show both
showBones(1);
showVertices(1);

// Hide all (normal rendering)
showBones(0);
showVertices(0);
```

## Example: Fixing the Soccer Player

### Original Problem
- Arms had 4 different colors (should be 3)
- Head was cyan/blue (wrong color)
- Some gray vertices on torso

### Root Causes
1. Arm X ranges too wide: `-5.0 to -1.8` (should be `-2.5 to -1.5`)
2. Head Y range started too low: `14.0` (should be `16.0`)
3. Gaps between pelvis and thighs

### Solution
See `soccer_player_demo_fixed.axo` for corrected version with:
- Tight X boundaries for arms
- Proper Y ranges with no gaps
- Head starting at 16.0 instead of 14.0

## When to Use Visualization

- **During initial setup** - Get the mapping right from the start
- **When adding new bones** - Make sure they don't conflict
- **When animation looks wrong** - Check if vertices are assigned correctly
- **When optimizing** - Find and fix unassigned vertices

## Performance Note

Visualization adds ~2-5ms per frame. Disable it for final builds:

```axolotl
// Development
showBones(1);
showVertices(1);

// Production
showBones(0);
showVertices(0);
```
