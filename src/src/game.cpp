#include "game.h"

#include <bits/stdc++.h>
#include <time.h>
#include <unistd.h>

#include "game_level.h"
#include "game_object.h"
#include "resource_manager.h"
#include "sprite_renderer.h"
#include "text_renderer.h"

#define FOR(i, a, b) for (int i = (a); i < (b); ++i)

using namespace std;
using namespace chrono;

bool CheckCollision(GameObject &one, GameObject &two);

SpriteRenderer *Renderer;
GameObject *Player;
vector<vector<GameObject *>> Enemies(3);
vector<vector<pair<int, int>>> movements(3);
int gameLevel = 0;
int startTime = 0;
chrono::time_point<std::chrono::system_clock> last;
vector<pair<int, int>> enemCells;
float brickWidth;
Game::Game(unsigned int width, unsigned int height)
    : State(GAME_ACTIVE), Keys(), Width(width), Height(height + 5)
{
}
TextRenderer *Text;

Game::~Game()
{
    // delete Renderer;
    delete Player;
}

int generateLevels(int enemiesRequired, int coinsRequired, int walls, string levelFile)
{
    int lev[20][20];
    int coinsGenerated = 0;
    int wallCount = 0;
    srand(time(0));
    for (int i = 0; i < 20; i++)
        for (int j = 0; j < 20; j++) {
            if (i == 0 || j == 0 || i == 19 || j == 19)
                lev[i][j] = 1;
            else if ((i + j) % 7 == (rand()) % 7 && wallCount <= walls) {
                lev[i][j] = (rand() % 2);
                wallCount++;
            }
            else if ((i + j) % 11 == (rand()) % 7 && coinsGenerated <= coinsRequired) {
                lev[i][j] = 2;
                coinsGenerated++;
            }
            else
                lev[i][j] = 0;
        }

    lev[0][9] = lev[0][10] = lev[0][11] = 0;
    lev[18][9] = lev[18][10] = lev[18][11] = 0;
    lev[19][9] = lev[19][10] = lev[19][11] = 0;
    ofstream lev1out;
    lev1out.open(levelFile);
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 20; j++)
            lev1out << lev[i][j] << " ";
        lev1out << "\n";
    }
    lev1out.close();

    int tempBricks[20][20];
    FOR(i, 0, 20)
    FOR(j, 0, 20)
    tempBricks[i][j] = lev[i][j];
    Enemies[gameLevel].resize(enemiesRequired);
    movements[gameLevel].resize(enemiesRequired);
    for (int i = 0; i < enemiesRequired; i++) {
        int ex = -1, ey = -1;
        while (1) {
            ex = (rand()) % 20;
            ey = (rand()) % 20;
            if (tempBricks[ex][ey] != 0)
                ex = -1;

            if (ex != -1 && ey != -1)
                break;
        }
        tempBricks[ex][ey] = -1;
        enemCells.push_back({ey, ex});
        // Enemies[levNum][i] = new GameObject(glm::vec2(ey * one.brickW, ex * one.brickH), glm::vec2(one.brickH, one.brickH), ResourceManager::GetTexture("enemy"));
    }
    return coinsGenerated;
}

void Game::Init()
{
    Text = new TextRenderer(this->Width, this->Height);
    Text->Load("fonts/OCRAEXT.TTF", 24);
    this->light = 0;
    startTime = time(0);
    this->score = 0;
    this->coinsCollected = 0;
    last = chrono::system_clock::now();
    ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.frag", nullptr, "sprite");
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->Width),
                                      static_cast<float>(this->Height), 0.0f, -1.0f, 1.0f);
    ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
    // set render-specific controls
    Renderer = new SpriteRenderer(ResourceManager::GetShader("sprite"));
    // load textures
    ResourceManager::LoadTexture("textures/background1.jpg", false, "background");
    ResourceManager::LoadTexture("textures/awesomeface.png", true, "face");
    ResourceManager::LoadTexture("textures/block.png", false, "block");
    ResourceManager::LoadTexture("textures/block_solid.png", false, "block_solid");
    ResourceManager::LoadTexture("textures/mask_guy.png", true, "paddle");
    ResourceManager::LoadTexture("textures/virus.png", true, "enemy");
    ResourceManager::LoadTexture("textures/gameOver.jpg", false, "gameOver");
    ResourceManager::LoadTexture("textures/gameWin.png", true, "gameWin");

    glm::vec2 playerPos = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
    Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle"));
    int coins = generateLevels(4, 15, 45, "levels/one.lvl");
    GameLevel one;
    one.Load("levels/one.lvl", this->Width, this->Height + 5);
    this->Levels.push_back(one);
    Enemies[gameLevel].resize(enemCells.size());
    movements[gameLevel].resize(enemCells.size());
    FOR(i, 0, enemCells.size())
    {
        cout << enemCells[i].first << " " << enemCells[i].second << " " << one.brickW << " " << one.brickH << endl;
        Enemies[gameLevel][i] = new GameObject(glm::vec2(enemCells[i].first * one.brickW, enemCells[i].second * one.brickH), glm::vec2(one.brickH, one.brickH), ResourceManager::GetTexture("enemy"));
    }
    brickWidth = one.brickW;
}

