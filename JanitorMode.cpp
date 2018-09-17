#include "JanitorMode.hpp"

#include "MenuMode.hpp"
#include "Load.hpp"
#include "Sound.hpp"
#include "MeshBuffer.hpp"
#include "WalkMesh.hpp"
#include "gl_errors.hpp" //helper for dumpping OpenGL error messages
#include "read_chunk.hpp" //helper for reading a vector of structures from a file
#include "data_path.hpp" //helper to get paths relative to executable
#include "compile_program.hpp" //helper to compile opengl shader programs
#include "draw_text.hpp" //helper to... um.. draw text
#include "vertex_color_program.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <cstddef>
#include <random>

Load< MeshBuffer > game_meshes(LoadTagDefault, [](){
	return new MeshBuffer(data_path("meshes.pnc"));
});

Load<WalkMesh> walk_mesh(LoadTagDefault, []() {
  return new WalkMesh(data_path("walkmesh.blob"));
});

Load< GLuint > game_meshes_for_vertex_color_program(LoadTagDefault, [](){
	return new GLuint(game_meshes->make_vao_for_program(vertex_color_program->program));
});

Load< Sound::Sample > sample_loop(LoadTagDefault, [](){
	return new Sound::Sample(data_path("loop.wav"));
});

Load< Sound::Sample > sample_blood(LoadTagDefault, [](){
	//all are creative commons liscenced sounds I found online
	return new Sound::Sample(data_path("blood.wav"));
});

Load< Sound::Sample > sample_vom(LoadTagDefault, [](){
	return new Sound::Sample(data_path("vomit.wav"));
});

Load< Sound::Sample > sample_mop(LoadTagDefault, [](){
	return new Sound::Sample(data_path("mop.wav"));
});


glm::vec3 random_coord() {
	// arbitrary choice to return an x coordinate between 3 and 13 in world space, 
	// 		and y coodinate between -4.5 and 4.5 (bc I was too lazy to resize mesh)
	float x = (rand() % 1000) * 0.01f + 3.0f;
	float y = (rand() % 900) * 0.01f + -4.5f;
	return glm::vec3(x, y, 1.0f);
}

void set_rotation(Scene::Transform *transform, glm::vec3 normal_vector) {
		glm::vec3 right_vector;
		if (normal_vector.x>0.0f) right_vector = glm::vec3(0.0f, 1.0f, 0.0f);
		else right_vector = glm::vec3(0.0f,-1.0f, 0.0f);

		float dist = std::sqrtf(normal_vector.x * normal_vector.x + normal_vector.y * normal_vector.y);
		float magnitude = std::atan2f(dist, normal_vector.z);
		transform->rotation = glm::normalize(glm::angleAxis(0.0f, normal_vector) *
                     glm::angleAxis(magnitude, right_vector));
}

bool intersect(glm::vec3 p1, glm::vec3 p2) {
	//lazy non mesh intersect bc it's not that important for a complex mess meshes
	float x = p2.x - p1.x;
	float y = p2.y - p1.y;
	float epsilon = 0.5f;

	float dist = std::sqrtf(x*x + y*y);
	if (dist < epsilon){
		return true;
	} else return false;
}

