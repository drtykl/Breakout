#include <algorithm>
#include <sstream>
#include <iostream>

#include "game.h"
#include "resource_manager.h"
#include"sprite_renderer.h"
#include"mytest.h"
#include"particle_generator.h"
#include"post_processer.h"
#include"text_render.h"

#include<irrKlang.h>
using namespace irrklang;

SpriteRenderer* sprite_render;
GameObject* player;
BallObject* ball;
ParticleGenerator* particle_generator;
PostProcessor* effects;
TextRenderer* text_renderer;
ISoundEngine* SoundEngine = createIrrKlangDevice();

//Mytest* mytest;

float ShakeTime = 0.0f;

Game::Game(unsigned int width, unsigned int height)
	: State(GameState::GAME_MENU), Keys(), Width(width), Height(height), Level(0), Lives(INITIAL_LIVES)
{

}

Game::~Game()
{
	delete sprite_render;
	delete player;
	delete ball;
	delete particle_generator;
	delete effects;
	delete text_renderer;
	SoundEngine->drop();
}

void Game::Init()
{
	////mytest
	//ResourceManager::LoadShader("shaders/vertex.vs", "shaders/fragment.fs", nullptr, "shader");
	//mytest = new Mytest(ResourceManager::GetShader("shader"));

	//Load shaders
	ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.fs", nullptr, "sprite");
	ResourceManager::LoadShader("shaders/particle.vs", "shaders/particle.fs", nullptr, "particle");
	ResourceManager::LoadShader("shaders/post_processing.vs", "shaders/post_processing.fs", nullptr, "postprocessing");
	ResourceManager::LoadShader("shaders/text_2d.vs", "shaders/text_2d.fs", nullptr, "text");

	//config shaders
	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->Width), static_cast<float>(this->Height), 0.0f, -1.0f, 1.0f);
	ResourceManager::GetShader("sprite").SetInteger("sprite", 0, true);
	ResourceManager::GetShader("sprite").SetMatrix4("projection", projection, true);
	ResourceManager::GetShader("particle").SetInteger("sprite", 0, true);
	ResourceManager::GetShader("particle").SetMatrix4("projection", projection, true);

	//Load textures
	ResourceManager::LoadTexture("textures/background.jpg", false, "background");
	ResourceManager::LoadTexture("textures/awesomeface.png", true, "face");
	ResourceManager::LoadTexture("textures/block.png", false, "block");
	ResourceManager::LoadTexture("textures/block_solid.png", false, "block_solid");
	ResourceManager::LoadTexture("textures/paddle.png", true, "paddle");
	ResourceManager::LoadTexture("textures/particle.png", true, "particle");
	ResourceManager::LoadTexture("textures/powerup_speed.png", true, "powerup_speed");
	ResourceManager::LoadTexture("textures/powerup_sticky.png", true, "powerup_sticky");
	ResourceManager::LoadTexture("textures/powerup_increase.png", true, "powerup_increase");
	ResourceManager::LoadTexture("textures/powerup_confuse.png", true, "powerup_confuse");
	ResourceManager::LoadTexture("textures/powerup_chaos.png", true, "powerup_chaos");
	ResourceManager::LoadTexture("textures/powerup_passthrough.png", true, "powerup_passthrough");
	ResourceManager::LoadTexture("textures/powerup_treat.png", true, "powerup_treat");
	ResourceManager::LoadTexture("textures/powerup_remove.png", true, "powerup_remove");

	//config renders
	sprite_render = new SpriteRenderer(ResourceManager::GetShader("sprite"));
	particle_generator = new ParticleGenerator(ResourceManager::GetShader("particle"), ResourceManager::GetTexture("particle"), 500);
	effects = new PostProcessor(ResourceManager::GetShader("postprocessing"), this->Width, this->Height);
	text_renderer = new TextRenderer(ResourceManager::GetShader("text"), this->Width, this->Height);
	text_renderer->Load("fonts/arial.ttf", 24);

	// load levels
	GameLevel one;
	one.Load("levels/one.txt", this->Width, this->Height / 2);
	GameLevel two;
	two.Load("levels/two.txt", this->Width, this->Height / 2);
	GameLevel three;
	three.Load("levels/three.txt", this->Width, this->Height / 2);
	GameLevel four;
	four.Load("levels/four.txt", this->Width, this->Height / 2);
	this->Levels.push_back(one);
	this->Levels.push_back(two);
	this->Levels.push_back(three);
	this->Levels.push_back(four);
	this->Level = 0;

	//GameObject(pos, size, texture, color, velocity)
	// configure game objects
	glm::vec2 playerPos = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
	player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle"));
	glm::vec2 ballPos = playerPos + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS, -BALL_RADIUS * 2.0f);
	ball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY, ResourceManager::GetTexture("face"));

	// audio
	SoundEngine->play2D("audio/breakout.mp3", true);
}