int Game::DoCollisions()
{
    bool check = false;
    for (GameObject &box : this->Levels[this->Level].Bricks) {
        if (!box.Destroyed) {
            if (CheckCollision(*Player, box)) {
                if (!box.IsSolid) {
                    box.Destroyed = true;
                    this->score++;
                    this->coinsCollected++;
                    if (this->light)
                        this->score++;
                }
                if (box.IsSolid)
                    check = true;
            }
        }
    }
    return (check ? 1 : 0);
}

int Game::EnemyCollisions(GameObject *object1)
{
    for (GameObject &box : this->Levels[this->Level].Bricks) {
        if (!box.Destroyed) {
            if (CheckCollision(*object1, box)) {
                if (box.IsSolid)
                    return 1;
            }
        }
    }
    return 0;
}

void Game::Update(float dt)
{
    if (this->State == GAME_ACTIVE) {
        this->playerPosition = Player->Position;
        std::chrono::time_point<std::chrono::system_clock> curr = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = curr - last;

        if (elapsed_seconds.count() >= 0.5) {
            FOR(i, 0, Enemies[gameLevel].size())
            movements[gameLevel][i] = {rand() % 2 ? 1 : -1, rand() % 2 ? -1 : 1};
            last = std::chrono::system_clock::now();
        }
        // cout << gameLevel << endl;
        // cout << Enemies[gameLevel].size();
        FOR(i, 0, Enemies[gameLevel].size())
        {
            if (CheckCollision(*Player, *Enemies[gameLevel][i])) {
                // cout << i << endl;
                this->State = GAME_LOST;
            }
            // cout << Enemies[gameLevel][i]->Position.;
        }
        float velocity = PLAYER_VELOCITY * dt;
        FOR(i, 0, Enemies[gameLevel].size())
        {
            velocity = Enemies[gameLevel][i]->Size.x;
            velocity /= 30;
            if (movements[gameLevel][i].first == -1) {
                if (Enemies[gameLevel][i]->Position.x >= velocity) {
                    Enemies[gameLevel][i]->Position.x -= velocity;
                    if (this->EnemyCollisions(Enemies[gameLevel][i]) == 1) {
                        Enemies[gameLevel][i]->Position.x += velocity;
                        movements[gameLevel][i].first = 1;
                    }
                }
                else
                    movements[gameLevel][i].first = 1;
            }
            else {
                if (Enemies[gameLevel][i]->Position.x <= this->Width - velocity) {
                    Enemies[gameLevel][i]->Position.x += velocity;
                    if (this->EnemyCollisions(Enemies[gameLevel][i]) == 1) {
                        Enemies[gameLevel][i]->Position.x -= velocity;
                        movements[gameLevel][i].first = -1;
                    }
                }
                else
                    movements[gameLevel][i].first = -1;
            }
            velocity = Enemies[gameLevel][i]->Size.y;
            velocity /= 30;
            if (movements[gameLevel][i].second == -1) {
                if (Enemies[gameLevel][i]->Position.y + velocity < this->Height) {
                    Enemies[gameLevel][i]->Position.y += velocity;
                    if (this->EnemyCollisions(Enemies[gameLevel][i]) == 1) {
                        Enemies[gameLevel][i]->Position.y -= velocity;
                        movements[gameLevel][i].second = 1;
                    }
                }
                else
                    movements[gameLevel][i].second = 1;
            }
            else {
                if (Enemies[gameLevel][i]->Position.y - velocity >= 0) {
                    Enemies[gameLevel][i]->Position.y -= velocity;
                    if (this->EnemyCollisions(Enemies[gameLevel][i]) == 1) {
                        Enemies[gameLevel][i]->Position.y += velocity;
                        movements[gameLevel][i].second = -1;
                    }
                }
                else
                    movements[gameLevel][i].second = -1;
            }
        }
    }
}

