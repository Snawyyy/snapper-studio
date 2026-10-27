# Project file format

## The file

A project is one file, `name.snapper`. Inside it is a zip archive
(read and written with libzip):

```
project.json        the whole document
history.json        undo history, optional, may be dropped if too big
assets/images/      imported pictures and brush pictures, by content
                    hash: <sha1>.png
assets/audio/       audio tracks, by content hash
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
  "version": 4,
  "app_version": "0.1.0",
  "canvas": { "width": 1920, "height": 1080, "fps": 24,
              "frames": 240, "background": "#FFFFFF" },
  "next_id": 1043,
  "styles":   [ { "id": 5, "name": "Skin", "fill": "#F2C9A0",
                  "line": "#3A2A20", "width": 3.0, "brush": 7 } ],
  "brushes":  [ { "id": 7, "name": "Pencil", "image": "assets/images/<sha1>.png",
                  "spacing": 0.2, "align": false, "angle_jitter": 180,
                  "spacing_jitter": 0.0, "size_jitter": 0.1,
                  "boil_every": 2 } ],
  "layers":   [ { "id": 10, "parent": 0, "type": "bone", "name": "Body",
                  "visible": true, "locked": false } ],
  "shapes":   [ { "id": 200, "layer": 11, "points": [201, 202, 203],
                  "closed": true, "styles": [5],
                  "fill": "#FFF2C9A0", "line": null, "line_width": 3.0,
                  "brush": 7 } ],
  "points":   [ { "id": 201, "x": 10.0, "y": 4.5, "curve": 0.33,
                  "width": 1.0 } ],
  "bones":    [ { "id": 300, "layer": 10, "parent": 0, "name": "upper_arm",
                  "x": 0.0, "y": 0.0, "angle": 1.57, "length": 80.0,
                  "scale": 1.0, "pinned": false } ],
  "bindings": [ { "kind": "layer", "layer": 11, "bone": 300 } ],
  "channels": [ { "target": 300, "property": "bone_angle",
                  "keys": [ { "t": 12, "v": 0.8, "curve": "smooth" } ] },
                { "target": 201, "property": "point",
                  "keys": [ { "t": 6, "x": 12.0, "y": 9.5,
                              "curve": "noisy", "param": 4.0 } ] } ],
  "actions":  [ { "id": 500, "bone": 301, "min_angle": 0.0,
                  "max_angle": 2.4, "channels": [401, 402] } ]
}
```

A shape's own `fill` and `line` are colors, or null for none, and
`line_width` is its full line width. A point's `curve` runs from 0 (a
sharp corner) to about 0.55 (round), and its `width` scales the line
width there, from pen pressure. A shape's own `brush` is left out for a
plain line.

A shape names up to two `styles`, written [first] or [0, second] when
only the second is set. Each style holds only the parts it sets, out of
`fill`, `line` (colors, or null for none), `width` and `brush` (0 for a
plain line). A shape draws with its own look, then each part its first
style sets, then each part its second style sets.

A brush stamps its picture along a line: the picture's alpha is how much
paint each spot lays down, tinted by the line color. `spacing` is the gap
between stamps as a share of the line width, `align` turns them along the
line, `angle_jitter` is in degrees either way, `spacing_jitter` and
`size_jitter` are shares, and `boil_every` is the number of frames
between new random stamps (0 never).

A layer that has been moved, turned or scaled has an `origin` [x, y],
the spot it turns and scales round in its own space, and a `transform`
{ `x`, `y`, `angle`, `sx`, `sy` } for its setup pose on frame 0.

A bone layer holds the layers whose `parent` it is. Its bones each name
the bone `layer` they belong to and their `parent` bone, 0 for none, and
come after their parent. A bone's `x` and `y` are where its base sits in
its parent's space: the parent's base at 0 with the parent running along
x, its `x` stretched with the parent. With no parent they are in the
bone layer's space. `angle` is from the parent's direction, `length` in
pixels, `scale` stretches it along its length (left out at 1), and
`pinned` stops IK at it (left out when false). A `layer` binding makes a
whole layer follow a bone of the bone layer it sits in.

A channel animates one `property` of its `target`: `point` (a point's
position), a layer's `position`, `angle` or `scale`, or a bone's
`bone_position`, `bone_angle` or `bone_scale`. There is at most
one per target and property. Frame 0 is the setup pose, taken from the
target itself, so keys sit on frame `t` 1 and later. Angles and bone
scale have one value `v`, the others `x` and `y`. A key's `curve` shapes the span after
it, up to the next key: `smooth`, `linear`, `step`, `ease_in`,
`ease_out`, `ease_in_out`, `noisy`, `cycle`, `bounce` or `elastic`. For
`noisy`, `param` is how far the value shakes; for `cycle`, how many
frames back it loops, left out for back to the key before.

Lists are flat. Trees (layers, bones) use `parent` ids, and parents are
written before children, so loading needs one forward pass. `parent: 0`
means top level.

## Rules

- **Versioned.** `version` goes up whenever the format changes.
  Version 2 added styles and brushes: a version 1 project opens with the
  default style and the standard brushes added. Version 3 added
  channels and layer transforms, and version 4 bone layers, bones and
  bindings, which older projects simply don't have. The app
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
