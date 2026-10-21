# Project file format

## The file

A project is one file, `name.snapper`. Inside it is a zip archive
(read and written with libzip):

```
project.json        the whole document
history.json        undo history, optional, may be dropped if too big
assets/images/      imported pictures, by content hash: <sha1>.png
assets/audio/       audio tracks, by content hash
assets/brushes/     brush images used by the project's styles
thumbnail.png       frame 0, 512 px wide, for file browsers
```

Assets are copied in, so a project never breaks when you move it.
Naming them by content hash means an image imported twice is stored
once.

## project.json

Readable JSON (Qt's `QJsonDocument`). Every object has an integer `id`
that is unique in the project. Links between objects use ids.

```json
{
  "format": "snapper",
  "version": 1,
  "app_version": "0.1.0",
  "canvas": { "width": 1920, "height": 1080, "fps": 24,
              "frames": 240, "background": "#FFFFFF" },
  "next_id": 1043,
  "styles":   [ { "id": 5, "name": "Skin", "fill": "#F2C9A0",
                  "line": "#3A2A20", "width": 3.0, "brush": 7 } ],
  "brushes":  [ { "id": 7, "name": "Pencil", "image": "assets/brushes/<sha1>.png",
                  "spacing": 0.05, "align": true, "angle_jitter": 10,
                  "spacing_jitter": 0.1, "boil_every": 0 } ],
  "layers":   [ { "id": 10, "parent": 0, "type": "bone", "name": "Body",
                  "visible": true, "locked": false } ],
  "shapes":   [ { "id": 200, "layer": 11, "points": [201, 202, 203],
                  "closed": true, "styles": [5] } ],
  "points":   [ { "id": 201, "x": 10.0, "y": 4.5, "curve": 0.33 } ],
  "bones":    [ { "id": 300, "layer": 10, "parent": 0, "name": "upper_arm",
                  "x": 0.0, "y": 0.0, "angle": 1.57, "length": 80.0,
                  "strength": 40.0 } ],
  "bindings": [ { "kind": "layer", "layer": 11, "bone": 300 },
                { "kind": "points", "points": [201, 202], "bone": 301 } ],
  "channels": [ { "id": 400, "target": 300, "property": "angle",
                  "keys": [ { "t": 12, "v": 0.8, "curve": "smooth" } ] } ],
  "actions":  [ { "id": 500, "bone": 301, "min_angle": 0.0,
                  "max_angle": 2.4, "channels": [401, 402] } ]
}
```

Lists are flat. Trees (layers, bones) use `parent` ids, and parents are
written before children, so loading needs one forward pass. `parent: 0`
means top level.

## Rules

- **Versioned.** `version` goes up whenever the format changes. The app
  opens every older version by upgrading it step by step (1 to 2 to 3),
  one small function per step. A newer version than the app knows opens
  read-only, with a message.
- **Safe saving.** Write to `name.snapper.tmp`, flush it, then rename it
  over the old file, so a crash mid-save never destroys the project.
- **Autosave.** Every 2 minutes and on focus loss, to a separate
  recovery file next to the app settings, never over the user's file. On
  start, a recovery newer than its project is offered back.
- **Checked on load.** Every id link is checked. A broken link is
  dropped with a warning listing what was lost, and the project still
  opens.
- **Limits.** Load rejects files over the hard limits (for example more
  than `kMaxPointsPerProject`) with a clear message, instead of hanging.
- **Units.** Positions in canvas pixels, angles in radians, time in
  frames.
