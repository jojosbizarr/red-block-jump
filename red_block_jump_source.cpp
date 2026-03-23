// Simple Platformer — SFML 3.x
// Collect all 9 coins to win. Touch the red floor and you die.
// Controls: A = left | D = right | W = jump | R = restart | Esc = quit

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

// ── Constants ────────────────────────────────────────────────────────────────
// Constant values of game aspects, such as gravity multiplier, jump distance, movement speed, player size, window dimensions, etc.
const float    GRAVITY = 1500.f;
const float    JUMP_FORCE = -600.f;
const float    MOVE_SPEED = 300.f;
const float    PLAYER_W = 40.f;
const float    PLAYER_H = 50.f;
const unsigned WIN_W = 1024;
const unsigned WIN_H = 600;
const float    MAX_DT = 0.05f;
const float    FLOOR_Y = 549.f;  // y threshold for the deadly floor

enum class GameState { Playing, Dead, Won };

// ── Collision helper ──────────────────────────────────────────────────────────
// Checks whether two rectangles are overlapping.
// Parameters: a, b — the two rectangles to test
// Returns: true if they overlap, false if not
bool overlaps(sf::FloatRect a, sf::FloatRect b) {
    if (a.findIntersection(b)) {
        return true;
    }
    return false;
}

// ── Platform ──────────────────────────────────────────────────────────────────
// A class that represents each static solid rectangle the player can stand on or collide with.
// Stores both a shape and a collision object.
// Dimensions are set at creation since platforms never move.
// Methods: Platform - creates each platform based on the set aspect values
struct Platform {
    sf::RectangleShape shape;
    sf::FloatRect      bounds;

    // Creates the platforms based on different aspects of a predetermined platform: position, size, color, outline color, and outline thickness.
    // Parameters: x coord, y coord, width, height, color, outline color, and outline thickness
    // Returns: none
    Platform(float x, float y, float w, float h,
        sf::Color fill = sf::Color(80, 160, 80),
        sf::Color edge = sf::Color(40, 100, 40))
        : bounds(sf::Vector2f(x, y), sf::Vector2f(w, h))
    {
        shape.setPosition({ x, y });
        shape.setSize({ w, h });
        shape.setFillColor(fill);
        shape.setOutlineColor(edge);
        shape.setOutlineThickness(2.f);
    }
};

// ── Coin ──────────────────────────────────────────────────────────────────────
// A collectible gold object. Tracks whether it has been picked up or not.
// Once all 9 have been collected, triggers win condition.
// Dimensions are set at construction since coins never move.
// Methods: Coin - creates each coin based on the set position and color of the coins.
struct Coin {
    sf::CircleShape shape;
    sf::FloatRect   bounds;
    bool            collected = false;

    // Creates the coins based on given aspects.
    // Parameters: x coord of coin and y coord of coin
    // Returns: none
    Coin(float x, float y)
        : bounds(sf::Vector2f(x, y), sf::Vector2f(24.f, 24.f))
    {
        shape.setRadius(12.f);
        shape.setFillColor(sf::Color(255, 215, 0));
        shape.setOutlineColor(sf::Color(200, 160, 0));
        shape.setOutlineThickness(2.f);
        shape.setPosition({ x, y });
    }
};

// ── Player ────────────────────────────────────────────────────────────────────
// The player character. Handles movement, gravity, jumping, and collision.
// Uses axis-separated AABB physics: move X then resolve X, move Y then resolve Y.
// Separating axes prevents corner-catching when moving diagonally into a platform.
// touchedFloor is set to true for one frame when the player lands on the deadly floor.
// Methods: Player - creates the player object based on the preset aspects; update - updates the player object by reading keyboard input, applying gravity, moving the player, and resolving collisions; jump - moves the player object upward based on the jump force value; resolveX - resolves collisions in the x axis between the player and platforms; resolveY - resolves collisions in the y axis between the player and platforms;
struct Player {
    sf::RectangleShape shape;
    sf::Vector2f       velocity{ 0.f, 0.f };
    bool               onGround = false;
    bool               touchedFloor = false;
    int                score = 0;

    // Sets up the player's appearance and spawns them in the air above the first platform.
    // Parameters: none
    // Returns: none
    Player() {
        shape.setSize({ PLAYER_W, PLAYER_H });
        shape.setFillColor(sf::Color(220, 80, 60));
        shape.setOutlineColor(sf::Color(160, 40, 20));
        shape.setOutlineThickness(2.f);
        shape.setPosition({ 175.f, 230.f });
    }

