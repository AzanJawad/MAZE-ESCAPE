# MAZE ESCAPE

A 2D top-down maze game with traps, hidden dangers and collectible stars.

**Programming Fundamentals Project (Group 10)**
- Muhammad Azan Jawad (26L-0736)
- Faseeh Ahmed Tayyab (26L-0735)

Written in C++17 with SFML 3.x.

---

## Game Description

You are trapped inside a dangerous maze. Find the glowing green exit portal in each of the three levels, but be careful: the maze is full of traps. Doors look identical, yet only one is safe. Floors that look safe may hide spikes or explosives. Fire vents burn in cycles, so timing matters.

You have **3 lives** for the whole game. If you lose all of them, it is game over. Every level also hides up to **3 stars** in random reachable spots, and collecting them improves your rating (a level can still be finished with 0 stars).

### Levels

| # | Name | Highlights |
|---|------|------------|
| 1 | The Beginning | Winding maze, fire traps, and a set of doors guarding the exit |
| 2 | The Hidden Danger | Many hidden spike traps and several door puzzles |
| 3 | The Final Escape | Fire traps, hidden booby traps (explosions), and the final door puzzle |

### Traps and Objects

| Object | Behaviour |
|--------|-----------|
| Safe door | Passes through safely |
| Trap door | Looks the same as a safe door, but kills you and turns red with an X when triggered |
| Fire trap | Burns for 2 s, then is off for 1.5 s. It flickers just before igniting, so watch the timing |
| Hidden spikes | Almost invisible floor tile; spikes shoot up when stepped on |
| Booby trap | Hidden floor tile that explodes when stepped on |
| Star | Collect by touching it; up to 3 per level |
| Exit portal | Pulsing green orb that completes the level |

---

## Features

- 3 handcrafted levels with increasing difficulty
- Random star placement every time a level loads (stars are only placed on tiles the player can actually reach, checked with a flood-fill)
- Lives system (3 lives) and star rating (0-3 per level, 9 in total)
- Smooth movement with wall sliding and diagonal movement
- Animated graphics: player walking animation, flickering flames, pulsing exit, death and explosion effects
- All graphics are drawn with SFML shapes, so no image files are needed
- Background music (looping)
- Menu, instructions, pause, level complete, game over and victory screens

---

## Controls

| Key | Action |
|-----|--------|
| Arrow Keys | Move the player |
| Enter | Start game / continue / return to menu |
| I | Open instructions (from the main menu) |
| P | Pause / resume |
| R | Restart the current level |
| Esc | Back to menu (Esc on the main menu quits the game) |

---

## Project Files

```
MAZE/
├── main.cpp                           # Complete game source code
├── arial.ttf                          # Font used for all text
├── playlistsons-pyramid-311175.ogg    # Background music
├── main.exe                           # Pre-built Windows executable
└── .clangd                            # Editor configuration (optional)
```

---

## Requirements

- A C++17 compiler (g++ / MinGW-w64, clang++, or MSVC)
- [SFML 3.x](https://www.sfml-dev.org/download.php) (Graphics, Window, System and Audio modules)

> SFML 3 is **not** compatible with SFML 2.x code, so make sure you download version 3.

---

## How to Compile

`arial.ttf` and the `.ogg` music file must be in the same folder as the executable when you run the game.

### Windows (MinGW-w64 / g++)

Assuming SFML is extracted to `C:/SFML`:

```bash
g++ main.cpp -o main.exe -std=c++17 -IC:/SFML/include -LC:/SFML/lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
```

Copy the SFML DLL files from `C:/SFML/bin` (e.g. `sfml-graphics-3.dll`, `sfml-window-3.dll`, `sfml-system-3.dll`, `sfml-audio-3.dll`) into the same folder as `main.exe`, or add `C:/SFML/bin` to your PATH.

### Linux

Install SFML 3 (build from source or use a package manager that provides version 3), then:

```bash
g++ main.cpp -o main -std=c++17 -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
```

### macOS

```bash
clang++ main.cpp -o main -std=c++17 -I/path/to/SFML/include -L/path/to/SFML/lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
```

### Visual Studio Code

1. Install SFML 3.x and a MinGW-w64 compiler.
2. Open the project folder in VS Code.
3. Update the include path in `.clangd` if your SFML folder is not `C:/SFML`.
4. Build with the g++ command above (or a `tasks.json` that runs it).

---

## How to Run

```bash
./main        # Linux / macOS
main.exe      # Windows (or just double-click it)
```

If the window opens but there is no text or music, check that `arial.ttf` and the `.ogg` file are in the same folder as the executable.

---

## Code Overview (`main.cpp`)

| Section | Purpose |
|---------|---------|
| Constants and game states | Grid size, tile size, speed, and the `GameState` enum (MENU, INSTRUCTIONS, PLAYING, PAUSED, LEVEL_COMPLETE, GAME_OVER, VICTORY) |
| Level maps | Each level is an array of text rows using characters for walls, floors, traps, doors and the exit |
| `floodFill` / `loadLevel` | Loads a map, finds the reachable tiles and places the stars randomly |
| `movePlayer` / `blocked` | Movement with corner-based wall collision (X and Y tested separately) |
| `update` | Trap checks, death handling, star collection, exit detection |
| `draw...` functions | Tiles, traps, player, effects, HUD and all screens |
| `handleKey` | Keyboard handling for every game state |
| `main` | Window, font and music setup, and the main game loop |

### Level Map Legend

| Char | Meaning | Char | Meaning |
|------|---------|------|---------|
| `#` | Wall | `E` | Exit |
| `.` | Floor | `D` | Safe door |
| `P` | Player start | `X` | Trap door (looks like `D`) |
| `F` | Fire trap | `H` | Hidden spike trap |
| `B` | Booby trap (hidden explosion) | | |

---

## Credits

- Game design and programming: Muhammad Azan Jawad and Faseeh Ahmed Tayyab
- Graphics, window and audio library: [SFML](https://www.sfml-dev.org/)
- Music: "Pyramid" (file `playlistsons-pyramid-311175.ogg`)
