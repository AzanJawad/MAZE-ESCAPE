//  MAZE ESCAPE  -  2D maze game in C++ with SFML 3.x
//  Programming Fundamentals Project  (Group 10)
//  Faseeh Ahmed Tayyab (26L-0735), Muhammad Azan Jawad (26L-0736)
//  Everything is drawn with SFML shapes, so NO image files are
//  needed. Only a font file (arial.ttf) is needed for the text.

#include <SFML/Graphics.hpp>
#include <optional>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <SFML/Audio.hpp>

using namespace sf;
using namespace std;


//   Constants and game states

const int   COLS = 20, ROWS = 15, TILE = 40, HUD = 40;
const int   WIN_W = COLS * TILE;
const int   WIN_H = ROWS * TILE + HUD;
const float PLAYER_HALF = 12.f;
const float SPEED = 150.f;
const float PI = 3.14159265f;


Vector2f V(float x, float y) { return Vector2f(x, y); }

enum GameState { MENU, INSTRUCTIONS, PLAYING, PAUSED, LEVEL_COMPLETE, GAME_OVER, VICTORY };


// Level maps (each level is an array of text rows)
//    #  wall          .  floor           P  player start
//    E  exit          D  SAFE door       X  trap door (looks like D)
//    F  fire trap     H  hidden floor trap (spikes)
//    B  booby trap (hidden explosion)

const char* LEVELS[3][ROWS] = {
    {
        "####################",
        "#P.....#...........#",
        "#.####.#.#########.#",
        "#.#....#.#.......#.#",
        "#.#.####.#.#####.#.#",
        "#.#......#.#...#.#.#",
        "#.########.#.#.#.#.#",
        "#....F.....#.#...#.#",
        "##########.#.#####.#",
        "#........#.#.......#",
        "#.######.#.#########",
        "#.#....#.#.......X.#",
        "#.#.##.#.#.......DE#",
        "#...##.....F.....X.#",
        "####################"
    },
    {
        "####################",
        "#P....H......H.#...#",
        "#.......#.H........#",
        "###########X#D#X####",
        "#....#.....H.......#",
        "#..H....H.....#....#",
        "##X#D#X#############",
        "#.....H.#..........#",
        "#..#......H...#.H..#",
        "#############X#D#X##",
        "#..#....#..H.H.....#",
        "#....H.........#...#",
        "##X#X#D#############",
        "#.........F...F...E#",
        "####################"
    },
    {
        "####################",
        "#P....#.....F......#",
        "#####.#.##########.#",
        "#...#.#.#........#.#",
        "#.#.#.#.#.######.#.#",
        "#.#.....#.#....#.#.#",
        "#.#######.#.##.#.#.#",
        "#.........#.##...#.#",
        "####X######D######.#",
        "#.......F.......#..#",
        "#.##################",
        "#.#...B.........X..#",
        "#.#.......B.....DE.#",
        "#.......B....B..X..#",
        "####################"
    }
};
const char* LEVEL_NAMES[3] = { "The Beginning", "The Hidden Danger", "The Final Escape" };


//  Global game data

char  grid[ROWS][COLS];        // current level map
bool  revealed[ROWS][COLS];    // traps that have already been triggered
bool  reach[ROWS][COLS];       // used for stars

GameState state = MENU;
int   currentLevel = 0;
int   lives = 3;
float gameTime = 0;          
float uiTime = 0;              

float px, py, startX, startY;  // player centre and start position
int   faceX = 1;
bool  moving = false;

bool   dying = false;          // death animation
float  deathTimer = 0;
string deathMsg;
int    deathTX = 0, deathTY = 0;
char   deathType = ' ';

int   starTX[3], starTY[3];
bool  starTaken[3];
int   starsThisLevel = 0;
int   levelStars[3] = { 0, 0, 0 };
Music music;
Font  font;

bool  fontOk = false;


//  Small helper functions

void loadFont()
{
    const char* paths[] = {
        "arial.ttf", "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf"
    };
    for (int i = 0; i < 5; i++)
        if (font.openFromFile(paths[i])) { fontOk = true; return; }
}

void drawText(RenderWindow& w, const string& s, int size, float x, float y,
              Color col, bool center = false)
{
    if (!fontOk) return;
    Text t(font, s, size);
    t.setFillColor(col);
    if (center) {
        FloatRect b = t.getLocalBounds();
        t.setOrigin(V(b.position.x + b.size.x / 2.f, 0));
    }
    t.setPosition(V(x, y));
    w.draw(t);
}

