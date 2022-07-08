#ifndef GAME_H
#define GAME_H

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "game_level.h"

enum GameState {
    GAME_ACTIVE,
    GAME_MENU,
    GAME_WIN,
    GAME_LOST
};
// Initial size of the player paddle
const glm::vec2 PLAYER_SIZE(50.0f, 50.0f);
// Initial velocity of the player paddle
const float PLAYER_VELOCITY(500.0f);

class Game {
   public:
    int score;
    int coinsCollected;
    GameState State;
    bool Keys[1024];
    unsigned int Width, Height;
    std::vector<GameLevel> Levels;
    unsigned int Level;
    Game(unsigned int width, unsigned int height);
    int light;
    glm::vec2 playerPosition;
    ~Game();

    void Init();
    void ProcessInput(float dt);
    void Update(float dt);
    void Render();
    int DoCollisions();
    int EnemyCollisions(GameObject *object1);
};

#endif