    // Reads keyboard input, applies gravity, moves the player, and resolves collisions.
    // Parameters: deltaTime — seconds since the last frame and platforms in the window
    // Returns: none
    void update(float deltaTime, const std::vector<Platform>& platforms) {
        velocity.x = 0.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
            velocity.x -= MOVE_SPEED;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
            velocity.x += MOVE_SPEED;

        velocity.y += GRAVITY * deltaTime;

        // Resolves x axis movement in each frame update.
        if (velocity.x != 0.f) {
            shape.move({ velocity.x * deltaTime, 0.f });
            resolveX(platforms);
        }

        // Y axis movement is constant because of gravity, it will only = 0 when the player is touching a platform.
        // This means that y axis doesn't need frame by frame calculations.
        shape.move({ 0.f, velocity.y * deltaTime });
        onGround = false;
        touchedFloor = false;
        resolveY(platforms);

        // Fell off the bottom of the screen — treat as floor contact - triggers loss condition
        if (shape.getPosition().y > static_cast<float>(WIN_H) + 100.f) {
            touchedFloor = true;
            velocity = { 0.f, 0.f };
        }

        if (shape.getPosition().x < 0.f) {
            shape.setPosition({ 0.f, shape.getPosition().y });
        }
    }

    // Launches the player upward if they are standing on a surface by changing the velocity in the y axis.
    // Parameters: none
    // Returns: none
    void jump() {
        if (onGround) {
            velocity.y = JUMP_FORCE;
            onGround = false;
        }
    }

private:

    // Creates the collisions with the x axis of objects in the window.
    // Stops horizontal velocity on contact.
    // Parameters: platforms — the list of platforms to check against
    // Returns: none
    void resolveX(const std::vector<Platform>& platforms) {
        sf::Vector2f  pos = shape.getPosition();
        sf::FloatRect playerBounds = sf::FloatRect(pos, { PLAYER_W, PLAYER_H });

        for (const auto& p : platforms) {
            if (overlaps(playerBounds, p.bounds)) {
                if (velocity.x > 0.f) {
                    pos.x = p.bounds.position.x - PLAYER_W;
                }
                else {
                    pos.x = p.bounds.position.x + p.bounds.size.x;
                }
                shape.setPosition(pos);
                playerBounds.position.x = pos.x;
                velocity.x = 0.f;
            }
        }
    }

    // Creates the collisions with the y axis of objects in the window.
    // Stops vertical velocity on contact.
    // Sets the values of onGround bool based on whether the user is on a platform or not.
    // Sets touchedFloor when that platform is the deadly floor, which triggers the loss condition.
    // Parameters: platforms — the list of platforms to check against
    // Returns: none
    void resolveY(const std::vector<Platform>& platforms) {
        sf::Vector2f  pos = shape.getPosition();
        sf::FloatRect playerBounds = sf::FloatRect(pos, { PLAYER_W, PLAYER_H });

        for (const auto& p : platforms) {
            if (overlaps(playerBounds, p.bounds)) {
                if (velocity.y > 0.f) {
                    pos.y = p.bounds.position.y - PLAYER_H;
                    onGround = true;
                    if (p.bounds.position.y >= FLOOR_Y) {
                        touchedFloor = true;
                    }
                }
                else {
                    pos.y = p.bounds.position.y + p.bounds.size.y;
                }
                shape.setPosition(pos);
                playerBounds.position.y = pos.y;
                velocity.y = 0.f;
            }
        }
    }
};

// ── Level data ────────────────────────────────────────────────────────────────

// Builds and returns all platforms in the window. The first platform is the red floor. Places every object into a vector with their aspects stored.
// Platforms are positioned with the max jump height in mind, so that the user can always move to a new platform, so every coin is reachable.
// Parameters: none
// Returns: a list of Platform objects
std::vector<Platform> buildPlatforms() {
    return {
        Platform(0, 550, 1024, 50, sf::Color(140, 30, 30), sf::Color(80, 10, 10)), // deadly floor
        Platform(120, 300,  160, 18, sf::Color(100, 180, 100), sf::Color(60, 120, 60)), // spawn platform (no coin)
        Platform(50, 460,  120, 18),  // left step 1
        Platform(50, 350,  120, 18),  // left step 2
        Platform(260, 500,   90, 18),
        Platform(150, 420,  180, 18),
        Platform(380, 340,  160, 18),
        Platform(580, 260,  140, 18),
        Platform(760, 370,  160, 18),
        Platform(500, 460,  120, 18),
        Platform(880, 220,  140, 18),
    };
}

// Builds and returns all 9 coins in the level, placed just above each floating platform.
// Parameters: none
// Returns: a list of Coin objects
std::vector<Coin> buildCoins() {
    return {
        Coin(80, 425), Coin(80, 315),
        Coin(200, 385), Coin(440, 305),
        Coin(630, 225), Coin(810, 335),
        Coin(540, 425), Coin(920, 185),
        Coin(300, 465),
    };
}

