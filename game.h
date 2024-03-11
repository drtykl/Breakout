#ifndef GAME_H
#define GAME_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include"game_level.h"
#include "ball_object.h"
#include"power_up.h"

enum class GameState {
    GAME_ACTIVE,
    GAME_MENU,
    GAME_WIN
};

// Represents the four possible (collision) directions
enum class Direction {
    UP,
    LEFT,
    DOWN,
    RIGHT
};

// Defines a Collision typedef that represents collision data
typedef std::tuple<bool, Direction, glm::vec2> Collision; // <collision?, what direction?, difference vector center - closest point>

// Initial size of the player paddle
const glm::vec2 PLAYER_SIZE(100.0f, 20.0f);
// Initial velocity of the player paddle
const float PLAYER_VELOCITY(500.0f);
// Initial velocity of the Ball
const glm::vec2 INITIAL_BALL_VELOCITY(100.0f, -350.0f);
// Radius of the ball object
const float BALL_RADIUS = 12.5f;
//Initial lives of player
const unsigned int INITIAL_LIVES = 3;

// Game holds all game-related state and functionality.
// Combines all game-related data into a single class for
// easy access to each of the components and manageability.
class Game
{
public:
    // game state
    GameState               State;
    bool                    Keys[1024];
    bool                    KeysProcessed[1024];
    unsigned int            Width, Height;
    std::vector<GameLevel>  Levels;
    unsigned int            Level;
    std::vector<PowerUp>    PowerUps;
    unsigned int            Lives;
    // constructor/destructor
    Game(unsigned int width, unsigned int height);
    ~Game();
    // initialize game state (load all shaders/textures/levels)
    void Init();
    // game loop
    void ProcessInput(float dt);
    void Update(float dt);
    void Render();
    void DoCollisions();
    // reset
    void ResetLevel();
    void ResetPlayer();
    // powerups
    void SpawnPowerUps(GameObject& block);
    void UpdatePowerUps(float dt);
private:
    bool CheckCollision(GameObject& one, GameObject& two);// AABB - AABB collision
    Collision CheckCollision(BallObject& one, GameObject& two);// AABB - Circle collision
    Direction VectorDirection(glm::vec2 target);
    bool ShouldSpawn(unsigned int chance);
    void ActivatePowerUp(PowerUp& powerUp);
    bool IsOtherPowerUpActive(std::string type);
};

#endif