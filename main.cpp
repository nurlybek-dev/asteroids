#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <chrono>
#include <math.h>
#include <algorithm>
#include <vector>
#include <random>

#define TICK_INTERVAL 30
#define WIDTH 1024
#define HEIGHT 768
#define PI 3.14159265
#define MAX_BIG_ASTEROIDS 23

static Uint32 next_time;

Uint32 time_left(void)
{
    Uint32 now;

    now = SDL_GetTicks();
    if(next_time <= now)
        return 0;
    else
        return next_time - now;
}

struct Vector2 {
    float x;
    float y;
    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}
};

enum AsteroidType {
    BIG=10,
    MEDIUM=50,
    SMALL=100,
};

bool collide(SDL_Rect a, SDL_Rect b);
void createNewAsteroids(Vector2 Pos, AsteroidType type);
void startGame();
void restartGame();
void startWave();
void clear();
SDL_Texture *loadText(std::string text, SDL_Color color);

SDL_Window *gWindow;
SDL_Renderer *gRenderer;
TTF_Font *gFont;

class Asteroid {
    public:
        Asteroid(SDL_Renderer *renderer, Vector2 pos, AsteroidType type) {
            mRenderer = renderer;
            SDL_Surface *surface = nullptr;
            mType = type;
            switch (type)
            {
            case BIG:
                surface = IMG_Load("assets/PNG/Meteors/meteorBrown_big1.png");
                mRotationSpeed = rand() % 10 * 0.05f;
                mVelocity = Vector2(rand()%10 * 0.1f,  rand()%10 * 0.1f);
                break;
            case MEDIUM:
                surface = IMG_Load("assets/PNG/Meteors/meteorBrown_med1.png");
                mRotationSpeed = rand() % 10 * 0.05f;
                mVelocity = Vector2(rand()%10 * 0.3f,  rand()%10 * 0.3f);
                break;
            case SMALL:
                surface = IMG_Load("assets/PNG/Meteors/meteorBrown_tiny1.png");
                mRotationSpeed = rand() % 10 * 0.1f;
                mVelocity = Vector2(rand()%10 * 0.5f,  rand()%10 * 0.5f);
                break;
            }
            mTexture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_QueryTexture(mTexture, nullptr, nullptr, &mWidth, &mHeight);
            SDL_FreeSurface(surface);

            mPos = pos;
            mAngle = rand() % 360;

            mDestroyed = false;
        }

        ~Asteroid() {
            SDL_DestroyTexture(mTexture);
            mRenderer = nullptr;
        }

        SDL_Rect Rect()
        {
            return {(int)mPos.x, (int)mPos.y, mWidth, mHeight};
        }

        Vector2 Pos() const
        {
            return mPos;
        }

        void Draw() {
            SDL_Rect dstrect = {(int)mPos.x, (int)mPos.y, mWidth, mHeight};
            SDL_RenderCopyEx(mRenderer, mTexture, nullptr, &dstrect, mAngle-90, nullptr, SDL_FLIP_NONE);
        }

        void SetVelocity(Vector2 velocity) {
            mVelocity = velocity;
        }

        void Update() {
            mPos.x += mVelocity.x;
            mPos.y += mVelocity.y;
            mAngle += mRotationSpeed;

            if(mAngle > 360) mAngle -= 360;
            else if(mAngle < 360) mAngle += 360;

            if(mPos.x < -mWidth)
            {
                mPos.x = WIDTH;
            }
            else if(mPos.x > WIDTH)
            {
                mPos.x = -mWidth;
            }

            if(mPos.y < -mHeight)
            {
                mPos.y = HEIGHT;
            }
            else if(mPos.y > HEIGHT)
            {
                mPos.y = -mHeight;
            }
        }

        void Destroy()
        {
            mDestroyed = true;
        }

        bool Destroyed() const
        {
            return mDestroyed;
        }

        AsteroidType Type() const
        {
            return mType;
        }

    private:
        SDL_Renderer *mRenderer;
        SDL_Texture *mTexture;
        Vector2 mPos;
        Vector2 mVelocity;
        float mRotationSpeed;
        float mAngle;

        int mWidth;
        int mHeight;