// ── Win screen ────────────────────────────────────────────────────────────────
// Draws a full-screen display when the player collects all coins.
// Shows the final coin count, elapsed time, and a restart prompt.
// Parameters: window — the game window to draw into, font — the loaded font for text, score — coins the player collected, total — total coins in the level, elapsed time — how many seconds the run took
// Returns: none
void drawWinScreen(sf::RenderWindow& window, sf::Font& font,
    int score, int total, float elapsed)
{
    const float centerX = WIN_W / 2.f;
    const float centerY = WIN_H / 2.f;

    sf::RectangleShape overlay({ (float)WIN_W, (float)WIN_H });
    overlay.setFillColor(sf::Color(10, 10, 30, 220));
    window.draw(overlay);

    sf::Text heading(font, "YOU WIN!", 72);
    heading.setFillColor(sf::Color(255, 230, 50));
    heading.setOutlineColor(sf::Color(100, 60, 0));
    heading.setOutlineThickness(4.f);
    heading.setPosition({ centerX - heading.getLocalBounds().size.x / 2.f,
                          centerY - heading.getLocalBounds().size.y / 2.f - 80.f });
    window.draw(heading);

    sf::Text scoreTxt(font, "Coins: " + std::to_string(score) + " / " + std::to_string(total), 28);
    scoreTxt.setFillColor(sf::Color(200, 255, 200));
    scoreTxt.setOutlineColor(sf::Color::Black);
    scoreTxt.setOutlineThickness(2.f);
    scoreTxt.setPosition({ centerX - scoreTxt.getLocalBounds().size.x / 2.f, centerY });
    window.draw(scoreTxt);

    const int mins = static_cast<int>(elapsed) / 60;
    const int secs = static_cast<int>(elapsed) % 60;
    std::string zeroPad = "";
    if (secs < 10) {
        zeroPad = "0";
    }
    const std::string timeStr = "Time: " + std::to_string(mins) + ":" + zeroPad + std::to_string(secs);
    sf::Text timeTxt(font, timeStr, 28);
    timeTxt.setFillColor(sf::Color(200, 220, 255));
    timeTxt.setOutlineColor(sf::Color::Black);
    timeTxt.setOutlineThickness(2.f);
    timeTxt.setPosition({ centerX - timeTxt.getLocalBounds().size.x / 2.f, centerY + 45.f });
    window.draw(timeTxt);

    sf::Text prompt(font, "Press R to play again   |   Esc to quit", 22);
    prompt.setFillColor(sf::Color(170, 170, 170));
    prompt.setPosition({ centerX - prompt.getLocalBounds().size.x / 2.f, centerY + 100.f });
    window.draw(prompt);
}

// ── Death screen ──────────────────────────────────────────────────────────────
// Draws a full-screen display when the player touches the deadly floor.
// Shows how many coins were collected before dying and a restart prompt.
// Parameters: window — the game window to draw into, font — the loaded font for text, score — coins collected before dying, total — total coins in the level
// Returns: none
void drawDeathScreen(sf::RenderWindow& window, sf::Font& font,
    int score, int total)
{
    const float centerX = WIN_W / 2.f;
    const float centerY = WIN_H / 2.f;

    sf::RectangleShape overlay({ (float)WIN_W, (float)WIN_H });
    overlay.setFillColor(sf::Color(40, 0, 0, 210));
    window.draw(overlay);

    sf::Text heading(font, "YOU DIED", 72);
    heading.setFillColor(sf::Color(220, 40, 40));
    heading.setOutlineColor(sf::Color(60, 0, 0));
    heading.setOutlineThickness(5.f);
    heading.setPosition({ centerX - heading.getLocalBounds().size.x / 2.f,
                          centerY - heading.getLocalBounds().size.y / 2.f - 80.f });
    window.draw(heading);

    sf::Text subTxt(font, "Coins collected: " + std::to_string(score) +
        " / " + std::to_string(total), 28);
    subTxt.setFillColor(sf::Color(255, 180, 180));
    subTxt.setOutlineColor(sf::Color::Black);
    subTxt.setOutlineThickness(2.f);
    subTxt.setPosition({ centerX - subTxt.getLocalBounds().size.x / 2.f, centerY });
    window.draw(subTxt);

    sf::Text prompt(font, "Press R to try again   |   Esc to quit", 22);
    prompt.setFillColor(sf::Color(170, 130, 130));
    prompt.setPosition({ centerX - prompt.getLocalBounds().size.x / 2.f, centerY + 50.f });
    window.draw(prompt);
}

