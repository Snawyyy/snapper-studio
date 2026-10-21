# Architecture

The app is a set of managers. Each manager owns one concern and its own
data, and others reach it only through a small interface. The UI never
edits data directly: every change is a command that goes through undo.

## Ground rules (from Snawy's Law, applied here)

- **Ids, not pointers.** Layers, shapes, points, bones, styles, actions
  and keys are named by integer ids. Managers hand out ids and look them
  up. No object holds a pointer into another manager's data.
- **Flat trees, no recursion.** The layer tree and each skeleton are
  stored as flat arrays where every item records its parent's index, and
  a parent always comes before its children. One forward loop then visits
  parents before children, so posing bones and walking layers need no
  recursion and the loop is bounded by the array size.
- **Hard limits.** Every collection has a maximum size, a named constant
  (for example `kMaxBonesPerSkeleton = 512`). Adding past it fails with a
  message in the UI.
- **No exceptions, no RTTI.** Fallible calls return `std::optional` or a
  small result type, and the caller checks it.
- **RAII for everything external:** files, zip archives, FFmpeg
  contexts, audio devices.

## The managers

### Data (the document)
| Manager | Owns |
|---|---|
| `StyleManager` | Styles: fill, line color, width, brush id, effects. Tells listeners which shapes a style touches. |
| `BrushManager` | Brush images and their settings (spacing, align to curve, jitter, boil interval). |
| `LayerManager` | The layer tree: vector, image, group, bone and switch layers, plus order, visibility and lock. |
| `ShapeManager` | Vector layer contents: points, curves, shapes, and each shape's style ids. |
| `SkeletonManager` | Bones of each bone layer: rest pose (frame 0), parent, length, strength (region size), constraints. |
| `BindingManager` | What follows what: layer to bone, points to bones, region and flexi weights. |
| `ChannelManager` | Every animated value is a channel with keys. Each key has a time, a value and a curve type. Bone angles, point offsets, layer transforms and switch choices are all channels. |
| `ActionManager` | Smart bone actions: a driving bone, an angle range, and their own channels holding point offsets from the rest pose. |

These eight together are the `Document`. The `Document` holds them,
saves and loads them, and is the only thing commands may change.

### Computing a frame
`FrameEvaluator` turns the document plus a time into a finished frame,
in fixed order:

1. Read every channel at the time (`ChannelManager`).
2. Pose the bones, parents first, with IK where set (`SkeletonManager`).
3. Add smart bone actions: each driving bone's angle picks a spot in its
   action, and the point offsets from all actions add up.
4. Deform points by their bindings (`BindingManager`).
5. Pick the shown child of each switch layer.

The result is a plain list of shapes with final positions and styles,
ready to draw. Frames are cached by time, and an edit clears only the
frames it affects.

### Drawing
| Manager | Job |
|---|---|
| `Renderer` | Draws a finished frame with `QPainter`: fills, plain lines, and hands brush lines to the brush stamper. |
| `BrushStamper` | Stamps brush images along a line with spacing, turn and seeded jitter. The seed changes only on boil frames. |
| `OverlayRenderer` | Draws the editing aids on top: bones, regions, selection, onion skin, mode border. |

### Editing
| Manager | Job |
|---|---|
| `UndoManager` | Wraps `QUndoStack`. Every edit is a command with do and undo. Selection changes are commands too. History is saved with the project. |
| `SelectionManager` | What's selected (ids), per kind: points, shapes, bones, layers, keys. |
| `ToolManager` | The active tool, hold-to-switch, and handing pen events to the tool. Each tool is a class implementing a small `Tool` interface. |
| `CommandRegistry` | Every user action by name, with its shortcut. It feeds menus, the Ctrl+K search and rebinding. |

### Around the app
| Manager | Job |
|---|---|
| `ProjectIo` | Save, load, autosave and crash recovery (see FILE_FORMAT.md). |
| `ExportManager` | PNG sequence and video through FFmpeg, on a worker thread with its own copy of the document. |
| `AudioManager` | Audio tracks and playback through Qt Multimedia, kept in sync with the timeline. |
| `ThemeManager` | Loads the theme file (colors, sizes, icons) and applies it. |
| `SettingsManager` | Remembered choices: last brush, layout, window, export settings (`QSettings`). |
| `StudyManager` | Usability recording, off by default and turned on by the user. Records pointer and pen movement (throttled), clicks with the name of the control hit, keys, commands run through `CommandRegistry`, undo use and panel changes. A hotkey opens a note box pinned to where the pointer rested: "what I'm trying to do" plus a frustration level from 1 to 5. Writes one local file per session, never uploads, never records artwork. A small script turns sessions into per-task metrics (time, travel, wrong clicks, pauses, notes). |

### UI panels (KDDockWidgets)
Canvas, Layers, Styles, Brushes, Timeline, Actions, Tool Options,
History. Panels only read from managers and send commands. Each panel
listens for change signals and redraws only what changed.

## How a change flows

```
pen event -> ToolManager -> Tool builds a command -> UndoManager runs it
  -> a Document manager changes its data and sends "changed (ids)"
  -> FrameEvaluator drops the affected cached frames
  -> canvas and panels redraw
```

## Threads

Editing, evaluating and drawing the visible frame run on the UI thread,
which keeps them simple and is fast enough at our sizes. Export and
autosave work on a copy of the document in a worker thread. Audio runs
on Qt Multimedia's own thread.

## Folder layout

```
src/
  document/   the eight data managers, Document
  eval/       FrameEvaluator, curve math, IK
  render/     Renderer, BrushStamper, OverlayRenderer
  edit/       UndoManager, commands, SelectionManager, ToolManager, tools/
  app/        ProjectIo, ExportManager, AudioManager, ThemeManager,
              SettingsManager, CommandRegistry
  ui/         main window and panels
tests/        Qt Test, one file per manager
theme/        theme file and icons
```