// Flood-fill from the start so stars only appear where the player can really go
void floodFill(int sx, int sy)
{
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++) reach[y][x] = false;

    int qx[ROWS * COLS], qy[ROWS * COLS];
    int head = 0, tail = 0;
    qx[tail] = sx; qy[tail] = sy; tail++;
    reach[sy][sx] = true;
    int dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };

    while (head < tail) {
        int x = qx[head], y = qy[head]; head++;
        for (int k = 0; k < 4; k++) {
            int nx = x + dx[k], ny = y + dy[k];
            if (nx < 0 || ny < 0 || nx >= COLS || ny >= ROWS) continue;
            if (reach[ny][nx]) continue;
            char c = grid[ny][nx];
            if (c == '#' || c == 'X' || c == 'H' || c == 'B' || c == 'E') continue;
            reach[ny][nx] = true;
            qx[tail] = nx; qy[tail] = ny; tail++;
        }
    }
}

// Loading a level

void loadLevel(int n)
{
    currentLevel = n;
    int stx = 1, sty = 1;
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++) {
            grid[y][x] = LEVELS[n][y][x];
            revealed[y][x] = false;
            if (grid[y][x] == 'P') {
                grid[y][x] = '.';
                startX = x * TILE + TILE / 2.f;
                startY = y * TILE + TILE / 2.f;
                stx = x; sty = y;
            }
        }
    px = startX; py = startY;
    dying = false;
    starsThisLevel = 0;
    gameTime = 0;

    floodFill(stx, sty);

    // collecting all reachable floor tiles, then pick up to 3 at random
    int cx[ROWS * COLS], cy[ROWS * COLS], count = 0;
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++)
            if (grid[y][x] == '.' && reach[y][x] && !(x == stx && y == sty)) {
                cx[count] = x; cy[count] = y; count++;
            }
    for (int i = 0; i < 3; i++) {
        starTaken[i] = true;          
        if (count == 0) continue;
        int r = rand() % count;
        starTX[i] = cx[r]; starTY[i] = cy[r];
        starTaken[i] = false;
        cx[r] = cx[count - 1]; cy[r] = cy[count - 1]; count--;
    }
}

void startNewGame()
{
    lives = 3;
    for (int i = 0; i < 3; i++) levelStars[i] = 0;
    loadLevel(0);
    state = PLAYING;
}

// Movement + wall collision

bool isWallAt(float x, float y)
{
    int tx = (int)floor(x / TILE), ty = (int)floor(y / TILE);
    if (tx < 0 || ty < 0 || tx >= COLS || ty >= ROWS) return true;
    return grid[ty][tx] == '#';
}

bool blocked(float cx, float cy)   // checks the 4 corners of the player's box
{
    float h = PLAYER_HALF;
    return isWallAt(cx - h, cy - h) || isWallAt(cx + h, cy - h) ||
           isWallAt(cx - h, cy + h) || isWallAt(cx + h, cy + h);
}

void movePlayer(float dt)
{
    float dx = 0, dy = 0;
    if (Keyboard::isKeyPressed(Keyboard::Key::Left))  dx -= 1;
    if (Keyboard::isKeyPressed(Keyboard::Key::Right)) dx += 1;
    if (Keyboard::isKeyPressed(Keyboard::Key::Up))    dy -= 1;
    if (Keyboard::isKeyPressed(Keyboard::Key::Down))  dy += 1;

    moving = (dx != 0 || dy != 0);
    if (dx != 0 && dy != 0) { dx *= 0.7071f; dy *= 0.7071f; }
    if (dx > 0) faceX = 1;
    if (dx < 0) faceX = -1;

    float nx = px + dx * SPEED * dt;
    if (!blocked(nx, py)) px = nx;          // X and Y are tested separately
    float ny = py + dy * SPEED * dt;        // so the player can slide along walls
    if (!blocked(px, ny)) py = ny;
}

// Traps, doors, death, stars, exit

bool fireOn(int tx, int ty)        // fire cycle: 2s burning, 1.5s off
{
    float t = fmod(gameTime + tx * 0.6f + ty * 0.4f, 3.5f);
    return t < 2.0f;
}
bool fireWarning(int tx, int ty)   // short flicker just before it ignites
{
    float t = fmod(gameTime + tx * 0.6f + ty * 0.4f, 3.5f);
    return t >= 3.0f;
}

