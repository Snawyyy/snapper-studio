# UI and UX philosophy

## First priority: it feels smooth and natural

Quality of life comes before feature count. A feature that feels rough
isn't done.

- **Never stutter.** The canvas keeps up with the pen at 60 frames per
  second or more while drawing, dragging, scrubbing and playing. Slow
  work (export, heavy renders) runs in the background and never freezes
  the window.
- **The pen line feels right.** Pressure, adjustable stroke smoothing,
  and the line appears under the pen with no visible lag.
- **Moving around is free.** Zoom toward the cursor with the wheel or a
  pinch. Hold space to pan. Rotate the canvas. Press one key to fit the
  view.
- **Scrub everything.** Drag the timeline and the canvas follows live.
  Drag on any number field to change it, and type in it to set it
  exactly.
- **Hold a key for a quick tool switch.** Hold a tool's key to use it,
  let go to return to the last tool.
- **No popup dialogs in the way.** Settings live in panels and change
  live. Confirmations only before real data loss, and undo makes most of
  those unnecessary.
- **Remember choices.** Last brush, style, panel layout, export settings
  and window size come back next time.
- **Find any command.** A search box (Ctrl+K) runs any action by name and
  shows its shortcut.
- **See motion while drawing.** Onion skin shows nearby frames, faded.
- **Small touches of motion.** Panels and highlights ease in over about
  100 ms. Never slower, and never on the canvas itself.

## Rules learned from testing

- **Selecting never moves other things.** Lists keep their layout.
  Editors open in a fixed place, never inside the list, so your hand
  learns where things are.
- **Things that belong together sit together.** A style's color, width
  and brush are edited in one Properties panel (as in Figma), not split
  between panels.
- **Commands are browsable.** A menu bar lists every command with its
  shortcut. The search box is called "Commands (Ctrl+K)".
- **Special things look special.** Frame 0 is a labeled "Setup" cell in
  the setup orange, set apart from the other frames.
- **Every number field takes the scroll wheel**, as well as dragging
  and typing.
- **Words name the exact intent.** "Collapse panel", not "hide", when
  "hide" could also mean hiding a layer. One word, one meaning, across
  the whole app.

## Selection and hotkeys

Every key can be rebound. These are the defaults.

**Selecting**
- Click selects, Shift+click adds, Ctrl+click removes.
- Drag on empty canvas for a box select. Hold Alt while dragging for a
  freehand lasso.
- Ctrl+A select all, Ctrl+Shift+A select none, Ctrl+I invert.
- Double click a shape to select all of its points.
- Alt+click picks the topmost layer under the pen and selects it, even
  inside a group.
- Tab and Shift+Tab step through points or bones in order.
- Up and Down arrows step to a bone's parent or child.
- Selection is part of undo.

**Moving and editing**
- Arrow keys nudge 1 px, Shift+arrows nudge 10 px, only while points or
  bones are selected and the canvas has focus. Otherwise Left and Right
  step frames (see Timeline).
- Hold Shift while dragging to lock to an axis or snap angles to 15
  degrees.
- Hold Ctrl while dragging to snap to grid, points and bones.
- G move, R rotate, S scale, then type a number for an exact value.
- Ctrl+D duplicate, Delete removes, Ctrl+C / Ctrl+V copy and paste,
  Ctrl+Shift+V paste in place.
- H hides the selection, Alt+H shows everything, L locks the layer.

**Timeline**
- Left and Right arrows step one frame back and forward (comma and
  period do the same), Shift+Left and Shift+Right jump to the previous
  or next keyframe. Found in testing: arrows are what people reach for.
- Home jumps to frame 0, End to the last frame.
- Enter plays and pauses, and stopping returns to where you started
  (a setting can change that).
- K adds a keyframe for the selection, Shift+K deletes it.
- Drag keys to retime them, Alt+drag copies them.
- O toggles onion skin.

**View**
- Space+drag pans, wheel zooms to the cursor, Shift+space+drag rotates
  the canvas.
- F fits the selection, Shift+F fits the whole canvas, 1 resets to
  100%.
- Ctrl+Z undo, Ctrl+Shift+Z redo, and history is in a panel you can
  click back through.

**Tools**
- One key per tool, and holding it gives a quick switch (see above).
- [ and ] shrink and grow the brush, Shift+[ and ] change its opacity.
- Right click on the canvas opens a pie menu of the tools for the
  current layer type.
- Eyedropper on I picks a color, Alt+I picks the whole style.

## Decisions

- **Frame 0 is setup, Moho style.** One workspace. On frame 0, bone and
  binding changes edit the rig. On any other frame, they make keyframes.
- **Moho style panels.** Layers, styles, timeline, actions and tool
  options are open by default. Every panel docks, floats and rearranges,
  and the layout is saved.
- **Pen tablet first.** Pressure drives brushes. Hit targets are big
  enough for a pen. Nothing needs hover to be found. The off hand gets
  keyboard shortcuts for tools, undo, frame stepping and playback.