void Game::Update(float dt)
{
	// update objects
	ball->Move(dt, this->Width);
	// check for collisions
	this->DoCollisions();
	// update particles
	particle_generator->Update(dt, *ball, 2, glm::vec2(ball->Radius / 2.0f));
	// update PowerUps
	this->UpdatePowerUps(dt);
	// reduce shake time
	if (ShakeTime > 0.0f)
	{
		ShakeTime -= dt;
		if (ShakeTime <= 0.0f)
			effects->Shake = false;
	}
	// check loss condition
	if (ball->Position.y >= this->Height) // did ball reach bottom edge?
	{
		--this->Lives;
		// did the player lose all his lives? : game over
		if (this->Lives == 0)
		{
			this->ResetLevel();
			this->State = GameState::GAME_MENU;
		}
		this->ResetPlayer();
	}
	// check win condition
	if (this->State == GameState::GAME_ACTIVE && this->Levels[this->Level].IsCompleted())
	{
		this->ResetLevel();
		this->ResetPlayer();
		effects->Chaos = true;
		this->State = GameState::GAME_WIN;
	}
}

void Game::ProcessInput(float dt)
{
	if (this->State == GameState::GAME_MENU)
	{
		if (this->Keys[GLFW_KEY_ENTER] && !this->KeysProcessed[GLFW_KEY_ENTER])
		{
			this->State = GameState::GAME_ACTIVE;
			this->KeysProcessed[GLFW_KEY_ENTER] = true;
		}
		if (this->Keys[GLFW_KEY_W] && !this->KeysProcessed[GLFW_KEY_W])
		{
			this->Level = (this->Level + 1) % 4;
			this->KeysProcessed[GLFW_KEY_W] = true;
		}
		if (this->Keys[GLFW_KEY_S] && !this->KeysProcessed[GLFW_KEY_S])
		{
			if (this->Level > 0)
				--this->Level;
			else
				this->Level = 3;
			//this->Level = (this->Level - 1) % 4;
			this->KeysProcessed[GLFW_KEY_S] = true;
		}
	}
	if (this->State == GameState::GAME_WIN)
	{
		if (this->Keys[GLFW_KEY_ENTER])
		{
			this->KeysProcessed[GLFW_KEY_ENTER] = true;
			effects->Chaos = false;
			this->State = GameState::GAME_MENU;
		}
	}
	if (this->State == GameState::GAME_ACTIVE)
	{
		float velocity = PLAYER_VELOCITY * dt;
		// move playerboard
		if (this->Keys[GLFW_KEY_A])
		{
			if (player->Position.x >= 0.0f) {
				player->Position.x -= velocity;
				if (ball->Stuck)
					ball->Position.x -= velocity;
			}
		}
		if (this->Keys[GLFW_KEY_D])
		{
			if (player->Position.x <= this->Width - player->Size.x) {
				player->Position.x += velocity;
				if (ball->Stuck)
					ball->Position.x += velocity;
			}
		}
		if (this->Keys[GLFW_KEY_SPACE])
			ball->Stuck = false;
	}
}