void killPlayer(const string& msg, int tx, int ty, char type)
{
    if (dying) return;
    dying = true;
    deathTimer = 0;
    deathMsg = msg;
    deathTX = tx; deathTY = ty; deathType = type;
}

void update(float dt)
{
    gameTime += dt;

    if (dying) {                         // play the death animation, then respawn
        deathTimer += dt;
        if (deathTimer >= 1.2f) {
            lives--;
            if (lives <= 0) state = GAME_OVER;
            else { px = startX; py = startY; dying = false; }
        }
        return;
    }

    movePlayer(dt);

    int tx = (int)(px / TILE), ty = (int)(py / TILE);
    char c = grid[ty][tx];

    if (c == 'X') {
        revealed[ty][tx] = true;
        killPlayer("Wrong door! A trap was waiting behind it.", tx, ty, 'X');
    }
    else if (c == 'H') {
        revealed[ty][tx] = true;
        killPlayer("Hidden spikes shot out of the floor!", tx, ty, 'H');
    }
    else if (c == 'B') {
        revealed[ty][tx] = true;
        killPlayer("BOOM! You stepped on a booby trap.", tx, ty, 'B');
    }
    else if (c == 'F' && fireOn(tx, ty)) {
        killPlayer("You were burned by a fire trap!", tx, ty, 'F');
    }
    else if (c == 'E') {
        levelStars[currentLevel] = starsThisLevel;
        state = LEVEL_COMPLETE;
        return;
    }

    for (int i = 0; i < 3; i++) {        // collect stars by touching them
        if (starTaken[i]) continue;
        float sx = starTX[i] * TILE + TILE / 2.f, sy = starTY[i] * TILE + TILE / 2.f;
        float d = sqrt((px - sx) * (px - sx) + (py - sy) * (py - sy));
        if (d < 22) { starTaken[i] = true; starsThisLevel++; }
    }
}

// Drawing (tiles, traps, animations, player, HUD)

void drawStar(RenderWindow& w, float cx, float cy, float r, Color col)
{
    ConvexShape s(10);
    for (int i = 0; i < 10; i++) {
        float a = -PI / 2 + i * PI / 5;
        float rad = (i % 2 == 0) ? r : r * 0.45f;
        s.setPoint(i, Vector2f(cx + cos(a) * rad, cy + sin(a) * rad));
    }
    s.setFillColor(col);
    w.draw(s);
}

void drawFire(RenderWindow& w, float x, float y, bool on, bool warn)
{
    RectangleShape vent(Vector2f(26, 8));
    vent.setPosition(V(x + 7, y + 30));
    vent.setFillColor(Color(25, 25, 25));
    w.draw(vent);

    if (on) {                                     // flickering flames
        for (int i = 0; i < 3; i++) {
            float h = 14 + 6 * sin(gameTime * 14 + i * 2.1f);
            float cx = x + 10 + i * 10;
            ConvexShape flame(3);
            flame.setPoint(0, Vector2f(cx - 6, y + 32));
            flame.setPoint(1, Vector2f(cx + 6, y + 32));
            flame.setPoint(2, Vector2f(cx, y + 32 - h - 6));
            flame.setFillColor(i == 1 ? Color(255, 215, 60) : Color(255, 100, 20));
            w.draw(flame);
        }
    } else if (warn) {                            // about to ignite
        CircleShape glow(5 + 2 * sin(gameTime * 30));
        glow.setOrigin(V(glow.getRadius(), glow.getRadius()));
        glow.setPosition(V(x + 20, y + 28));
        glow.setFillColor(Color(255, 140, 30, 200));
        w.draw(glow);
    } else {                
        CircleShape ember(2);
        ember.setFillColor(Color(150, 40, 20));
        ember.setPosition(V(x + 12, y + 26)); w.draw(ember);
        ember.setPosition(V(x + 24, y + 24)); w.draw(ember);
    }
}