        bool mDestroyed;

        AsteroidType mType;
};

class Bullet
{
    public:
        Bullet(SDL_Renderer *renderer, Vector2 pos, float angle) {
            mRenderer = renderer;
            mSpeed = 15;
            mAngle = angle;
            mPos = pos;

            mDestroyed = false;

            SDL_Surface *surface = IMG_Load("assets/PNG/laser.png");
            mTexture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_QueryTexture(mTexture, nullptr, nullptr, &mWidth, &mHeight);
            mWidth = mWidth/2;
            mHeight = mHeight/2;
            SDL_FreeSurface(surface);
        }

        ~Bullet() {
            SDL_DestroyTexture(mTexture);
            mRenderer = nullptr;
        }

        void Draw() {
            SDL_Rect dstrect = {(int)mPos.x, (int)mPos.y, mWidth, mHeight};
            // dstrect.x -= (dstrect.w / 2);
	        // dstrect.y -= (dstrect.h / 2);
            SDL_RenderCopyEx(mRenderer, mTexture, nullptr, &dstrect, mAngle-90, nullptr, SDL_FLIP_NONE);
        }

        void Update() {
            Vector2 velocity = Vector2(-1 * cos(mAngle * PI / 180), -1 * sin(mAngle * PI / 180));
            auto m = sqrt(velocity.x*velocity.x + velocity.y*velocity.y);
            velocity.x /= m;
            velocity.y /= m;

            mPos.x += velocity.x * mSpeed;
            mPos.y += velocity.y * mSpeed;
        }

        SDL_Rect Rect()
        {
            return {(int)mPos.x, (int)mPos.y, mWidth, mHeight};
        }

        bool Destroyed() const
        {
            return mDestroyed;
        }

        void Collide(std::vector<Asteroid*> &asteroids)
        {
            size_t i=0;
            bool collided = false;
            for(; i<asteroids.size(); i++)
            {
                if(!asteroids[i]->Destroyed() && collide(Rect(), asteroids[i]->Rect()))
                {
                    collided = true;
                    break;
                }
            }
            if(collided)
            {
                asteroids[i]->Destroy();
                mDestroyed = true;
            }
        }

    private:
        SDL_Renderer *mRenderer;
        Vector2 mPos;
        SDL_Texture *mTexture;

        float mSpeed;

        int mWidth;
        int mHeight;

        float mAngle;
        bool mDestroyed;
};

std::vector<Bullet*> gBullets;
std::vector<Asteroid*> gAsteroids;

SDL_Texture *gBackgroundTexture;
SDL_Texture *gScoreTexture;
SDL_Texture *gLivesTexture;
SDL_Texture* gWaveTexture;
SDL_Texture *gNewWaveTexture;
SDL_Texture *gGameOverTexture;
SDL_Texture *gGameStartTexture;
SDL_Texture *gGameStartingTexture;
SDL_Texture *gRestartTexture;
SDL_Rect gScorePos = {WIDTH/2-8, 20, 16, 16};
SDL_Rect gNewWavePos = {WIDTH/2 - 8*32/2, 200, 8*32, 32};
SDL_Rect gLivesPos = {20, 20, 16, 16};
SDL_Rect gWavePos = {WIDTH-20-16, 20, 16, 16};
SDL_Rect gGameOverPos = {WIDTH/2 - 9*16/2, HEIGHT/2 - 8, 9*16, 16};
SDL_Rect gGameStartPos = {WIDTH/2 - 10*32/2, 200, 10*32, 32};
SDL_Rect gGameStartingPos = {WIDTH/2 - 21*32/2, 300, 21*32, 32};
SDL_Rect gRestartPos = {WIDTH/2 - 23*32/2, 300, 23*32, 32};

float gStartGameTime = 5000.0f;
float gStartGameTimer = 0.0f;

float gNewWaveTime = 3000.0f;
float gNewWaveTimer = 0.0f;

int gLives = 2;
int gScore = 0;
int gWave = 0;

bool gIsGameStarted = false;
bool gIsWaveEnd = true;
bool gGameOver = false;