void Game::Render()
{
	if (this->State == GameState::GAME_ACTIVE || this->State == GameState::GAME_MENU || this->State == GameState::GAME_WIN)
	{
		// begin rendering to postprocessing framebuffer
		effects->BeginRender();

		//mytest
	/*mytest->DrawSprite(ResourceManager::GetTexture("sprite"));*/

		//DrawSprite(texture, position, size, rotate, color)
		// draw background 
		sprite_render->DrawSprite(ResourceManager::GetTexture("background"), glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);

		// draw level
		this->Levels[this->Level].Draw(*sprite_render);

		// draw player
		player->Draw(*sprite_render);

		//draw ball
		ball->Draw(*sprite_render);

		// draw PowerUps
		for (PowerUp& powerUp : this->PowerUps)
			if (!powerUp.Destroyed)
				powerUp.Draw(*sprite_render);

		// draw particles	
		particle_generator->Draw();

		// end rendering to postprocessing framebuffer
		effects->EndRender();
		// render postprocessing quad
		effects->Render(glfwGetTime());

		// render text (don't include in postprocessing)
		std::stringstream ss;
		ss << this->Lives;
		text_renderer->RenderText("Lives:" + ss.str(), 5.0f, 5.0f, 1.0f);
	}
	if (this->State == GameState::GAME_MENU)
	{
		text_renderer->RenderText("Press ENTER to start", 250.0f, this->Height / 2.0f, 1.0f);
		text_renderer->RenderText("Press W or S to select level", 245.0f, this->Height / 2.0f + 20.0f, 0.75f);
	}
	if (this->State == GameState::GAME_WIN)
	{
		text_renderer->RenderText("You WON!!!", 320.0f, this->Height / 2.0f - 20.0f, 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		text_renderer->RenderText("Press ENTER to retry or ESC to quit", 130.0f, this->Height / 2.0f, 1.0f, glm::vec3(1.0f, 1.0f, 0.0f));
	}
}

void Game::ResetLevel()
{
	if (this->Level == 0)
		this->Levels[0].Load("levels/one.txt", this->Width, this->Height / 2);
	else if (this->Level == 1)
		this->Levels[1].Load("levels/two.txt", this->Width, this->Height / 2);
	else if (this->Level == 2)
		this->Levels[2].Load("levels/three.txt", this->Width, this->Height / 2);
	else if (this->Level == 3)
		this->Levels[3].Load("levels/four.txt", this->Width, this->Height / 2);

	this->PowerUps.clear();
	this->Lives = INITIAL_LIVES;
}

void Game::ResetPlayer()
{
	// reset player/ball stats
	player->Size = PLAYER_SIZE;
	player->Position = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
	ball->Reset(player->Position + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS, -(BALL_RADIUS * 2.0f)), INITIAL_BALL_VELOCITY);
	// also disable all active powerups
	effects->Chaos = effects->Confuse = false;
	ball->PassThrough = ball->Sticky = false;
	player->Color = glm::vec3(1.0f);
	ball->Color = glm::vec3(1.0f);
}

// AABB - AABB collision
bool Game::CheckCollision(GameObject& one, GameObject& two)
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

// AABB - Circle collision
Collision Game::CheckCollision(BallObject& one, GameObject& two)
{
	// get center point circle first 
	glm::vec2 center(one.Position + one.Radius);
	// calculate AABB info (center, half-extents)
	glm::vec2 aabb_half_extents(two.Size.x / 2.0f, two.Size.y / 2.0f);
	glm::vec2 aabb_center(two.Position.x + aabb_half_extents.x, two.Position.y + aabb_half_extents.y);
	// get difference vector between both centers
	glm::vec2 difference = center - aabb_center;
	glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
	//as same as closest - center
	difference -= clamped;

	if (glm::length(difference) < one.Radius) // not <= since in that case a collision also occurs when object one exactly touches object two, which they are at the end of each collision resolution stage.
		return std::make_tuple(true, VectorDirection(difference), difference);
	else
		return std::make_tuple(false, Direction::UP, glm::vec2(0.0f, 0.0f));
}