void drawDoor(RenderWindow& w, float x, float y, bool trapRevealed)
{
    RectangleShape frame(Vector2f(TILE - 6, TILE - 4));
    frame.setPosition(V(x + 3, y + 2));
    frame.setFillColor(Color(95, 60, 25));
    w.draw(frame);

    RectangleShape panel(Vector2f(TILE - 14, TILE - 10));
    panel.setPosition(V(x + 7, y + 6));
    panel.setFillColor(trapRevealed ? Color(160, 30, 30) : Color(160, 105, 50));
    w.draw(panel);

    if (trapRevealed) {                           // white X on a triggered trap door
        RectangleShape bar(Vector2f(28, 4));
        bar.setOrigin(V(14, 2));
        bar.setPosition(V(x + 20, y + 20));
        bar.setFillColor(Color::White);
        bar.setRotation(degrees(45));  w.draw(bar);
        bar.setRotation(degrees(-45)); w.draw(bar);
    } else {
        CircleShape knob(3);
        knob.setFillColor(Color(240, 200, 60));
        knob.setPosition(V(x + 26, y + 19));
        w.draw(knob);
    }
}

void drawTiles(RenderWindow& w)
{
    RectangleShape r(Vector2f(TILE, TILE));
    for (int ty = 0; ty < ROWS; ty++) {
        for (int tx = 0; tx < COLS; tx++) {
            float x = tx * TILE, y = ty * TILE;
            char c = grid[ty][tx];

            if (c == '#') {                       // brick wall
                r.setPosition(V(x, y));
                r.setFillColor(Color(60, 60, 78));
                w.draw(r);
                RectangleShape line(Vector2f(TILE, 2));
                line.setFillColor(Color(38, 38, 50));
                line.setPosition(V(x, y + 13)); w.draw(line);
                line.setPosition(V(x, y + 27)); w.draw(line);
                RectangleShape v(Vector2f(2, 13));
                v.setFillColor(Color(38, 38, 50));
                v.setPosition(V(x + ((tx + ty) % 2 ? 10 : 28), y));      w.draw(v);
                v.setPosition(V(x + ((tx + ty) % 2 ? 28 : 10), y + 14)); w.draw(v);
                continue;
            }

            // everything else sits on a floor tile
            r.setPosition(V(x, y));
            r.setFillColor((tx + ty) % 2 ? Color(88, 82, 74) : Color(84, 78, 70));
            w.draw(r);

            if (c == 'H' || c == 'B') {
                if (!revealed[ty][tx]) {          // almost invisible hint for sharp eyes
                    RectangleShape hint(Vector2f(TILE - 6, TILE - 6));
                    hint.setPosition(V(x + 3, y + 3));
                    hint.setFillColor(Color(0, 0, 0, 14));
                    w.draw(hint);
                } else if (c == 'H') {            // spikes
                    for (int i = 0; i < 4; i++) {
                        ConvexShape sp(3);
                        float bx = x + 4 + i * 9;
                        sp.setPoint(0, Vector2f(bx, y + 34));
                        sp.setPoint(1, Vector2f(bx + 8, y + 34));
                        sp.setPoint(2, Vector2f(bx + 4, y + 8));
                        sp.setFillColor(Color(200, 200, 210));
                        sp.setOutlineColor(Color(130, 20, 20));
                        sp.setOutlineThickness(1);
                        w.draw(sp);
                    }
                } else {                          // scorch mark
                    CircleShape burn(16);
                    burn.setOrigin(V(16, 16));
                    burn.setPosition(V(x + 20, y + 20));
                    burn.setFillColor(Color(15, 15, 15, 210));
                    w.draw(burn);
                }
            }
            else if (c == 'F') drawFire(w, x, y, fireOn(tx, ty), fireWarning(tx, ty));
            else if (c == 'D') drawDoor(w, x, y, false);
            else if (c == 'X') drawDoor(w, x, y, revealed[ty][tx]);
            else if (c == 'E') {                  // glowing exit portal
                float pulse = 3 * sin(uiTime * 4);
                CircleShape o(15 + pulse);
                o.setOrigin(V(o.getRadius(), o.getRadius()));
                o.setPosition(V(x + 20, y + 20));
                o.setFillColor(Color(60, 220, 120));
                w.draw(o);
                CircleShape in(8);
                in.setOrigin(V(8, 8));
                in.setPosition(V(x + 20, y + 20));
                in.setFillColor(Color(230, 255, 235));
                w.draw(in);
            }
        }
    }
}

void drawStarsOnMap(RenderWindow& w)
{
    for (int i = 0; i < 3; i++) {
        if (starTaken[i]) continue;
        float bob = 2 * sin(uiTime * 4 + i);
        drawStar(w, starTX[i] * TILE + 20, starTY[i] * TILE + 20 + bob, 11, Color(255, 215, 0));
    }
}