JanitorMode::JanitorMode() {
	//----------------
	//set up scene:
	srand(time(NULL));

	auto attach_object = [this](Scene::Transform *transform, std::string const &name) {
		Scene::Object *object = scene.new_object(transform);
		object->program = vertex_color_program->program;
		object->program_mvp_mat4 = vertex_color_program->object_to_clip_mat4;
		object->program_mv_mat4x3 = vertex_color_program->object_to_light_mat4x3;
		object->program_itmv_mat3 = vertex_color_program->normal_to_light_mat3;
		object->vao = *game_meshes_for_vertex_color_program;
		MeshBuffer::Mesh const &mesh = game_meshes->lookup(name);
		object->start = mesh.start;
		object->count = mesh.count;
		return object;
	};

	{ //build some sort of content:
		//Crate at the origin:
		glm::vec3 start_point = glm::vec3(0.0f, 0.0f, 0.0f);
		wp = walk_mesh->start(start_point);
		glm::vec3 world_point = walk_mesh->world_point(wp);
		glm::vec3 world_normal = walk_mesh->world_normal(wp);
		
		glm::vec3 vom_norm, blood_norm;
		glm::vec3 vom_point = random_coord();
		WalkMesh::WalkPoint vom_p = walk_mesh->start(vom_point);
		vom_point = walk_mesh->world_point(vom_p);
		vom_norm = walk_mesh->world_normal(vom_p);

		glm::vec3 blood_point = random_coord();
		WalkMesh::WalkPoint blood_p = walk_mesh->start(blood_point);
		blood_point = walk_mesh->world_point(blood_p);
		blood_norm = walk_mesh->world_normal(blood_p);

		Scene::Transform *transform1 = scene.new_transform();
		transform1->position = glm::vec3(0.0f, 0.0f, 0.0f);
		transform1->rotation = glm::angleAxis(float(2 * M_PI), glm::vec3(0.0f, 0.0f, 1.0f));
		large_crate = attach_object(transform1, "Floor");

		Scene::Transform *transform2 = scene.new_transform();
		transform2->position = world_point;
		set_rotation(transform2, world_normal);
		player = attach_object(transform2, "Player");

		Scene::Transform *transform3 = scene.new_transform();
		transform3->position = vom_point + 0.05f*vom_norm;
		//raise them off the ground
		set_rotation(transform3, walk_mesh->world_normal(vom_p));
		vomit = attach_object(transform3, "Vomit");

		Scene::Transform *transform4 = scene.new_transform();
		transform4->position = blood_point + 0.05f*blood_norm;
		set_rotation(transform4, walk_mesh->world_normal(blood_p));
		blood = attach_object(transform4, "Blood");
		//smaller crate on top:
	}

	{ //Camera looking at the origin:
		Scene::Transform *transform = scene.new_transform();
		transform->position = glm::vec3(-20.0f, 0.0f, 30.0f);
		//Cameras look along -z, so rotate view to look at origin:
		transform->rotation = glm::angleAxis(-glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		camera = scene.new_camera(transform);
	}
	
	//start the 'loop' sample playing at the large crate:
	loop = sample_loop->play(large_crate->transform->position, 1.0f, Sound::Loop);


}

JanitorMode::~JanitorMode() {
	if (loop) loop->stop();
}

bool JanitorMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	//ignore any keys that are the result of automatic key repeat:
	if (evt.type == SDL_KEYDOWN && evt.key.repeat) {
		return false;
	}
	//handle tracking the state of WSAD for movement control:
	if (!lose && !win && (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP)) {
		if (evt.key.keysym.scancode == SDL_SCANCODE_W) {
			controls.forward = (evt.type == SDL_KEYDOWN);
			return true;
		} else if (evt.key.keysym.scancode == SDL_SCANCODE_S) {
			controls.backward = (evt.type == SDL_KEYDOWN);
			return true;
		} else if (evt.key.keysym.scancode == SDL_SCANCODE_A) {
			controls.left = (evt.type == SDL_KEYDOWN);
			return true;
		} else if (evt.key.keysym.scancode == SDL_SCANCODE_D) {
			controls.right = (evt.type == SDL_KEYDOWN);
			return true;
		} 
		// else if (evt.key.keysym.scancode == SDL_SCANCODE_P) {
		// 	glm::vec3 t = player->transform->position;
		// 	std::cout << "x " << t.x << " y " << t.y << " z " << t.x << "\n";
		// 	return true;
		// }
	}
	//handle tracking the mouse for rotation control:
	// if (!mouse_captured) {
	if (evt.type == SDL_KEYDOWN && evt.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
		show_pause_menu();
		return true;
	}
	if (evt.type == SDL_MOUSEBUTTONDOWN) {
		SDL_SetRelativeMouseMode(SDL_TRUE);
		mouse_captured = true;
		return true;
	}
	// } else if (mouse_captured) {
	// 	if (evt.type == SDL_KEYDOWN && evt.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
	// 		SDL_SetRelativeMouseMode(SDL_FALSE);
	// 		mouse_captured = false;
	// 		return true;
	// 	}
	// 	if (evt.type == SDL_MOUSEMOTION) {
	// 		//Note: float(window_size.y) * camera->fovy is a pixels-to-radians conversion factor
	// 		float yaw = evt.motion.xrel / float(window_size.y) * camera->fovy;
	// 		float pitch = evt.motion.yrel / float(window_size.y) * camera->fovy;
	// 		yaw = -yaw;
	// 		pitch = -pitch;
	// 		// camera->transform->rotation = glm::normalize(
	// 		// 	camera->transform->rotation
	// 		// 	* glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f))
	// 		// 	* glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f))
	// 		// );

	// 		return true;
	// 	}
	// }
	return false;
}