// ── Main ──────────────────────────────────────────────────────────────────────
int main() {
    // Creates the game window.
    sf::RenderWindow window(
        sf::VideoMode({ WIN_W, WIN_H }),
        "SFML Platformer",
        sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    // Background Sky Gradient
    sf::VertexArray bg(sf::PrimitiveType::TriangleFan, 4);
    bg[0] = { {0.f,          0.f},          sf::Color(20,  20,  60) };
    bg[1] = { {(float)WIN_W, 0.f},          sf::Color(20,  20,  60) };
    bg[2] = { {(float)WIN_W, (float)WIN_H}, sf::Color(70, 120, 170) };
    bg[3] = { {0.f,          (float)WIN_H}, sf::Color(70, 120, 170) };

    // Loads windows Arial font for the text in headers and windows.
    sf::Font font;
    bool fontLoaded = font.openFromFile("C:/Windows/Fonts/arial.ttf");
    
    // Error check to confirm that the font loaded correctly.
    if (!fontLoaded) {
        return -1;  // exit the program with an error code
    }

    // Sets text color and other aspects of the hud that tracks player score.
    sf::Text hud(font, "", 20);
    hud.setFillColor(sf::Color::White);
    hud.setOutlineColor(sf::Color::Black);
    hud.setOutlineThickness(2.f);
    hud.setPosition({ 12.f, 8.f });
    int lastHudScore = -1;

    // Saves each platform as a value of a vector called platforms.
    const std::vector<Platform> platforms = buildPlatforms();
    const int TOTAL = 9;

    // Saves each coin as a value of a vector called coins. Resets once game is restarted.
    std::vector<Coin> coins = buildCoins();
    
    // Saves the player as a struct object called player.
    Player    player;

    // Starts the game and begins the clock.
    GameState state = GameState::Playing;
    float     playTime = 0.f;

    // Resets all game state for a fresh run.
    // This is a lambda which is basically a function that runs in line. Useful for game while loops and other things.
    // Parameters: all variables set previously in main
    // Returns: none
    auto resetGame = [&]() {
        player = Player();
        coins = buildCoins();
        state = GameState::Playing;
        playTime = 0.f;
        lastHudScore = -1;
        };

    sf::Clock clock;

    // While loop that runs each frame of the game.
    // deltaTime is the time it takes for each frame to generate, so if the FPS is 60, then deltaTime = 1 second / 60 fps.
    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        if (deltaTime > MAX_DT) deltaTime = MAX_DT;

        // Checks for events, namely key press events depending on the game state. Movement key presses work while the game is being played, and once a win or lose screen appear, then the key press events checked are r for restart and esc to leave the program.
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();

            if (const auto* keyPress = event->getIf<sf::Event::KeyPressed>()) {
                using Key = sf::Keyboard::Key;
                if (keyPress->code == Key::Escape) window.close();

                if (state == GameState::Playing) {
                    if (keyPress->code == Key::W)
                        player.jump();
                }
                if (keyPress->code == Key::R &&
                    (state == GameState::Dead || state == GameState::Won))
                    resetGame();
            }
        }

        // Updates the game for each frame. Checks for win and loss conditions (collecting all 9 coins or touching the floor) and tracks the time.
        if (state == GameState::Playing) {
            playTime += deltaTime;
            player.update(deltaTime, platforms);

            if (player.touchedFloor) {
                state = GameState::Dead;
            }
            else {
                const sf::FloatRect playerBounds = sf::FloatRect(
                    player.shape.getPosition(), { PLAYER_W, PLAYER_H });
                for (auto& c : coins) {
                    if (!c.collected && overlaps(playerBounds, c.bounds)) {
                        c.collected = true;
                        ++player.score;
                    }
                }
                if (player.score >= TOTAL)
                    state = GameState::Won;
            }
        }

        // Rebuilds the HUD when score changes to display the new player score.
        if (state == GameState::Playing && player.score != lastHudScore) {
            hud.setString("Coins: " + std::to_string(player.score) + " / " +
                std::to_string(TOTAL) + "   |   A/D: move   W: jump");
            lastHudScore = player.score;
        }

        // Draws every object each frame. Recreates the window and checks the game state to initiate win or loss screens.
        window.clear(sf::Color(10, 10, 20));
        window.draw(bg);
        for (const auto& p : platforms) window.draw(p.shape);
        for (const auto& c : coins)     if (!c.collected) window.draw(c.shape);
        window.draw(player.shape);

        if (state == GameState::Playing)
            window.draw(hud);

        if (state == GameState::Won)
            drawWinScreen(window, font, player.score, TOTAL, playTime);
        else if (state == GameState::Dead)
            drawDeathScreen(window, font, player.score, TOTAL);

        window.display();
    }

    return 0;
}