# GF Amiga Golly

Golly 5.0, the cellular automata explorer, for AmigaOS 3 (68040; the FPU when there is one, through fpuauto), on the GFOpenWX Toolkit (wxWidgets on GFOpenLayout).

Golly and what it builds on are fetched at build time at the versions `golly.lock` pins, and `patches/` is applied to Golly; nothing third-party is kept in this repository.

## How it works

- Golly's windows, menus, dialogs, help and preferences are wx, drawn by the GFOpenWX Toolkit.
- Golly draws its pattern view with OpenGL. `glsoft/` is GLSoft, our own software GL for the calls Golly makes (2D, rectangles, lines, textures, one blend), drawing into a picture the toolkit puts on the window, and the `wxGLCanvas` it draws through.
- Lua scripts run (Lua is part of Golly's source). Python scripts need a Python library, which the Amiga doesn't have. Downloading patterns from the web is off: this wx build has no sockets.
- Help, Patterns, Rules and Scripts sit in Golly's drawer; your own rules, downloads and prefs go in its `User` drawer.

## Building

```
STOVE=~/AmiCradle/stoves/os32-gcc16-open6 OUT=out ./build-amiga.sh
```

The first run builds the toolkit's SDK, then fpuauto's library, then Golly. `out/` gets `Golly`, `bgolly` (Golly's command-line runner) and `Golly-5.0.lha`, the drawer to install.

| Where | What |
|---|---|
| `glsoft/` | GLSoft and its `wxGLCanvas` |
| `patches/` | Golly's changes for the Amiga |
| `scripts/` | the makefile and the 68040 link specs |

## Licence

Golly is GPL v2 or later; the patches and build files here are under the same licence. GLSoft (`glsoft/`) is under the wxWindows Library Licence, as the toolkit is.

Designed, tested and delivered by The AmiCradle Team.
Copyright (c) 2026 AmiCradle Team