- **Inline hints, no wizards.** The status bar names the next step ("bind
  this layer to a bone"). No popups, no tours.

## Rules every feature follows

1. **Always show what mode you are in.** Frame 0 gets a colored canvas
   border and a "Setup pose" label. Editing a smart bone action gets a
   different color and the action's name. You should never wonder whether
   a drag edits the rig or makes a keyframe.
2. **Everything can be undone.** Every edit goes through undo, including
   style edits, binding, panel layout and timeline changes. Undo shows
   what it will undo.
3. **A control that can't act is greyed out and says why.** For example,
   "Bind Layer: select a bone first".
4. **Preview before you commit.** Dragging a style value updates the
   canvas live. Bone strength shows its region on the canvas while you
   drag. Bending a bone on frame 0 shows the deformation and snaps back on
   release.
5. **Sensible defaults.** A new project opens ready to draw: one vector
   layer, a default style and a brush. A new bone layer uses region
   binding, sized to the art.
6. **Shared things look shared.** A shape using a style shows the style's
   name. Editing a style highlights every shape it will change.
7. **No data loss.** Autosave, and recovery after a crash. Destructive
   actions (deleting a bone that has bindings) say what they will break.

## Visual language

The look follows Apple's macOS dark appearance: calm grays, soft rounded
shapes, translucent menus, and the artwork as the loudest thing on
screen. Every value below lives in one theme file, so changing the look
later touches nothing else.

- **Colors (macOS dark system grays).**
  - Window and panels `#1C1C1E`, raised surfaces (fields, menus,
    popovers) `#2C2C2E`, hover `#3A3A3C`.
  - Area around the canvas `#141416`. The canvas itself is the
    project's background color.
  - Separators: white at 10% opacity, one pixel.
  - Text: white at 85%, secondary text at 55%, disabled at 25% (Apple's
    label colors).
  - One accent for selection and focus: Teto red `#D63D55` (crimson
    with a little pink, picked by eye since no official hex exists).
    Selected list rows are a rounded pill in the accent at 25% opacity.
  - Mode colors, used only for the canvas border, the Setup cell and
    labels: setup pose (frame 0) Apple orange `#FF9F0A`, smart bone
    action edit Apple purple `#BF5AF2`.
  - Warnings Apple yellow `#FFD60A`, errors Apple red `#FF453A`. Errors
    always carry an icon and text, so they never read as a selection.
- **Away from the edges.** Nothing you click or read sits against the
  screen edge: panels keep 16 px inside padding, and a value sits right
  next to its label in a visible field, not pushed to the far side.
  Popups (pickers, menus) open inside the window, never past its edge.
- **Shapes.** Rounded: 6 px on buttons and fields, 8 px on list pills,
  10 px on menus, popovers and the command box. Spacing on an 8 px grid
  (4 px inside tight groups).
- **Depth.** Menus, popovers and the command box are translucent with a
  background blur and a soft shadow, like macOS. Panels themselves stay
  flat.
- **Density, sized for a pen.** List rows 28 px, buttons and tool icons
  32 px, icon glyphs 18 px.
- **Type.** Inter (open license, close to Apple's San Francisco, which
  can't be shipped outside Apple platforms), bundled with the app. Body
  13 px, secondary 11 px, panel titles 11 px semibold in secondary color.
- **Icons.** Lucide line icons (ISC license, SVG) at a 1.5 px stroke,
  close to SF Symbols, which are licensed for Apple platforms only.
  Drawn in the text color, accent when active. Tools Lucide lacks
  (bone, bind, smart bone) are drawn by us in the same stroke width.
- **Motion.** Small, quick eases (about 150 ms) on hover, menus and
  panel collapse. Never on the canvas itself.
- **States.** Hover lightens by one step, pressed darkens by one step,
  selected uses the accent pill, disabled uses the disabled text color
  and shows its reason in the tooltip and status bar.

## Reference design documents

When a choice is unclear, check these in order. What we take from each:

1. **Figma UI3**, our main reference for the panels around the canvas.
   https://www.figma.com/blog/behind-our-redesign-ui3/
   https://www.figma.com/blog/our-approach-to-designing-ui3/
   What we take: the work is the center, not the interface. It's friendly
   to newcomers without slowing pros down. Panels collapse but tools stay
   one click away. Labels replace cryptic fields, and rarely used fields
   are cut.
2. **Procreate Dreams handbook**, the closest well-loved animation app.
   https://help.procreate.com/dreams/handbook/interface-and-gestures
   What we take: one interface for both beginners and pros, a timeline
   with clear modes, scrubbing on the playhead, a quick fit-to-screen
   gesture, and undo history that survives closing the app.
3. **Apple Human Interface Guidelines**, for overall polish, and its
   Apple Pencil page for how drawing with a pen should feel.
   https://developer.apple.com/design/human-interface-guidelines
   https://developer.apple.com/design/human-interface-guidelines/apple-pencil-and-scribble
   What we take: hover may preview but never triggers an action, the pen
   and the mouse or finger can each be chosen freely, and it works
   equally well for left- and right-handed users.

## Open UX questions, for each feature as we build it

- What happens in the edge cases (empty layer, bone with no art, a style
  deleted while in use)?
- What does the user see while dragging, and after?
- What does undo restore?
