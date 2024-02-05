#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

SDL_Window* gWindow = nullptr;
SDL_Renderer* gRenderer = nullptr;

const int MAP_WIDTH = 8;
const int MAP_HEIGHT = 8;
int worldMap[MAP_WIDTH][MAP_HEIGHT];

float playerX = 4.5; // Initial player position
float playerY = 4.5;
float playerAngle = 0.0; // Initial player angle
float playerSpeed = 0.1; // Player movement speed
float rotationSpeed = 0.05; // Player rotation speed

bool drawMapEnabled = true;
bool rainEnabled = false; // Flag to control rain

SDL_Texture* wallTexture = nullptr;
SDL_Texture* floorTexture = nullptr;
SDL_Texture* ceilingTexture = nullptr;
SDL_Texture* weaponTexture = nullptr;
SDL_Texture* raindropTexture = nullptr;

struct Raindrop {
	float x;
	float y;
	float speed;
};

std::vector<Raindrop> raindrops;

void initializeSDL() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
		SDL_Quit();
		exit(EXIT_FAILURE);
	}

	gWindow = SDL_CreateWindow("Raycasting Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if (gWindow == nullptr) {
		std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
		SDL_Quit();
		exit(EXIT_FAILURE);
	}

	gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
	if (gRenderer == nullptr) {
		std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
		SDL_DestroyWindow(gWindow);
		SDL_Quit();
		exit(EXIT_FAILURE);
	}

	SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
	SDL_RenderClear(gRenderer);

	int imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags)) {
		std::cerr << "SDL_image initialization failed: " << IMG_GetError() << std::endl;
		SDL_DestroyRenderer(gRenderer);
		SDL_DestroyWindow(gWindow);
		SDL_Quit();
		exit(EXIT_FAILURE);
	}

	wallTexture = loadTexture("wall_texture.png");
	floorTexture = loadTexture("floor_texture.png");
	ceilingTexture = loadTexture("ceiling_texture.png");
	weaponTexture = loadTexture("weapon_texture.png");
	raindropTexture = loadTexture("raindrop_texture.png");
}

void closeSDL() {
	SDL_DestroyTexture(wallTexture);
	SDL_DestroyTexture(floorTexture);
	SDL_DestroyTexture(ceilingTexture);
	SDL_DestroyTexture(weaponTexture);
	SDL_DestroyTexture(raindropTexture);

	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyWindow(gWindow);

	IMG_Quit();
	SDL_Quit();
}

SDL_Texture* loadTexture(const std::string& path) {
	SDL_Texture* texture = nullptr;

	SDL_Surface* loadedSurface = IMG_Load(path.c_str());
	if (loadedSurface == nullptr) {
		std::cerr << "Unable to load image " << path << "! SDL_image Error: " << IMG_GetError() << std::endl;
	} else {
		texture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
		if (texture == nullptr) {
			std::cerr << "Unable to create texture from " << path << "! SDL Error: " << SDL_GetError() << std::endl;
		}

		SDL_FreeSurface(loadedSurface);
	}

	return texture;
}

void renderTexture(SDL_Texture* texture, int x, int y, int width, int height) {
	SDL_Rect destinationRect = {x, y, width, height};
	SDL_RenderCopy(gRenderer, texture, nullptr, &destinationRect);
}

void handleInput(SDL_Event& e) {
	if (e.type == SDL_QUIT) {
		closeSDL();
		exit(EXIT_SUCCESS);
	} else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
		const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);

		// Toggle rain on/off with the 'R' key
		if (currentKeyStates[SDL_SCANCODE_R]) {
			rainEnabled = !rainEnabled;
		}

		// Move forward and backward
		if (currentKeyStates[SDL_SCANCODE_W] && !currentKeyStates[SDL_SCANCODE_S]) {
			float nextX = playerX + playerSpeed * cos(playerAngle);
			float nextY = playerY + playerSpeed * sin(playerAngle);
			if (!isCollision(nextX, nextY)) {
				playerX = nextX;
				playerY = nextY;
			}
		} else if (!currentKeyStates[SDL_SCANCODE_W] && currentKeyStates[SDL_SCANCODE_S]) {
			float nextX = playerX - playerSpeed * cos(playerAngle);
			float nextY = playerY - playerSpeed * sin(playerAngle);
			if (!isCollision(nextX, nextY)) {
				playerX = nextX;
				playerY = nextY;
			}
		}

		// Strafe left and right
		if (currentKeyStates[SDL_SCANCODE_A] && !currentKeyStates[SDL_SCANCODE_D]) {
			float nextX = playerX - playerSpeed * cos(playerAngle + M_PI / 2);
			float nextY = playerY - playerSpeed * sin(playerAngle + M_PI / 2);
			if (!isCollision(nextX, nextY)) {
				playerX = nextX;
				playerY = nextY;
			}
		} else if (!currentKeyStates[SDL_SCANCODE_A] && currentKeyStates[SDL_SCANCODE_D]) {
			float nextX = playerX + playerSpeed * cos(playerAngle + M_PI / 2);
			float nextY = playerY + playerSpeed * sin(playerAngle + M_PI / 2);
			if (!isCollision(nextX, nextY)) {
				playerX = nextX;
				playerY = nextY;
			}
		}

		// Rotate left and right
		if (currentKeyStates[SDL_SCANCODE_LEFT] && !currentKeyStates[SDL_SCANCODE_RIGHT]) {
			playerAngle -= rotationSpeed;
		} else if (!currentKeyStates[SDL_SCANCODE_LEFT] && currentKeyStates[SDL_SCANCODE_RIGHT]) {
			playerAngle += rotationSpeed;
		}
	}
}