void Game::DoCollisions() {
	for (GameObject& box : this->Levels[this->Level].Bricks)
	{
		if (!box.Destroyed)
		{
			Collision collision = CheckCollision(*ball, box);
			if (std::get<0>(collision)) // if collision is true
			{
				// destroy block if not solid
				if (!box.IsSolid) {
					box.Destroyed = true;
					this->SpawnPowerUps(box);
					SoundEngine->play2D("audio/bleep.mp3", false);
				}
				else
				{   // if block is solid, enable shake effect
					ShakeTime = 0.05f;
					effects->Shake = true;
					SoundEngine->play2D("audio/solid.wav", false);
				}
				// collision resolution
				Direction dir = std::get<1>(collision);
				glm::vec2 diff_vector = std::get<2>(collision);
				if (!(ball->PassThrough && !box.IsSolid)) {
					if (dir == Direction::LEFT || dir == Direction::RIGHT) // horizontal collision
					{
						ball->Velocity.x = -ball->Velocity.x; // reverse horizontal velocity
						// relocate
						float penetration = ball->Radius - std::abs(diff_vector.x);
						if (dir == Direction::LEFT)
							ball->Position.x -= penetration; // move ball to left
						else
							ball->Position.x += penetration; // move ball to right;
					}
					else // vertical collision
					{
						ball->Velocity.y = -ball->Velocity.y; // reverse vertical velocity
						// relocate
						float penetration = ball->Radius - std::abs(diff_vector.y);
						if (dir == Direction::UP)
							ball->Position.y -= penetration; // move ball bback up
						else
							ball->Position.y += penetration; // move ball back down
					}
				}
			}
		}
	}

	//check collisions on PowerUps and if so, activate them
	for (PowerUp& powerUp : this->PowerUps)
	{
		if (!powerUp.Destroyed)
		{
			// first check if powerup passed bottom edge, if so: keep as inactive and destroy
			if (powerUp.Position.y >= this->Height)
				powerUp.Destroyed = true;

			if (CheckCollision(*player, powerUp))
			{	// collided with player, now activate powerup
				ActivatePowerUp(powerUp);
				powerUp.Destroyed = true;
				powerUp.Activated = true;
				SoundEngine->play2D("audio/powerup.wav", false);
			}
		}
	}

	// check collisions for player pad (unless stuck)
	Collision result = CheckCollision(*ball, *player);
	if (!ball->Stuck && std::get<0>(result))
	{
		// check where it hit the board, and change velocity based on where it hit the board
		float centerBoard = player->Position.x + player->Size.x / 2.0f;
		float distance = (ball->Position.x + ball->Radius) - centerBoard;
		float percentage = distance / (player->Size.x / 2.0f);
		// then move accordingly
		float strength = 2.0f;
		glm::vec2 oldVelocity = ball->Velocity;
		ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
		ball->Velocity = glm::normalize(ball->Velocity) * glm::length(oldVelocity); // keep speed consistent over both axes (multiply by length of old velocity, so total strength is not changed)
		// fix sticky paddle
		ball->Velocity.y = -1.0f * abs(ball->Velocity.y);

		// if Sticky powerup is activated, also stick ball to paddle once new velocity vectors were calculated
		ball->Stuck = ball->Sticky;

		SoundEngine->play2D("audio/paddle.wav", false);
	}
}

//center is start,closest - center
// left,up is origin
// calculates which direction a vector is facing (N,E,S or W)
Direction Game::VectorDirection(glm::vec2 target)
{
	glm::vec2 compass[] = {
		glm::vec2(0.0f, 1.0f),	// up
		glm::vec2(1.0f, 0.0f),	// left
		glm::vec2(0.0f, -1.0f),	// down
		glm::vec2(-1.0f, 0.0f)	// right
	};
	float max = 0.0f;
	unsigned int best_match = -1;
	for (unsigned int i = 0; i < 4; i++)
	{
		float dot_product = glm::dot(glm::normalize(target), compass[i]);
		if (dot_product > max)
		{
			max = dot_product;
			best_match = i;
		}
	}
	return (Direction)best_match;
}

bool Game::ShouldSpawn(unsigned int chance)
{
	unsigned int random = rand() % chance;
	return random == 0;
}

//GameObject->PowerUps
void Game::SpawnPowerUps(GameObject& block)
{
	//PowerUps(type, color,  duration, position, texture) VELOCITY(0.0f, 150.0f)
	if (ShouldSpawn(75)) // 1 in 75 chance
		this->PowerUps.push_back(PowerUp("speed", glm::vec3(0.5f, 0.5f, 1.0f), 0.0f, block.Position, ResourceManager::GetTexture("powerup_speed")));
	if (ShouldSpawn(75))
		this->PowerUps.push_back(PowerUp("sticky", glm::vec3(1.0f, 0.5f, 1.0f), 20.0f, block.Position, ResourceManager::GetTexture("powerup_sticky")));
	if (ShouldSpawn(75))
		this->PowerUps.push_back(PowerUp("pass-through", glm::vec3(0.5f, 1.0f, 0.5f), 10.0f, block.Position, ResourceManager::GetTexture("powerup_passthrough")));
	if (ShouldSpawn(75))
		this->PowerUps.push_back(PowerUp("pad-size-increase", glm::vec3(1.0f, 0.6f, 0.4), 0.0f, block.Position, ResourceManager::GetTexture("powerup_increase")));
	if (ShouldSpawn(15)) // Negative powerups should spawn more often
		this->PowerUps.push_back(PowerUp("confuse", glm::vec3(1.0f, 0.3f, 0.3f), 15.0f, block.Position, ResourceManager::GetTexture("powerup_confuse")));
	if (ShouldSpawn(15))
		this->PowerUps.push_back(PowerUp("chaos", glm::vec3(0.9f, 0.25f, 0.25f), 15.0f, block.Position, ResourceManager::GetTexture("powerup_chaos")));
	//treat powerup
	if (ShouldSpawn(50))
		this->PowerUps.push_back(PowerUp("treat", glm::vec3(1.0f, 0.5f, 0.7f), 0.0f, block.Position, ResourceManager::GetTexture("powerup_treat")));
	//remove negative powerups
	if (effects->Chaos || effects->Confuse) {
		if (ShouldSpawn(20))
			this->PowerUps.push_back(PowerUp("remove", glm::vec3(1.0f, 0.5f, 0.5f), 1.0f, block.Position, ResourceManager::GetTexture("powerup_remove")));
	}
}