void Game::ProcessInput(float dt)
{
    if (this->State == GAME_ACTIVE) {
        if (Player->Position.y <= (Player->Size.y) / 2) {
            if (Player->Position.x >= 9 * brickWidth && Player->Position.x <= 12 * brickWidth) {
                if (gameLevel == 0) {
                    enemCells.clear();
                    int coins = generateLevels(6, 20, 45, "levels/two.lvl");
                    GameLevel two;
                    two.Load("levels/two.lvl", this->Width, this->Height + 5);
                    this->Levels.push_back(two);
                    gameLevel++;
                    Enemies[gameLevel].resize(enemCells.size());
                    movements[gameLevel].resize(enemCells.size());
                    FOR(i, 0, enemCells.size())
                    {
                        // cout << enemCells[i].first << " " << enemCells[i].second << " " << one.brickW << " " << one.brickH << endl;
                        Enemies[gameLevel][i] = new GameObject(glm::vec2(enemCells[i].first * two.brickW, enemCells[i].second * two.brickH), glm::vec2(two.brickH, two.brickH), ResourceManager::GetTexture("enemy"));
                    }
                    // exit(0);
                    glm::vec2 playerPos = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
                    Player->Position = playerPos;
                    brickWidth = two.brickW;
                    this->Level++;
                }
                else if (gameLevel == 1) {
                    enemCells.clear();
                    int coins = generateLevels(8, 25, 45, "levels/three.lvl");
                    GameLevel three;
                    three.Load("levels/three.lvl", this->Width, this->Height + 5);
                    this->Levels.push_back(three);
                    gameLevel++;
                    Enemies[gameLevel].resize(enemCells.size());
                    movements[gameLevel].resize(enemCells.size());
                    FOR(i, 0, enemCells.size())
                    {
                        // cout << enemCells[i].first << " " << enemCells[i].second << " " << one.brickW << " " << one.brickH << endl;
                        Enemies[gameLevel][i] = new GameObject(glm::vec2(enemCells[i].first * three.brickW, enemCells[i].second * three.brickH), glm::vec2(three.brickH, three.brickH), ResourceManager::GetTexture("enemy"));
                    }
                    // exit(0);
                    glm::vec2 playerPos = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
                    Player->Position = playerPos;
                    brickWidth = three.brickW;
                    this->Level++;
                }
                else {
                    this->State = GAME_WIN;
                }
            }
        }

        float velocity = PLAYER_VELOCITY * dt;
        velocity /= 3;
        if (this->Keys[GLFW_KEY_LEFT]) {
            if (Player->Position.x >= 0.0f) {
                Player->Position.x -= velocity;
                if (this->DoCollisions() == 1)
                    Player->Position.x += velocity;
            }
        }
        if (this->Keys[GLFW_KEY_RIGHT]) {
            if (Player->Position.x <= this->Width - Player->Size.x)
                Player->Position.x += velocity;
            if (this->DoCollisions() == 1)
                Player->Position.x -= velocity;
        }
        if (this->Keys[GLFW_KEY_DOWN]) {
            if (Player->Position.y <= this->Height - Player->Size.y)
                Player->Position.y += velocity;
            if (this->DoCollisions() == 1)
                Player->Position.y -= velocity;
        }
        if (this->Keys[GLFW_KEY_UP]) {
            if (Player->Position.y >= 0)
                Player->Position.y -= velocity;
            if (this->DoCollisions() == 1)
                Player->Position.y += velocity;
        }
    }
}

void Game::Render()
{
    if (this->State == GAME_ACTIVE) {
        // draw background
        Renderer->DrawSprite(ResourceManager::GetTexture("background"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f, glm::vec3(1.0f), playerPosition, light);
        // draw level

        this->Levels[this->Level].Draw(*Renderer, playerPosition, light);
        // draw player
        Player->Draw(*Renderer, playerPosition, light);

        // draw ene6mies
        FOR(i, 0, Enemies[gameLevel].size())
        {
            Enemies[gameLevel][i]->Draw(*Renderer, playerPosition, light);
        }
        int timeElapsed = time(0) - startTime;
        string finalDisplay = "Score:" + to_string(this->score);
        Text->RenderText(finalDisplay, 0.0f, 0.0f, 1.0f);
        finalDisplay = "Coins Collected:" + to_string(this->coinsCollected);
        Text->RenderText(finalDisplay, 0.0f, 60.0f, 1.0f);
        finalDisplay = "Time:" + to_string(timeElapsed);
        Text->RenderText(finalDisplay, 0.0f, 120.0f, 1.0f);
    }
    if (this->State == GAME_LOST) {
        Renderer->DrawSprite(ResourceManager::GetTexture("gameOver"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f, glm::vec3(1.0f), playerPosition, 0);
    }
    if (this->State == GAME_WIN) {
        Renderer->DrawSprite(ResourceManager::GetTexture("gameWin"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f, glm::vec3(1.0f), playerPosition, 0);
    }
}
bool CheckCollision(GameObject &one, GameObject &two)  // AABB - AABB collision
{
    // collision x-axis?
    bool collisionX = one.Position.x + one.Size.x >= two.Position.x &&
                      two.Position.x + two.Size.x >= one.Position.x;
    // collision y-axis?
    bool collisionY = one.Position.y + one.Size.y >= two.Position.y &&
                      two.Position.y + two.Size.y >= one.Position.y;
    // collision only if on both axes
    return collisionX && collisionY;
}