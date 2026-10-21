# Prototype test 1 findings

One session, 2026-10-21, mouse only (no pen), about 2 minutes, 9 tasks.
6 done, 3 skipped. Log: interaction-log-1792576893396.json.

| Task | Result | What happened |
|---|---|---|
| Draw a head | done, 14 s | Picked the pen even though it was already on, then drew. |
| Darker Skin style | done, 18 s | Two pauses over 2 s, and the pointer hovered near Skip. Clicked the style row, then the color swatch. |
| Add and rename a layer | skipped | Added the layer, then clicked its row 14 times. A prototype bug blocked double click rename (fixed now). |
| Bigger brush | done, 13 s | Scrolled the wheel over the number 7 times first, then clicked and typed 10. |
| Go to frame 12 | done, 6 s | Clicked the frame directly. |
| Back to setup pose | done, 8 s | Clicked frame 0. No keyboard. |
| Run "Export video" | skipped | 33 s searching with the most pointer travel of any task. Never opened the search box. |
| Hide Layers panel | skipped | Clicked layer rows 14 times. Never found the panel header click. |

## What it means, and changes to the design

1. **The command search wasn't found.** A button reading "Search Ctrl+K"
   doesn't say "every command is here". Change: a classic menu bar
   (File, Edit, View...) holding every command with its shortcut, so
   people can browse. Ctrl+K stays for those who know it. The search
   box shows a hint the first few times.
2. **Panel collapse was invisible.** Clicking a header to collapse wasn't
   discovered. Change: every panel header gets a visible collapse
   button and a close button, and the View menu lists every panel with
   a checkbox to bring it back.
3. **The wheel is expected to change numbers.** Change: scrolling over a
   number field changes it (Shift for bigger steps), alongside dragging
   and typing.
4. **Double click to rename is expected.** Keep it, plus F2. A single
   click on an already selected name also renames (as in file
   managers).
5. **Frames are clicked, not keyed.** The timeline must be easy to hit:
   tall frames, and frame 0 clearly marked. Keyboard steps stay as a
   bonus.
6. **Hesitation on styles.** The swatch and the row did different things
   (select vs edit). Change: clicking a style selects it and opens its
   editor in place, one obvious path.
7. **Not tested yet:** the pen tablet, onion skin, keys, panel docking.
   Next test should use the pen.

# Prototype test 2 findings

2026-10-21, mouse only, about 5 minutes, 5 think-aloud notes. Log:
interaction-log-1792577424614.json. 7 done, 1 skipped, 1 not ended.

| Task | Result | Compared with test 1 |
|---|---|---|
| Draw a head | done, 17 s | same |
| Darker Skin style | done, **128 s** | much worse, see notes |
| Add and rename a layer | done, 8 s | fixed (was skipped) |
| Bigger brush | done, 43 s | the scroll wheel worked right away, 12 steps |
| Go to frame 12 | done, 7 s | same |
| Back to setup pose | done, 21 s | slower, frame 0 not recognized |
| Run "Export video" | skipped | still not found |
| Hide Layers panel | done, 29 s | found the header after 5 row clicks |

## Your notes, and what they change

1. *"When I click a line style to switch to another I need to move my
   mouse all the way above the current color settings... I would have
   to look where the others move to."*
   The editor opening under the selected row pushes the other rows
   around. **Rule: selecting never moves other things.** Lists keep a
   fixed layout, and editors open in a fixed place, never inside the
   list.
2. *"Isn't there a better way to change color?"*
   Sliders for hue, saturation and lightness are slow. **Change:** a
   real color picker, a square for saturation and brightness, a hue
   strip, recent colors and swatches, plus the eyedropper.
3. *"Why are the color settings and the brush options separate? That's
   weird."*
   **Change:** one Properties panel (as in Figma) for whatever is
   selected or about to be drawn: style, fill, line color, width,
   brush. Tool options only hold settings that belong to the tool
   alone, like smoothing.
4. *"What is that? Oh, is it zero? It looks the same as the others."*
   A thin orange line wasn't enough. **Change:** frame 0 is its own
   labeled cell ("Setup"), filled with the setup orange and set apart
   from frame 1 by a gap.
5. *"There is no tools at the top? Is it a hotkey? Maybe search? But
   he said command, not search."*
   Confirms test 1: people look for a menu bar, and the word "Search"
   doesn't say "commands". **Change:** a menu bar, and the search box
   is called "Commands (Ctrl+K)".
6. **Panel collapse** was found this time, but only after 29 s and 5
   wrong clicks. Part of that was my wording: "hide the Layers panel"
   read as hiding a layer. The task now says "collapse". The visible
   collapse button from test 1 stays. Rule for test tasks and UI text
   alike: name the exact intent, never a word with two meanings.
7. **Prototype issue:** notes were pinned to the spot the pointer
   passed on its way to the Note button, not to what you were looking
   at. Next version pins them to where the pointer rested for longest
   in the previous 3 seconds.

# Prototype test 3 findings

2026-10-21, mouse only, 98 s in total (test 2: 296 s), no notes. Log:
interaction-log-1792577995010.json. **All 9 tasks done**, none
skipped.

| Task | Test 2 | Test 3 |
|---|---|---|
| Darker Skin style | 128 s | **12 s**, three drags on the color square |
| Thicker lines | 43 s | **9 s**, scroll wheel on Line width |
| Back to setup pose | 21 s | **7 s**, clicked the Setup cell |
| Run "Export video" | skipped | **11 s**, Commands button, typed "export", Enter |
| Collapse Layers panel | 29 s | **7 s**, two clicks on the header |

The fixed Properties panel, the color square, the Setup cell, the
"Commands" label and the clear task wording all worked.

## New from free play
1. **Arrow keys for frames.** You pressed Right and Left to step frames.
   We had comma and period. **Change:** Left and Right step frames,
   Shift+Left and Right jump to keyframes. Arrows nudge only while
   points or bones are selected on the canvas and the canvas has focus.
2. **Every tool was tried on the canvas** (bone, magnet, eyedropper),
   and each said "pick the pen". That's a prototype limit, not a
   design finding, but it shows people expect every tool to do
   something visible on its first click.
3. **Still untested:** the pen tablet, notes in this round, and the
   menu bar (Commands was used instead).
