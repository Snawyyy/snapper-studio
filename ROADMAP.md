# Build order

Each milestone ends with something you can open and use. A milestone is
done only when it builds with all warnings fixed, passes its tests and
the linter, and its UX questions have been asked and answered.

## Names
The app is **Snapper Studio**. The program and package are
`snapper-studio`, since Linux already ships a backup tool called
`snapper`. Project files are `.snapper`. GitHub repo:
`git@github.com:Snawyyy/snapper-studio.git`.

## M0. Empty app that builds
Git repo, CMake project, Qt and KDDockWidgets linked, the main window
with empty docked panels, the theme applied (dark gray, Teto red),
command registry with Ctrl+K search, settings remembered, Qt Test
running.
**You can:** open the app, move panels around, and they stay put next
time.

## M1. Draw
Canvas with smooth zoom, pan and rotate. Vector layers. Pen tool with
pressure and smoothing, shapes with fill and line. Select, move and edit
points. Undo and redo with a history panel. Save and open `.snapper`
files, autosave.
Study mode (recording plus notes with a frustration level), so every
later milestone gets tested the way the prototype was.
**You can:** draw a character over an imported picture and save it, and
record a test session of doing it.

## M2. Styles and brushes
Styles panel, shapes using up to two styles, editing a style updates
every shape live. Brush stamping with spacing, align, jitter and boil.
Eyedropper for colors and styles.
**You can:** change one style and watch every line of the character
change, with natural media looking lines.

## M3. Timeline and keyframes
Timeline panel, frames, playback, onion skin. Channels and keys for
layer transforms and points. All curve types. Scrubbing with live
canvas.
**You can:** animate shapes and layers by hand and play them back.

## M4. Bones
Bone layers, adding bones, frame 0 as the setup pose with its orange
border, posing bones on other frames, IK. Layer binding.
**You can:** rig a character with limbs on separate layers and animate
it with bones.

## M5. Bending
Point binding, bone strength regions shown on the canvas, flexi
binding.
**You can:** bend one shape over two bones, like an elbow.

## M6. Smart bones and switch layers
Actions panel, smart bone actions with their purple editing mode,
offsets adding up, dials. Switch layers driven by keys or smart bones.
**You can:** build a rig with head turns, corrected elbows and mouth
swaps.

## M7. Sound and export
Audio tracks synced to the timeline. Export to PNG sequence and video
through FFmpeg, in the background with progress.
**You can:** finish a short, lip synced scene and share the video.

## Later, only when asked for
Lua scripting, PSD import, mesh warping for pictures, physics, light
theme.