class Ship
{
    public:
        Ship(SDL_Renderer *renderer) {
            mRenderer = renderer;
            mIsMoving = false;
            mSpeed = 16;
            mRotationSpeed = 4;
            mRotationDir = 0;
            mAngle = 90;
            mXPosDir = true;
            mYPosDir = true;
            mDestroyed = false;

            mRespawnTime = 3000.0f;
            mRespawnTimer = 0.0f;

            mShootTime = 1000.0f;
            mShootTimer = mShootTime;

            SDL_Surface *surface = IMG_Load("assets/PNG/playerShip1_blue.png");
            mTexture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_QueryTexture(mTexture, nullptr, nullptr, &mWidth, &mHeight);
            mWidth = mWidth/2;
            mHeight = mHeight/2;
            SDL_FreeSurface(surface);
            mPos = Vector2(WIDTH/2-mWidth/2, HEIGHT/2-mHeight/2);
            shootPos = Vector2(mWidth/2, 0);
            mMaxVelocity = Vector2(mSpeed, mSpeed);
        }
        ~Ship() {
            SDL_DestroyTexture(mTexture);
            mRenderer = nullptr;
        }

        SDL_Rect Rect()
        {
            return {(int)mPos.x, (int)mPos.y, mWidth, mHeight};
        }

        void Destroy()
        {
            mDestroyed = true;
        }

        bool Destroyed()
        {
            return mDestroyed;
        }

        void Respawn()
        {
            if(mRespawnTimer >= mRespawnTime)
            {
                mDestroyed = false;
                mPos = Vector2(WIDTH/2-mWidth/2, HEIGHT/2-mHeight/2);
                mAngle = 90;
                mIsMoving = false;
                mXPosDir = true;
                mYPosDir = true;
                mRotationDir = 0;
                mRespawnTimer = 0.0f;
                mVelocity = Vector2(0, 0);
                gLives--;
            }
            else
            {
                mRespawnTimer += TICK_INTERVAL;
            }
        }

        void Draw() {
            if(Destroyed()) return;
            SDL_Rect dstrect = {(int)mPos.x, (int)mPos.y, mWidth, mHeight};
            SDL_RenderCopyEx(mRenderer, mTexture, nullptr, &dstrect, mAngle-90, nullptr, SDL_FLIP_NONE);
        }

