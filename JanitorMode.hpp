#pragma once

#include "Mode.hpp"
#include "WalkMesh.hpp"


#include "MeshBuffer.hpp"
#include "GL.hpp"
#include "Scene.hpp"
#include "Sound.hpp"

#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>

// The 'JanitorMode' shows scene with some crates in it:

struct JanitorMode : public Mode {
	JanitorMode();
	virtual ~JanitorMode();

	//handle_event is called when new mouse or keyboard events are received:
	// (note that this might be many times per frame or never)
	//The function should return 'true' if it handled the event.
	virtual bool handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) override;

	//update is called at the start of a new frame, after events are handled:
	virtual void update(float elapsed) override;

	//draw is called after update:
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//starts up a 'quit/resume' pause menu:
	void show_pause_menu();
	void show_win();
	void show_lose();

	struct {
		bool forward = false;
		bool backward = false;
		bool left = false;
		bool right = false;
	} controls;

	bool mouse_captured = false;

	Scene scene;
	Scene::Camera *camera = nullptr;

	Scene::Object *large_crate = nullptr;
	Scene::Object *player = nullptr;
	Scene::Object *vomit = nullptr;
	Scene::Object *blood = nullptr;

	//this 'loop' sample is played at the large crate:
	std::shared_ptr< Sound::PlayingSample > loop;
	std::shared_ptr< Sound::PlayingSample > woosh;

	WalkMesh::WalkPoint wp;

	glm::vec3 player_pos;

	bool win = false;
	bool lose = false;
	bool mop_dirty = false;

	float game_time = 60.0f;
	uint messes_cleaned = 0;
	uint score = 0;

};
