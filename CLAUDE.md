# Snapper Studio

C++20, Qt 6.12 Widgets, KDDockWidgets 2.4, FFmpeg 9, libzip 1.11, all
installed system-wide (Arch). Design docs: ARCHITECTURE.md, ROADMAP.md,
UX_PHILOSOPHY.md.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON                    # configure, feeds clangd
cmake --build build                                     # build
ctest --test-dir build --output-on-failure              # Qt Test suite
run-clang-tidy -p build -quiet "$PWD/(src|tools|tests)/" # lint
clang-format -i src/*/*.cpp src/*/*.h src/main.cpp tests/*.cpp tools/*/*.cpp
./build/snapper-studio                                  # run
```

## Checking the UI without the user's screen

Never launch `snapper-studio` to look at it: it opens on the user's
screen. Use `snapper-drive`, which runs the app offscreen from a script
(actions listed at the top of `tools/ui-driver/main.cpp`), then read the
screenshots it saves:

```bash
printf 'size 1280 800\nwait 300\nkey Ctrl+K\ntype layers\nshot /tmp/a.png\n' \
  | ./build/snapper-drive -
```

It keeps its own settings (`~/.qttest/config/Snapper/`), separate from the
user's. Delete that folder for a fresh start.

## Gotchas

- Code builds with `-fno-rtti`, and KDDockWidgets type-checks its views at
  runtime: never subclass its MainWindow or DockWidget classes, own them.
- Save the panel layout on the window's close event, not on quit: by then
  the panels are already closed.