        void Input(SDL_Event event)
        {
            if(Destroyed()) return;
            if(event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_w:
                    mIsMoving = true;
                    break;
                case SDLK_a:
                    mRotationDir = -mRotationSpeed;
                    break;
                case SDLK_d:
                    mRotationDir = mRotationSpeed;
                    break;
                case SDLK_SPACE:
                    Shoot();
                    break;
                }
            }
            if(event.type == SDL_KEYUP)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_w:
                    mIsMoving = false;
                    break;
                case SDLK_a:
                    mRotationDir = 0;
                    break;
                case SDLK_d:
                    mRotationDir = 0;
                    break;
                }
            }
        }

        void Update()
        {
            if(Destroyed()) return;
            if(mIsMoving)
            {
                mVelocity = Vector2(-1 * cos(mAngle * PI / 180), -1 * sin(mAngle * PI / 180));
                auto m = sqrt(mVelocity.x*mVelocity.x + mVelocity.y*mVelocity.y);
                mVelocity.x /= m;
                mVelocity.y /= m;

                mVelocity.x *= mSpeed * 0.25;
                mVelocity.y *= mSpeed * 0.25;

                if(mVelocity.x > mMaxVelocity.x)
                    mVelocity.x = mMaxVelocity.x;
                
                if(mVelocity.y > mMaxVelocity.y)
                    mVelocity.y = mMaxVelocity.y;

                if(mVelocity.x > 0) mXPosDir = true;
                else mXPosDir = false;

                if(mVelocity.y > 0) mYPosDir = true;
                else mYPosDir = false;
            }
            else
            {
                if(mXPosDir && mVelocity.x > 0) mVelocity.x -= 0.25; 
                else if(mVelocity.x < 0) mVelocity.x += 0.25;
                else mVelocity.x = 0;

                if(mYPosDir && mVelocity.y > 0) mVelocity.y -= 0.25;
                else if(mVelocity.y < 0) mVelocity.y += 0.25;
                else mVelocity.y = 0;
            }

            mPos.x += mVelocity.x;
            mPos.y += mVelocity.y;
            if(mRotationDir) mAngle += mRotationDir;
            if(mAngle > 360) mAngle -= 360;
            else if(mAngle < 360) mAngle += 360;

            if(mPos.x < -mWidth)
            {
                mPos.x = WIDTH;
            }
            else if(mPos.x > WIDTH)
            {
                mPos.x = -mWidth;
            }

            if(mPos.y < -mHeight)
            {
                mPos.y = HEIGHT;
            }
            else if(mPos.y > HEIGHT)
            {
                mPos.y = -mHeight;
            }

            if(mShootTimer < mShootTime)
            {
                mShootTimer += TICK_INTERVAL;
            }
        }

        void Shoot() {
            if(mShootTimer >= mShootTime)
            {
                Vector2 bulletPos = Vector2(-cos(mAngle * PI / 180), -sin(mAngle * PI / 180));
                auto m = sqrt(bulletPos.x*bulletPos.x + bulletPos.y*bulletPos.y);
                bulletPos.x /= m;
                bulletPos.y /= m;

                // bulletPos = Vector2((mPos.x + 45) + mWidth / 2 * bulletPos.x, mPos.y + mHeight / 2 * bulletPos.y);

                bulletPos = Vector2(mPos.x + shootPos.x, mPos.y + shootPos.y);

                Bullet *bullet = new Bullet(mRenderer, bulletPos, mAngle);
                gBullets.push_back(bullet);
                mShootTimer = 0.0f;
            }
        }

    private:
        SDL_Texture *mTexture;
        SDL_Renderer *mRenderer;

        Vector2 mPos;
        Vector2 shootPos;
        Vector2 mVelocity;
        Vector2 mMaxVelocity;
        bool mIsMoving;
        float mSpeed;
        float mRotationSpeed;
        float mRotationDir;
        int mWidth;
        int mHeight;
        float mAngle;

        float mRespawnTime;
        float mRespawnTimer;

        float mShootTime;
        float mShootTimer;

        bool mXPosDir;
        bool mYPosDir;
        bool mDestroyed;
};


Ship *gShip = nullptr;