void JanitorMode::update(float elapsed) {
	if (win) show_win();
	if (game_time > 0) game_time -= elapsed;
	else lose = true;
	if (score >= 20) {
		lose = false;
		win = true;
	}

	if (3 < messes_cleaned){
		mop_dirty = true;
		messes_cleaned = 0;
	}

	if (!mop_dirty){
		if (intersect(player->transform->position, 
		vomit->transform->position)) {
			glm::vec3 vom_norm;
			glm::vec3 vom_point = random_coord();
			WalkMesh::WalkPoint vom_p = walk_mesh->start(vom_point);
			vom_point = walk_mesh->world_point(vom_p);
			vom_norm = walk_mesh->world_normal(vom_p);
			messes_cleaned ++;
			score++;

			vomit->transform->position = vom_point + 0.05f*vom_norm;
			set_rotation(vomit->transform, vom_norm);

			glm::mat4x3 to_world = camera->transform->make_local_to_world();
			sample_vom->play(to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) );
		}
		if (intersect(player->transform->position, 
			blood->transform->position)) {
			glm::vec3 blood_norm;
			glm::vec3 blood_point = random_coord();
			WalkMesh::WalkPoint blood_p = walk_mesh->start(blood_point);
			blood_point = walk_mesh->world_point(blood_p);
			blood_norm = walk_mesh->world_normal(blood_p);
			messes_cleaned ++;
			score++;

			blood->transform->position = blood_point + 0.05f*blood_norm;
			set_rotation(blood->transform, blood_norm);

			glm::mat4x3 to_world = camera->transform->make_local_to_world();
			sample_blood->play(to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) );
		}
	} else {
		if (intersect(player->transform->position, 
			glm::vec3(22.248f, 10.29f, 22.248f))){
			//this is about where the janitor closet is but it doesn't have its 
			//own object because I made it all one manifold mesh for some reason
			mop_dirty = false;
			glm::mat4x3 to_world = camera->transform->make_local_to_world();
			sample_mop->play(to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) );
		}
	}

	glm::vec3 x_dir = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 y_dir = glm::vec3(0.0f, 1.0f, 0.0f);
	float amt = 10.0f * elapsed;

	// if (controls.right) camera->transform->position += amt * directions[0];
	// if (controls.left) camera->transform->position -= amt * directions[0];
	// if (controls.backward) camera->transform->position += amt * directions[2];
	// if (controls.forward) camera->transform->position -= amt * directions[2];
	if (!lose && !win){
		if (controls.right) walk_mesh->walk(wp, amt * x_dir);
		if (controls.left) walk_mesh->walk(wp, -amt * x_dir);
		if (controls.backward) walk_mesh->walk(wp, -amt * y_dir);
		if (controls.forward) walk_mesh->walk(wp, amt * y_dir);
	}
	{ //set sound positions:
		glm::mat4 cam_to_world = camera->transform->make_local_to_world();
		Sound::listener.set_position( cam_to_world[3] );
		//camera looks down -z, so right is +x:
		Sound::listener.set_right( glm::normalize(cam_to_world[0]) );

		if (loop) {
			glm::mat4 large_crate_to_world = large_crate->transform->make_local_to_world();
			loop->set_position( large_crate_to_world * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) );
		}
	}

	if (!win && !lose) {
		glm::vec3 normal_vector = walk_mesh->world_normal(wp);
		player->transform->position = walk_mesh->world_point(wp);
		set_rotation(player->transform, normal_vector);
		player_pos = player->transform->position;

		camera->transform->position = player_pos + 10.0f * normal_vector;
		set_rotation(camera->transform, normal_vector);
	}

	if (lose) {
		player->transform->position = glm::vec3(100.0f, -0.6f, 0.0f);
		camera->transform->position = glm::vec3(100.0f, 0.0f, 2.0f);
		// basically same as set_rotation logic but it was being weird idk
		glm::vec3 normal_vector = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 right_vector = glm::vec3(-1.0f, 0.0f, 0.0f);
		float dist = std::sqrtf(normal_vector.x * normal_vector.x + normal_vector.y * normal_vector.y);
		float magnitude = std::atan2f(dist, normal_vector.z);
		player->transform->rotation = glm::normalize(glm::angleAxis(0.0f, normal_vector) *
                     glm::angleAxis(magnitude, right_vector));
		set_rotation(camera->transform, glm::vec3(0.0f, 0.0f, 1.0f));
		player->program = vertex_color_program->program;
		player->program_mvp_mat4 = vertex_color_program->object_to_clip_mat4;
		player->program_mv_mat4x3 = vertex_color_program->object_to_light_mat4x3;
		player->program_itmv_mat3 = vertex_color_program->normal_to_light_mat3;
		player->vao = *game_meshes_for_vertex_color_program;
		MeshBuffer::Mesh const &mesh = game_meshes->lookup("Player_Lose");
		player->start = mesh.start;
		player->count = mesh.count;
		show_lose();
	}

	if (win && !lose) {
		player->transform->position = glm::vec3(100.0f, -0.8f, 0.0f);
		camera->transform->position = glm::vec3(100.0f, 0.0f, 2.0f);
		// basically same as set_rotation logic but it was being weird idk
		glm::vec3 normal_vector = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 right_vector = glm::vec3(-1.0f, 0.0f, 0.0f);
		float dist = std::sqrtf(normal_vector.x * normal_vector.x + normal_vector.y * normal_vector.y);
		float magnitude = std::atan2f(dist, normal_vector.z);
		player->transform->rotation = glm::normalize(glm::angleAxis(0.0f, normal_vector) *
                     glm::angleAxis(magnitude, right_vector));
		set_rotation(camera->transform, glm::vec3(0.0f, 0.0f, 1.0f));
		player->program = vertex_color_program->program;
		player->program_mvp_mat4 = vertex_color_program->object_to_clip_mat4;
		player->program_mv_mat4x3 = vertex_color_program->object_to_light_mat4x3;
		player->program_itmv_mat3 = vertex_color_program->normal_to_light_mat3;
		player->vao = *game_meshes_for_vertex_color_program;
		MeshBuffer::Mesh const &mesh = game_meshes->lookup("Player_Win");
		player->start = mesh.start;
		player->count = mesh.count;
		show_win();
	}
}