bool isCollision(float x, float y) {
	int cellX = static_cast<int>(x);
	int cellY = static_cast<int>(y);

	return worldMap[cellX][cellY] == 1;
}

void generateRaindrops() {
	// Generate a new raindrop at random x position
	float randomX = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * SCREEN_WIDTH;
	Raindrop newRaindrop = {randomX, 0, 5.0}; // Start from the top with speed 5.0
	raindrops.push_back(newRaindrop);
}

void moveRaindrops() {
	for (auto& raindrop : raindrops) {
		raindrop.y += raindrop.speed;
		if (raindrop.y > SCREEN_HEIGHT) {
			// Remove raindrops that are outside the screen
			raindrop = raindrops.back();
			raindrops.pop_back();
		}
	}
}

void drawRaindrops() {
	SDL_SetRenderDrawColor(gRenderer, 0, 0, 255, 255); // Blue color for raindrops

	for (const auto& raindrop : raindrops) {
		renderTexture(raindropTexture, static_cast<int>(raindrop.x), static_cast<int>(raindrop.y), 5, 5);
	}
}

void drawMap() {
	for (int x = 0; x < MAP_WIDTH; ++x) {
		for (int y = 0; y < MAP_HEIGHT; ++y) {
			if (worldMap[x][y] == 1) {
				// Introduce shadows based on player's angle
				float dx = x - playerX;
				float dy = y - playerY;
				float distance = sqrt(dx * dx + dy * dy);
				float dot = (dx * cos(playerAngle) + dy * sin(playerAngle)) / distance;

				if (dot > 0.7) { // Shadow threshold
					renderTexture(wallTexture, x * (SCREEN_WIDTH / MAP_WIDTH), y * (SCREEN_HEIGHT / MAP_HEIGHT), SCREEN_WIDTH / MAP_WIDTH, SCREEN_HEIGHT / MAP_HEIGHT);
				}
			} else {
				renderTexture(floorTexture, x * (SCREEN_WIDTH / MAP_WIDTH), y * (SCREEN_HEIGHT / MAP_HEIGHT), SCREEN_WIDTH / MAP_WIDTH, SCREEN_HEIGHT / MAP_HEIGHT);
				renderTexture(ceilingTexture, x * (SCREEN_WIDTH / MAP_WIDTH), 0, SCREEN_WIDTH / MAP_WIDTH, SCREEN_HEIGHT / (2 * MAP_HEIGHT));
			}
		}
	}
}

void drawPlayerLineOfSight() {
	SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);

	float lineX = playerX * (SCREEN_WIDTH / MAP_WIDTH) + (SCREEN_WIDTH / MAP_WIDTH) / 2;
	float lineY = playerY * (SCREEN_HEIGHT / MAP_HEIGHT) + (SCREEN_HEIGHT / MAP_HEIGHT) / 2;
	float lineLength = 50.0;

	float lineEndX = lineX + lineLength * cos(playerAngle);
	float lineEndY = lineY + lineLength * sin(playerAngle);

	SDL_RenderDrawLine(gRenderer, static_cast<int>(lineX), static_cast<int>(lineY), static_cast<int>(lineEndX), static_cast<int>(lineEndY));
}

void drawWeapon() {
	SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);

	float weaponX = (SCREEN_WIDTH / 2) - 25;
	float weaponY = SCREEN_HEIGHT - 50;

	renderTexture(weaponTexture, static_cast<int>(weaponX), static_cast<int>(weaponY), 50, 50);
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <map_file_path>" << std::endl;
		return EXIT_FAILURE;
	}

	std::string mapFilePath = argv[1];
	if (!loadMapFromFile(mapFilePath)) {
		return EXIT_FAILURE;
	}

	initializeSDL();

	SDL_Event e;
	bool quit = false;

	// Seed for random number generation
	std::srand(static_cast<unsigned>(std::time(0)));

	while (!quit) {
		while (SDL_PollEvent(&e) != 0) {
			handleInput(e);
		}

		SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
		SDL_RenderClear(gRenderer);

		if (drawMapEnabled) {
			drawMap();
		}

		SDL_SetRenderDrawColor(gRenderer, 255, 0, 0, 255);
		SDL_Rect playerRect = {playerX * (SCREEN_WIDTH / MAP_WIDTH), playerY * (SCREEN_HEIGHT / MAP_HEIGHT), 5, 5};
		SDL_RenderFillRect(gRenderer, &playerRect);

		drawPlayerLineOfSight();
		drawWeapon();

		if (rainEnabled) {
			generateRaindrops();
			moveRaindrops();
			drawRaindrops();
		}

		SDL_RenderPresent(gRenderer);
	}

	closeSDL();

	return 0;
}
