# Moho workflow, and how our plan compares

## Moho's workflow, step by step

1. **Draw.** You draw vector shapes in vector layers, either straight on the
   canvas or traced over an imported picture. Each body part usually goes in
   its own layer, or several parts share one layer.
2. **Style.** Each shape has a fill and a line. You can save these as a
   *style*: fill color, line color, line width, line brush and effects. A
   shape can use up to two styles. When you edit a style, every shape that
   uses it changes too, across all layers. A shape's own setting wins only
   when its override box is ticked.
3. **Brushes.** A line can carry a brush. Moho stamps a small image along
   the vector path, so the line looks like a natural media stroke but stays
   editable as a vector. Since 14.2 and 14.3, brushes also support pen
   pressure opacity, color drift, tighter spacing and "smart boil" (a slight
   wobble from frame to frame).
4. **Bones.** You put the drawing layers inside a *bone layer*, then draw a
   skeleton of parent and child bones and give each bone a name.
5. **Bind.** You tie the art to the bones in one of three ways:
   - Layer binding: a whole layer follows one bone. This suits separate
     limb layers.
   - Point binding: chosen points in a shape follow chosen bones. One shape
     can bend over several bones.
   - Flexi binding: points follow nearby bones, weighted by how close they
     are. Works on vectors and images.
6. **Frame 0 is setup.** Frame 0 is the rest pose. If you move a bone there,
   the change is only a test and is not saved as animation. Building and
   binding the rig happens at frame 0.
7. **Smart bones.** You pick a bone and create an action for it in the
   Actions window. Inside that action you shape the drawing (fix an elbow
   bend, turn a head, open a mouth). After that, rotating the bone plays the
   action. A free-floating smart bone used only as a control is called a
   *dial*.
8. **Switch layers.** A switch layer holds several versions of a part (mouth
   shapes, hands) and shows one at a time. A smart bone or a keyframe picks
   which one.
9. **Animate.** You go back to the main timeline. On frames after 0 you pose
   the bones, and keyframes are set for you. Moho fills in the frames between
   keyframes. Frame by frame drawing is there for effects.
10. **Export.** You render to images or video.

## Our plan compared with Moho

| Our plan | Moho | Match |
|---|---|---|
| Put a drawing on the canvas, then draw over it | Import a picture, trace it in vector layers | Yes |
| Styles, where changing one updates every line using it | Same, up to two styles per shape | Yes |
| Raster looking vector brushes | Image stamps along a vector path, with pressure options | Yes |
| Draw limbs on separate layers, pick which bone owns each drawing | Layer binding | Yes |
| Bind points of one shape to several bones, weighted | Point and flexi binding | Yes |
| Build the rig at frame 0, give each smart bone its own action timeline | Same | Yes |
| Switch layers for swapping mouths and hands | Same | Yes |
| Animate the scene | Pose bones on frames after 0, keyframes and in-betweens are automatic | Yes |

## Added to our plan

- **Bending limbs.** Layer binding alone leaves a limb stiff, pivoting at
  the joint. We add point binding (pick points, pick a bone) and flexi
  binding (each point follows nearby bones, weighted by distance), so one
  shape can bend over two bones, like an elbow.
- **Smart bone timelines.** Frame 0 stays the rest pose only. Each smart
  bone gets its own short action timeline. Its start is the bone's rest
  angle and its end is the bone's full angle. Rotating the bone picks the
  matching spot in that timeline and blends the shape fixes stored there.
- **Switch layers.** A layer that holds several versions of a part and
  shows one at a time. A keyframe or a smart bone picks which one.

## Research on the open questions

- **Brushes.** A brush is a small image stamped over and over along the
  line. Settings: spacing between stamps (Moho allows down to 3% of brush
  size), "align with curve" so each stamp turns with the line, jitter for
  random angle and spacing, and "minimize frame to frame randomness" so
  the stamps don't flicker between frames. Line boil (a slight wobble
  every few frames) is set per brush, with its own frame interval. For
  us: a seeded random number per stroke keeps the jitter steady, and the
  seed changes only on boil frames. Brush color tint was not found.
- **Bone strength.** Each bone has a region of influence, set by the Bone
  Strength tool. Points near the middle of the region move more, points
  near the edge move less. Joints bend where two regions overlap. A point
  outside every region follows the closest bone. Flexi binding lets every
  bone pull on every point, weighted by distance. It's the default and
  quick to set up, but it bends rubbery. Region binding is cleaner. A
  layer can be limited to chosen bones.
- **Two smart bones on the same points.** Moho has no clean built-in
  answer. Users add a third "fix it" smart bone or use scripts. For us:
  each action stores how far each point moves from the rest pose, and
  when two actions move the same point, the moves add up. A third
  corrective action can be driven by both bones later if needed.
- **Keyframe curves.** Set per keyframe, and they shape the motion up to
  the next key: step (jump), linear, smooth (default), ease in, ease out,
  bezier (edited in a graph), noisy (shaky), cycle (loops back to an
  earlier key), plus stagger, hold and bounce or elastic presets. A
  forum user reported that ease on one key seemed to affect both ends,
  so we define it clearly: a key's curve covers only the span after it.

## Sources

- Style window: https://www.lostmarble.com/moho/manual/stylewnd.html
- Styles shared across layers: https://www.lostmarble.com/moho/manual/tut02/07/index.html
- Frame 0 bone testing: https://www.lostmarble.com/moho/manual/tut03/02/
- Binding methods: https://lesterbanks.com/2016/11/walkthrough-moho-binding-methods/
- Smart bone actions: https://www.lostmarble.com/forum/viewtopic.php?p=155127
- Smart bone dials: https://lostmarble.com/forum/viewtopic.php?p=157619
- Brushes in 14.3: https://moho.lostmarble.com/blogs/news/moho-14-3-mantis-shrimp-free-update-the-most-powerful-brushes-for-animation
- Features: https://mohoanimation.com/pages/features
- Brush settings tips: https://lostmarble.net/forum/viewtopic.php?t=1887
- Brush spacing in 14.2: https://moho.lostmarble.com/blogs/news/moho-14-2-is-here-free-update-for-moho-14-owners
- Line boil: https://lostmarble.net/forum/viewtopic.php?t=37828
- Bone strength and regions: https://www.lostmarble.com/moho/manual/tut03/04/index.html
- Bone tools: https://www.lostmarble.com/moho/manual/bone_tools.html
- Flexi vs region binding: https://lostmarble.net/forum/viewtopic.php?t=30615
- Smart bone conflicts: https://www.lostmarble.com/forum/viewtopic.php?p=171792
- Interpolation types: https://users.monash.edu.au/~myless/catnap/mohotips/moho_interpolation.html
- Interpolation menu: https://lesterbanks.com/?p=43218
- Ease behavior report: https://www.lostmarble.com/forum/viewtopic.php?p=212480
- Video (title only, no transcript): https://www.youtube.com/watch?v=uHcw3sBaZdk