void JanitorMode::draw(glm::uvec2 const &drawable_size) {
	//set up basic OpenGL state:
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//set up light position + color:
	glUseProgram(vertex_color_program->program);
	glUniform3fv(vertex_color_program->sun_color_vec3, 1, glm::value_ptr(glm::vec3(0.81f, 0.81f, 0.76f)));
	glUniform3fv(vertex_color_program->sun_direction_vec3, 1, glm::value_ptr(glm::normalize(glm::vec3(-0.2f, 0.2f, 1.0f))));
	glUniform3fv(vertex_color_program->sky_color_vec3, 1, glm::value_ptr(glm::vec3(0.4f, 0.4f, 0.45f)));
	glUniform3fv(vertex_color_program->sky_direction_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 1.0f, 0.0f)));
	glUseProgram(0);

	//fix aspect ratio of camera
	camera->aspect = drawable_size.x / float(drawable_size.y);

	scene.draw(camera);

	if (Mode::current.get() == this) {
		glDisable(GL_DEPTH_TEST);
		std::string help_message;
		if (!mop_dirty) help_message = "USE WASD TO MOVE";
		else help_message = "YOUR MOP IS DIRTY!";
		float height = 0.06f;
		float width = text_width(help_message, height);
		draw_text(help_message, glm::vec2(-0.5f * width,-0.99f), height, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
		draw_text(help_message, glm::vec2(-0.5f * width,-1.0f), height, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));


		std::string time_str = std::to_string((int)game_time);
		height = 0.1f;
		width = text_width(time_str, height);
		draw_text(time_str, glm::vec2(1.0f,0.8f), height, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		
		std::string score_str = std::to_string(score);
		std::string score_msg = "SCORE ";
		height = 0.05f;
		width = text_width(score_msg, height);
		draw_text(score_msg, glm::vec2(0.6f,0.7f), height, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		height = 0.1f;
		width = text_width(score_str, height);
		draw_text(score_str, glm::vec2(1.0f,0.68f), height, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		glUseProgram(0);
	}

	GL_ERRORS();
}

void JanitorMode::show_win(){
	std::shared_ptr< MenuMode > menu = std::make_shared< MenuMode >();

	std::shared_ptr< Mode > game = shared_from_this();
	menu->background = game;

	menu->choices.emplace_back("YOU WIN!!!");

	Mode::set_current(menu);
}

void JanitorMode::show_lose(){
	std::shared_ptr< MenuMode > menu = std::make_shared< MenuMode >();

	std::shared_ptr< Mode > game = shared_from_this();
	menu->background = game;

	menu->choices.emplace_back("YOU LOSE");

	Mode::set_current(menu);


}

void JanitorMode::show_pause_menu() {
	std::shared_ptr< MenuMode > menu = std::make_shared< MenuMode >();

	std::shared_ptr< Mode > game = shared_from_this();
	menu->background = game;

	menu->choices.emplace_back("PAUSED");
	menu->choices.emplace_back("RESUME", [game](){
		Mode::set_current(game);
	});
	menu->choices.emplace_back("QUIT", [](){
		Mode::set_current(nullptr);
	});

	menu->selected = 1;

	Mode::set_current(menu);
}