int main(int, char**)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    TTF_Init();

    gWindow = SDL_CreateWindow("Asteroids", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
    gFont = TTF_OpenFont("assets/Bonus/kenvector_future.ttf", 16);

    SDL_Surface *surface = IMG_Load("assets/Backgrounds/black.png");
    gBackgroundTexture = SDL_CreateTextureFromSurface(gRenderer, surface);
    SDL_FreeSurface(surface);

    gNewWaveTexture = loadText("New Wave", {255, 255, 255, 255});
    gGameOverTexture = loadText("Game Over", {255, 255, 255, 255});
    gGameStartTexture = loadText("Start Game", {255, 255, 255, 255});
    gGameStartingTexture = loadText("Press space to start", {255, 255, 255, 255});
    gRestartTexture = loadText("Press Escape to restart", {255, 255, 255, 255});
    gShip = new Ship(gRenderer);

    bool running = true;
    SDL_Event event;

    while(running)
    {
        next_time = SDL_GetTicks() + TICK_INTERVAL;
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT)
            {
                running = false;
            }
            if(!gIsGameStarted)
            {
                if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
                    startGame();
                }
            }
            else if(gGameOver)
            {
                if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                {
                    restartGame();
                }
            }
            else
            {
                if(!gShip->Destroyed()) {
                    gShip->Input(event);
                }
            }
        }
        
        if(gIsGameStarted && gIsWaveEnd)
        {
            startWave();
        }

        for(auto &bullet : gBullets)
        {
            bullet->Update();
        }
        for(auto &asteroid: gAsteroids)
        {
            asteroid->Update();
        }
        gShip->Update();

        if(!gShip->Destroyed())
        {
            for(auto &asteroid : gAsteroids)
            {
                if(collide(gShip->Rect(), asteroid->Rect()))
                {
                    gShip->Destroy();
                    asteroid->Destroy();
                }
            }
        }

        if(gShip->Destroyed())
        {
            if(gLives <= 0)
            {
                gGameOver = true;
            }
            else
            {
                gShip->Respawn();
            }
        }

        for(auto &bullet : gBullets)
        {
            bullet->Collide(gAsteroids);
        }

        for(auto i=0; i < gBullets.size(); i++)
        {
            if(gBullets[i]->Destroyed())
            {
                delete gBullets[i];
                gBullets[i] = nullptr;
            }
        }

        gBullets.erase(std::remove_if(gBullets.begin(), gBullets.end(), [](const Bullet *bullet) { return bullet == nullptr; }), gBullets.end());

        for(auto &asteroid: gAsteroids)
        {
            if(asteroid->Destroyed())
            {
                gScore += (int)asteroid->Type();
                switch (asteroid->Type())
                {
                    case BIG:
                        createNewAsteroids(asteroid->Pos(), MEDIUM);
                        break;
                    case MEDIUM:
                        createNewAsteroids(asteroid->Pos(), SMALL);
                        break;
                    case SMALL:
                        break;
                }
            }
        }

        for(auto i=0; i < gAsteroids.size(); i++)
        {
            if(gAsteroids[i]->Destroyed())
            {
                delete gAsteroids[i];
                gAsteroids[i] = nullptr;
            }
        }

        gAsteroids.erase(std::remove_if(gAsteroids.begin(), gAsteroids.end(), [](const Asteroid *asteroid) { return asteroid == nullptr; }), gAsteroids.end());

        if(gAsteroids.size() == 0)
        {
            gIsWaveEnd = true;
        }

        SDL_DestroyTexture(gScoreTexture);
        gScoreTexture = nullptr;
        std::string scoreStr = std::to_string(gScore);
        gScoreTexture = loadText(scoreStr, {255, 255, 255, 255});
        gScorePos.w = scoreStr.size() * 16;
        gScorePos.x = WIDTH / 2 - gScorePos.w / 2;

        SDL_DestroyTexture(gLivesTexture);
        gLivesTexture = nullptr;
        std::string livesStr = std::to_string(gLives);
        gLivesTexture = loadText(livesStr, {255, 255, 255, 255});
        gLivesPos.w = livesStr.size() * 16;

        SDL_DestroyTexture(gWaveTexture);
        gWaveTexture = nullptr;
        std::string waveStr = std::to_string(gWave);
        gWaveTexture = loadText(waveStr, {255, 255, 255, 255});
        gWavePos.w = waveStr.size() * 16;
        gWavePos.x = WIDTH-20-gWavePos.w;

        SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 1);
        SDL_RenderClear(gRenderer);

        SDL_RenderCopy(gRenderer, gBackgroundTexture, nullptr, nullptr);

        for(auto &bullet : gBullets)
        {
            bullet->Draw();
        }
        for(auto &asteroid: gAsteroids)
        {
            asteroid->Draw();
        }
        gShip->Draw();

        SDL_RenderCopy(gRenderer, gScoreTexture, nullptr, &gScorePos);
        SDL_RenderCopy(gRenderer, gLivesTexture, nullptr, &gLivesPos);
        SDL_RenderCopy(gRenderer, gWaveTexture, nullptr, &gWavePos);

        if(!gIsGameStarted)
        {
            SDL_RenderCopy(gRenderer, gGameStartingTexture, nullptr, &gGameStartingPos);
            SDL_RenderCopy(gRenderer, gGameStartTexture, nullptr, &gGameStartPos);
        }
        else if(gIsWaveEnd)
        {
            SDL_RenderCopy(gRenderer, gNewWaveTexture, nullptr, &gNewWavePos);
        }

        if(gGameOver)
        {
            SDL_RenderCopy(gRenderer, gGameOverTexture, nullptr, &gGameOverPos);
            SDL_RenderCopy(gRenderer, gRestartTexture, nullptr, &gRestartPos);
        }

        SDL_RenderPresent(gRenderer);
        SDL_Delay(time_left());
        next_time += TICK_INTERVAL;
    }

    clear();

    SDL_DestroyTexture(gBackgroundTexture);
    gBackgroundTexture = nullptr;
    SDL_DestroyTexture(gNewWaveTexture);
    gNewWaveTexture = nullptr;
    SDL_DestroyTexture(gGameOverTexture);
    gGameOverTexture = nullptr;
    SDL_DestroyTexture(gGameStartTexture);
    gGameStartTexture = nullptr;
    SDL_DestroyTexture(gGameStartingTexture);
    gGameStartingTexture = nullptr;
    SDL_DestroyTexture(gRestartTexture);
    gRestartTexture = nullptr;

    TTF_CloseFont(gFont);
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);

    return 0;
}