void Game::ActivatePowerUp(PowerUp& powerUp)
{
	if (powerUp.Type == "speed")
	{
		ball->Velocity *= 1.2;
	}
	else if (powerUp.Type == "sticky")
	{
		ball->Sticky = true;
		player->Color = glm::vec3(1.0f, 0.5f, 1.0f);
	}
	else if (powerUp.Type == "pass-through")
	{
		ball->PassThrough = true;
		ball->Color = glm::vec3(1.0f, 0.5f, 0.5f);
	}
	else if (powerUp.Type == "pad-size-increase")
	{
		player->Size.x += 50;
	}
	else if (powerUp.Type == "confuse")
	{
		if (!effects->Chaos && !effects->Remove)
			effects->Confuse = true; // only activate if chaos wasn't already active
	}
	else if (powerUp.Type == "chaos")
	{
		if (!effects->Confuse && !effects->Remove)
			effects->Chaos = true;
	}
	else if (powerUp.Type == "treat")
	{
		++this->Lives;
	}
	else if (powerUp.Type == "remove") {
		effects->Chaos = false;
		effects->Confuse = false;
		for (PowerUp& p : this->PowerUps) {
			if (p.Activated) {
				if (p.Type == "chaos" || p.Type == "confuse")
					p.Activated = false;
			}
		}
		effects->Remove = true;
	}
}

bool Game::IsOtherPowerUpActive(std::string type) {
	// Check if another PowerUp of the same type is still active
	// in which case we don't disable its effect (yet)
	for (const PowerUp& powerUp : this->PowerUps)
	{
		if (powerUp.Activated)
			if (powerUp.Type == type)
				return true;
	}
	return false;
}

void Game::UpdatePowerUps(float dt)
{
	for (PowerUp& powerUp : this->PowerUps)
	{
		powerUp.Position += powerUp.Velocity * dt;
		if (powerUp.Activated)
		{
			powerUp.Duration -= dt;

			if (powerUp.Duration <= 0.0f)
			{
				// remove powerup from list (will later be removed)
				powerUp.Activated = false;
				// deactivate effects
				if (powerUp.Type == "sticky")
				{
					if (!IsOtherPowerUpActive("sticky"))
					{	// only reset if no other PowerUp of type sticky is active
						ball->Sticky = false;
						player->Color = glm::vec3(1.0f);
					}
				}
				else if (powerUp.Type == "pass-through")
				{
					if (!IsOtherPowerUpActive("pass-through"))
					{	// only reset if no other PowerUp of type pass-through is active
						ball->PassThrough = false;
						ball->Color = glm::vec3(1.0f);
					}
				}
				else if (powerUp.Type == "confuse")
				{
					if (!IsOtherPowerUpActive("confuse"))
					{	// only reset if no other PowerUp of type confuse is active
						effects->Confuse = false;
					}
				}
				else if (powerUp.Type == "chaos")
				{
					if (!IsOtherPowerUpActive("chaos"))
					{	// only reset if no other PowerUp of type chaos is active
						effects->Chaos = false;
					}
				}
				else if (powerUp.Type == "remove")
				{
					if (!IsOtherPowerUpActive("remove"))
					{	// only reset if no other PowerUp of type remove is active
						effects->Remove = false;
					}
				}
			}
		}
	}
	// Remove all PowerUps from vector that are destroyed AND !activated (thus either off the map or finished)
	// Note we use a lambda expression to remove each PowerUp which is destroyed and not activated
	this->PowerUps.erase(std::remove_if(this->PowerUps.begin(), this->PowerUps.end(), [](const PowerUp& powerUp) {
		return powerUp.Destroyed && !powerUp.Activated;
		}), this->PowerUps.end());
}