void drawPlayer(RenderWindow& w)
{
    float scale = dying ? max(0.f, 1.f - deathTimer * 0.9f) : 1.f;
    float bob = (moving && !dying) ? sin(uiTime * 18) * 2 : 0;

    CircleShape body(13);
    body.setOrigin(V(13, 13));
    body.setPosition(V(px, py + bob));
    body.setScale(V(scale, scale));
    body.setFillColor(dying ? Color(230, 80, 80) : Color(70, 150, 255));
    body.setOutlineColor(Color(20, 50, 130));
    body.setOutlineThickness(2);
    w.draw(body);

    if (scale <= 0.05f) return;
    for (int e = -1; e <= 1; e += 2) {            // eyes look where the player faces
        CircleShape eye(3.5f * scale);
        eye.setOrigin(V(3.5f * scale, 3.5f * scale));
        eye.setPosition(V(px + e * 5 * scale + faceX * 2 * scale, py - 3 * scale + bob));
        eye.setFillColor(Color::White);
        w.draw(eye);
        CircleShape pupil(1.6f * scale);
        pupil.setOrigin(V(1.6f * scale, 1.6f * scale));
        pupil.setPosition(V(px + e * 5 * scale + faceX * 3 * scale, py - 3 * scale + bob));
        pupil.setFillColor(Color::Black);
        w.draw(pupil);
    }
    if (moving && !dying) {                       // little walking feet
        float s = sin(uiTime * 18) * 4;
        RectangleShape foot(Vector2f(6, 4));
        foot.setFillColor(Color(20, 50, 130));
        foot.setPosition(V(px - 9 + s, py + 11)); w.draw(foot);
        foot.setPosition(V(px + 3 - s, py + 11)); w.draw(foot);
    }
}

void drawDeathEffects(RenderWindow& w)
{
    if (!dying) return;
    float cx = deathTX * TILE + 20, cy = deathTY * TILE + 20;

    if (deathType == 'B') {                       // explosion
        float rad = deathTimer * 90;
        CircleShape boom(rad);
        boom.setOrigin(V(rad, rad));
        boom.setPosition(V(cx, cy));
        int a = (int)max(0.f, 255 * (1.f - deathTimer / 1.2f));
        boom.setFillColor(Color(255, 150, 30, a));
        w.draw(boom);
        CircleShape core(rad * 0.5f);
        core.setOrigin(V(rad * 0.5f, rad * 0.5f));
        core.setPosition(V(cx, cy));
        core.setFillColor(Color(255, 240, 150, a));
        w.draw(core);
    }
    RectangleShape flash(Vector2f(WIN_W, ROWS * TILE));
    flash.setFillColor(Color(200, 0, 0, (int)max(0.f, 110 * (1.f - deathTimer / 1.2f))));
    w.draw(flash);

    drawText(w, deathMsg, 26, WIN_W / 2.f, 260, Color::White, true);
}

void drawHUD(RenderWindow& w)
{
    RectangleShape bar(Vector2f(WIN_W, HUD));
    bar.setPosition(V(0, ROWS * TILE));
    bar.setFillColor(Color(20, 18, 28));
    w.draw(bar);

    float y = ROWS * TILE;
    drawText(w, "Level " + to_string(currentLevel + 1) + ": " + LEVEL_NAMES[currentLevel],
             18, 12, y + 9, Color(230, 230, 240));

    drawText(w, "Lives:", 18, 300, y + 9, Color(230, 230, 240));
    for (int i = 0; i < lives; i++) {
        CircleShape heart(7);
        heart.setFillColor(Color(230, 50, 70));
        heart.setPosition(V(362 + i * 20, y + 13));
        w.draw(heart);
    }

    drawText(w, "Stars:", 18, 450, y + 9, Color(230, 230, 240));
    for (int i = 0; i < 3; i++)
        drawStar(w, 520 + i * 24, y + 21, 9,
                 i < starsThisLevel ? Color(255, 215, 0) : Color(80, 80, 90));

    drawText(w, "P Pause  R Restart  Esc Menu", 14, 600, y + 13, Color(150, 150, 165));
}

void drawOverlay(RenderWindow& w, int alpha = 170)
{
    RectangleShape o(Vector2f(WIN_W, WIN_H));
    o.setFillColor(Color(0, 0, 0, alpha));
    w.draw(o);
}

void drawRating(RenderWindow& w, int stars, float y)
{
    for (int i = 0; i < 3; i++)
        drawStar(w, WIN_W / 2.f - 70 + i * 70, y, 28,
                 i < stars ? Color(255, 215, 0) : Color(80, 80, 90));
}