bool collide(SDL_Rect a, SDL_Rect b)
{

	float aLeft = a.x;
	float aRight = a.x + a.w;
	float aTop = a.y;
	float aBottom = a.y + a.h;

	float bLeft = b.x;
	float bRight = b.x + b.w;
	float bTop = b.y;
	float bBottom = b.y + b.h;

	if (aLeft >= bRight)
	{
		return false;
	}

	if (aRight <= bLeft)
	{
		return false;
	}

	if (aTop >= bBottom)
	{
		return false;
	}

	if (aBottom <= bTop)
	{
		return false;
	}

    return true;
}

void createNewAsteroids(Vector2 pos, AsteroidType type)
{
        Vector2 velocity = Vector2(rand()%10 * 0.3f,  rand()%10 * 0.3f);
        Asteroid *a1 = new Asteroid(gRenderer, pos, type);
        a1->SetVelocity(velocity);
        velocity = Vector2(rand()%10 * -0.3f,  rand()%10 * -0.3f);
        Asteroid *a2 = new Asteroid(gRenderer, pos, type);
        a2->SetVelocity(velocity);
        gAsteroids.push_back(a1);
        gAsteroids.push_back(a2);
}

void startGame()
{
    gIsGameStarted = true;
    startWave();
}

void restartGame()
{
    gGameOver = false;
    gIsGameStarted = false;
    gIsWaveEnd = true;
    gWave = 0;
    gLives = 2;
    gScore = 0;
    gNewWaveTimer = 0.0;
    clear();
    gShip = new Ship(gRenderer);
}

void startWave()
{
    if(gNewWaveTimer >= gNewWaveTime)
    {
        gWave++;
        gIsWaveEnd = false;
        gNewWaveTimer = 0.0f;
        int n = std::min(gWave+2, MAX_BIG_ASTEROIDS);
        int xRange = 50 + 50 + 1;
        int yRange = 50 + 50 + 1;
        for(int i=0; i < n; i++)
        {
            int x = (int)(cos(2 * 3.14 * i / n) * 350 + 0.5) + WIDTH/2 - 50 + rand() % xRange - 50;
            int y = (int)(sin(2 * 3.14 * i / n) * 350 + 0.5) + HEIGHT/2 - 40 + rand() % yRange - 50;
            Vector2 pos = Vector2(+x, +y);
            Asteroid *asteroid = new Asteroid(gRenderer, pos, BIG);
            gAsteroids.push_back(asteroid);
        }
    }
    else
    {
        gNewWaveTimer += TICK_INTERVAL;
    }
}

SDL_Texture *loadText(std::string text, SDL_Color color)
{
    SDL_Texture *newTexture = nullptr;

    SDL_Surface *loadedSurface = TTF_RenderText_Solid(gFont, text.c_str(), color);
    if(loadedSurface == nullptr)
    {
        SDL_Log("Unable to create text %s! SDL_ttf Error: %s\n", text.c_str(), TTF_GetError());
    }
    else
    {
        newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
        if( newTexture == nullptr )
        {
            SDL_Log( "Unable to create text from %s! SDL Error: %s\n", text.c_str(), SDL_GetError() );
        }

        SDL_FreeSurface(loadedSurface);
    }

    return newTexture;
}

void clear()
{
    for(auto i=0; i<gAsteroids.size(); i++)
    {
        delete gAsteroids[i];
        gAsteroids[i] = nullptr;
    }
    gAsteroids.clear();

    for(auto i=0; i<gBullets.size(); i++)
    {
        delete gBullets[i];
        gBullets[i] = nullptr;
    }
    gBullets.clear();

    delete gShip;
    gShip = nullptr;
}