void drawWorld(RenderWindow& w)
{
    drawTiles(w);
    drawStarsOnMap(w);
    drawPlayer(w);
    drawDeathEffects(w);
    drawHUD(w);
}

// Screens (menu, instructions, pause, complete, ...)

void drawMenu(RenderWindow& w)
{
    w.clear(Color(22, 18, 32));
    // flickering torches on both sides
    for (int s = 0; s < 2; s++) {
        float tx = s == 0 ? 120 : WIN_W - 120;
        float f = 6 * sin(uiTime * 11 + s * 2);
        CircleShape glow(46 + f);
        glow.setOrigin(V(glow.getRadius(), glow.getRadius()));
        glow.setPosition(V(tx, 220));
        glow.setFillColor(Color(255, 140, 30, 45));
        w.draw(glow);
        ConvexShape fl(3);
        fl.setPoint(0, Vector2f(tx - 12, 240));
        fl.setPoint(1, Vector2f(tx + 12, 240));
        fl.setPoint(2, Vector2f(tx, 190 - f));
        fl.setFillColor(Color(255, 150, 30));
        w.draw(fl);
        RectangleShape stick(Vector2f(8, 70));
        stick.setPosition(V(tx - 4, 240));
        stick.setFillColor(Color(90, 60, 30));
        w.draw(stick);
    }
    drawText(w, "MAZE ESCAPE", 72, WIN_W / 2.f, 110, Color(255, 205, 70), true);
    drawText(w, "Choose the right door. Avoid the traps. Escape!", 22,
             WIN_W / 2.f, 205, Color(200, 200, 215), true);

    Color blink = ((int)(uiTime * 2) % 2) ? Color(255, 255, 255) : Color(255, 205, 70);
    drawText(w, "Press ENTER to Start", 34, WIN_W / 2.f, 300, blink, true);
    drawText(w, "Press I for Instructions", 26, WIN_W / 2.f, 360, Color(200, 200, 215), true);
    drawText(w, "Press ESC to Quit", 22, WIN_W / 2.f, 410, Color(150, 150, 165), true);
    drawText(w, "Muhammad Azan Jawad (26L-0736)  |  Faseeh Ahmed Tayyab (26L-0735) ",
             22, WIN_W / 2.f, 560, Color(120, 120, 140), true);
}

void drawInstructions(RenderWindow& w)
{
    w.clear(Color(22, 18, 32));
    drawText(w, "HOW TO PLAY", 48, WIN_W / 2.f, 30, Color(255, 205, 70), true);
    Color c(225, 225, 235);
    float y = 110;
    drawText(w, "Arrow Keys  - move your character", 24, 120, y, c); y += 40;
    drawText(w, "P  - pause / resume       R  - restart level       Esc  - menu", 22, 120, y, c); y += 55;
    drawText(w, "Reach the glowing green EXIT to finish each level.", 22, 120, y, c); y += 34;
    drawText(w, "Doors look the same, but only ONE is safe. A wrong door kills you!", 22, 120, y, c); y += 34;
    drawText(w, "Fire traps burn in cycles - watch the timing before you run.", 22, 120, y, c); y += 34;
    drawText(w, "Hidden spikes and booby traps wait on 'safe' floor. Look closely...", 22, 120, y, c); y += 34;
    drawText(w, "You have 3 lives. Lose them all and the game is over.", 22, 120, y, c); y += 34;
    drawText(w, "Up to 3 stars are hidden in each level - collect them for a better rating!", 22, 120, y, c); y += 34;
    drawText(w, "You can finish a level with 0 stars - they only change your rating.", 22, 120, y, c);
    drawText(w, "Press ENTER or ESC to go back", 24, WIN_W / 2.f, 540, Color(255, 205, 70), true);
}

void drawScreens(RenderWindow& w)
{
    if (state == MENU) { drawMenu(w); return; }
    if (state == INSTRUCTIONS) { drawInstructions(w); return; }

    w.clear(Color(10, 10, 14));
    if (state == VICTORY) {
        w.clear(Color(18, 28, 22));
        drawText(w, "YOU ESCAPED!", 72, WIN_W / 2.f, 90, Color(255, 215, 0), true);
        drawText(w, "You beat all three mazes!", 28, WIN_W / 2.f, 190, Color::White, true);
        int total = levelStars[0] + levelStars[1] + levelStars[2];
        drawText(w, "Total stars: " + to_string(total) + " / 9", 32, WIN_W / 2.f, 260, Color(255, 215, 0), true);
        for (int i = 0; i < 3; i++) {
            drawText(w, "Level " + to_string(i + 1), 22, 130 + i * 230, 340, Color::White);
            for (int s = 0; s < 3; s++)
                drawStar(w, 150 + i * 230 + s * 36, 410, 14,
                         s < levelStars[i] ? Color(255, 215, 0) : Color(80, 80, 90));
        }
        drawText(w, "Press ENTER to return to the menu", 26, WIN_W / 2.f, 520, Color(200, 200, 215), true);
        return;
    }

    drawWorld(w);

    if (state == PAUSED) {
        drawOverlay(w);
        drawText(w, "PAUSED", 64, WIN_W / 2.f, 200, Color::White, true);
        drawText(w, "P - resume    R - restart level    Esc - menu", 24, WIN_W / 2.f, 300, Color(210, 210, 220), true);
    }
    else if (state == LEVEL_COMPLETE) {
        drawOverlay(w);
        drawText(w, "LEVEL " + to_string(currentLevel + 1) + " COMPLETE!", 56, WIN_W / 2.f, 140, Color(120, 255, 160), true);
        drawRating(w, levelStars[currentLevel], 280);
        drawText(w, "Stars collected: " + to_string(levelStars[currentLevel]) + " / 3", 28, WIN_W / 2.f, 330, Color::White, true);
        drawText(w, currentLevel == 2 ? "Press ENTER to see the final screen" : "Press ENTER for the next level",
                 26, WIN_W / 2.f, 420, Color(255, 205, 70), true);
    }
    else if (state == GAME_OVER) {
        drawOverlay(w, 200);
        drawText(w, "GAME OVER", 72, WIN_W / 2.f, 200, Color(230, 60, 60), true);
        drawText(w, "You ran out of lives.", 26, WIN_W / 2.f, 300, Color::White, true);
        drawText(w, "Press ENTER to return to the menu", 26, WIN_W / 2.f, 400, Color(255, 205, 70), true);
    }
}


//   Keyboard events for every screen

void handleKey(Keyboard::Key k, RenderWindow& w)
{
    switch (state) {
    case MENU:
        if (k == Keyboard::Key::Enter) startNewGame();
        else if (k == Keyboard::Key::I) state = INSTRUCTIONS;
        else if (k == Keyboard::Key::Escape) w.close();
        break;
    case INSTRUCTIONS:
        if (k == Keyboard::Key::Enter || k == Keyboard::Key::Escape) state = MENU;
        break;
    case PLAYING:
        if (k == Keyboard::Key::P) state = PAUSED;
        else if (k == Keyboard::Key::R) loadLevel(currentLevel);
        else if (k == Keyboard::Key::Escape) state = MENU;
        break;
    case PAUSED:
        if (k == Keyboard::Key::P) state = PLAYING;
        else if (k == Keyboard::Key::R) { loadLevel(currentLevel); state = PLAYING; }
        else if (k == Keyboard::Key::Escape) state = MENU;
        break;
    case LEVEL_COMPLETE:
        if (k == Keyboard::Key::Enter) {
            if (currentLevel == 2) state = VICTORY;
            else { loadLevel(currentLevel + 1); state = PLAYING; }
        }
        break;
    case GAME_OVER:
    case VICTORY:
        if (k == Keyboard::Key::Enter || k == Keyboard::Key::Escape) state = MENU;
        break;
    }
}

int main()
{
    srand((unsigned)time(0));
    RenderWindow window(VideoMode(Vector2u(WIN_W, WIN_H)), "Maze Escape");
    window.setFramerateLimit(60);
    loadFont();
    if (music.openFromFile("playlistsons-pyramid-311175.ogg")) {
    music.setLooping(true);
    music.setVolume(40);      // 0 to 100
    music.play();
}
    loadLevel(0);

    Clock clock;
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.05f) dt = 0.05f;
        uiTime += dt;

        while (const optional<Event> e = window.pollEvent()) {
            if (e->is<Event::Closed>())
                window.close();
            else if (const auto* key = e->getIf<Event::KeyPressed>())
                handleKey(key->code, window);
        }

        if (state == PLAYING) update(dt);

        drawScreens(window);
        window.display();
    }
    return 0;
}