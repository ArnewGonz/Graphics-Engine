#include "application.h"
#include "utils.h"
#include "mesh.h"
#include "texture.h"

#include "fbo.h"
#include "shader.h"
#include "input.h"
#include "includes.h"
#include "prefab.h"
#include "gltf_loader.h"
#include "renderer.h"
#include "extra/hdre.h"

#include <cmath>
#include <string>
#include <cstdio>

Application* Application::instance = nullptr;

Camera* camera = nullptr;
GTR::Scene* scene = nullptr;
GTR::Prefab* prefab = nullptr;
GTR::Renderer* renderer = nullptr;
GTR::BaseEntity* selected_entity = nullptr;
FBO* fbo = nullptr;
Texture* texture = nullptr;

float cam_speed = 10;

Application::Application(int window_width, int window_height, SDL_Window* window)
{
	this->window_width = window_width;
	this->window_height = window_height;
	this->window = window;
	instance = this;
	must_exit = false;
	render_debug = true;
	render_gui = true;

	render_wireframe = false;

	fps = 0;
	frame = 0;
	time = 0.0f;
	elapsed_time = 0.0f;
	mouse_locked = false;

	quality = MEDIUM;
	change_res = false;

	//loads and compiles several shaders from one single file
    //change to "data/shader_atlas_osx.txt" if you are in XCODE
#ifdef __APPLE__
    const char* shader_atlas_filename = "data/shader_atlas_osx.txt";
#else
    const char* shader_atlas_filename = "data/shader_atlas.txt";
#endif
	if(!Shader::LoadAtlas(shader_atlas_filename))
        exit(1);
    checkGLErrors();


	// Create camera
	camera = new Camera();
	camera->lookAt(Vector3(-150.f, 150.0f, 250.f), Vector3(0.f, 0.0f, 0.f), Vector3(0.f, 1.f, 0.f));
	camera->setPerspective( 45.f, window_width/(float)window_height, 1.0f, 10000.f);

	//Example of loading a prefab
	//prefab = GTR::Prefab::Get("data/prefabs/gmc/scene.gltf");

	scene = new GTR::Scene();
	if (!scene->load("data/scene.json"))
		exit(1);

	camera->lookAt(scene->main_camera.eye, scene->main_camera.center, Vector3(0, 1, 0));
	camera->fov = scene->main_camera.fov;

	//This class will be the one in charge of rendering all 
	renderer = new GTR::Renderer(); //here so we have opengl ready in constructor

	HDRE* hdre = new HDRE();
	if (hdre->load("data/night.hdre"));
		scene->environment = GTR::CubemapFromHDRE("data/night.hdre");


	//hide the cursor
	SDL_ShowCursor(!mouse_locked); //hide or show the mouse
}

//what to do when the image has to be draw
void Application::render(void)
{
	//be sure no errors present in opengl before start
	checkGLErrors();

	//set the camera as default (used by some functions in the framework)
	camera->enable();

	//set default flags
	glDisable(GL_BLEND);
    
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	if(render_wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//lets render something
	//Matrix44 model;
	//renderer->renderPrefab( model, prefab, camera );

	//renderer->renderToFBO(scene, camera);
	renderer->renderToFBO(scene, camera);

	//Draw the floor grid, helpful to have a reference point
	//if(render_debug && !renderer->depth_viewport)
	//	drawGrid();

    glDisable(GL_DEPTH_TEST);
    //render anything in the gui after this

	//the swap buffers is done in the main loop after this function
}

void Application::update(double seconds_elapsed)
{
	float speed = seconds_elapsed * cam_speed * 25; //the speed is defined by the seconds_elapsed so it goes constant
	float orbit_speed = seconds_elapsed * 0.5;
	
	//async input to move the camera around
	if (Input::isKeyPressed(SDL_SCANCODE_LSHIFT)) speed *= 10; //move faster with left shift
	if (Input::isKeyPressed(SDL_SCANCODE_W) || Input::isKeyPressed(SDL_SCANCODE_UP)) camera->move(Vector3(0.0f, 0.0f, 1.0f) * speed);
	if (Input::isKeyPressed(SDL_SCANCODE_S) || Input::isKeyPressed(SDL_SCANCODE_DOWN)) camera->move(Vector3(0.0f, 0.0f,-1.0f) * speed);
	if (Input::isKeyPressed(SDL_SCANCODE_A) || Input::isKeyPressed(SDL_SCANCODE_LEFT)) camera->move(Vector3(1.0f, 0.0f, 0.0f) * speed);
	if (Input::isKeyPressed(SDL_SCANCODE_D) || Input::isKeyPressed(SDL_SCANCODE_RIGHT)) camera->move(Vector3(-1.0f, 0.0f, 0.0f) * speed);

	//mouse input to rotate the cam
	#ifndef SKIP_IMGUI
	if (!ImGuizmo::IsUsing())
	#endif
	{
		if (mouse_locked || Input::mouse_state & SDL_BUTTON(SDL_BUTTON_RIGHT)) //move in first person view
		{
			camera->rotate(-Input::mouse_delta.x * orbit_speed * 0.5, Vector3(0, 1, 0));
			Vector3 right = camera->getLocalVector(Vector3(1, 0, 0));
			camera->rotate(-Input::mouse_delta.y * orbit_speed * 0.5, right);
		}
		else //orbit around center
		{
			bool mouse_blocked = false;
			#ifndef SKIP_IMGUI
						mouse_blocked = ImGui::IsAnyWindowHovered() || ImGui::IsAnyItemHovered() || ImGui::IsAnyItemActive();
			#endif
			if (Input::mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT) && !mouse_blocked) //is left button pressed?
			{
				camera->orbit(-Input::mouse_delta.x * orbit_speed, Input::mouse_delta.y * orbit_speed);
			}
		}
	}
	
	//move up or down the camera using Q and E
	if (Input::isKeyPressed(SDL_SCANCODE_Q)) camera->moveGlobal(Vector3(0.0f, -1.0f, 0.0f) * speed);
	if (Input::isKeyPressed(SDL_SCANCODE_E)) camera->moveGlobal(Vector3(0.0f, 1.0f, 0.0f) * speed);

	//to navigate with the mouse fixed in the middle
	SDL_ShowCursor(!mouse_locked);
	#ifndef SKIP_IMGUI
		ImGui::SetMouseCursor(mouse_locked ? ImGuiMouseCursor_None : ImGuiMouseCursor_Arrow);
	#endif
	if (mouse_locked)
	{
		Input::centerMouse();
		//ImGui::SetCursorPos(ImVec2(Input::mouse_position.x, Input::mouse_position.y));
	}
}

void Application::renderDebugGizmo()
{
	if (!selected_entity || !render_debug)
		return;

	//example of matrix we want to edit, change this to the matrix of your entity
	Matrix44& matrix = selected_entity->model;

	#ifndef SKIP_IMGUI

	static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
	static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
	if (ImGui::IsKeyPressed(90))
		mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	if (ImGui::IsKeyPressed(69))
		mCurrentGizmoOperation = ImGuizmo::ROTATE;
	if (ImGui::IsKeyPressed(82)) // r Key
		mCurrentGizmoOperation = ImGuizmo::SCALE;
	if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
		mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
		mCurrentGizmoOperation = ImGuizmo::ROTATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
		mCurrentGizmoOperation = ImGuizmo::SCALE;
	float matrixTranslation[3], matrixRotation[3], matrixScale[3];
	ImGuizmo::DecomposeMatrixToComponents(matrix.m, matrixTranslation, matrixRotation, matrixScale);
	ImGui::InputFloat3("Tr", matrixTranslation, 3);
	ImGui::InputFloat3("Rt", matrixRotation, 3);
	ImGui::InputFloat3("Sc", matrixScale, 3);
	ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrix.m);

	if (mCurrentGizmoOperation != ImGuizmo::SCALE)
	{
		if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
			mCurrentGizmoMode = ImGuizmo::LOCAL;
		ImGui::SameLine();
		if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
			mCurrentGizmoMode = ImGuizmo::WORLD;
	}
	static bool useSnap(false);
	if (ImGui::IsKeyPressed(83))
		useSnap = !useSnap;
	ImGui::Checkbox("", &useSnap);
	ImGui::SameLine();
	static Vector3 snap;
	switch (mCurrentGizmoOperation)
	{
	case ImGuizmo::TRANSLATE:
		//snap = config.mSnapTranslation;
		ImGui::InputFloat3("Snap", &snap.x);
		break;
	case ImGuizmo::ROTATE:
		//snap = config.mSnapRotation;
		ImGui::InputFloat("Angle Snap", &snap.x);
		break;
	case ImGuizmo::SCALE:
		//snap = config.mSnapScale;
		ImGui::InputFloat("Scale Snap", &snap.x);
		break;
	}
	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	ImGuizmo::Manipulate(camera->view_matrix.m, camera->projection_matrix.m, mCurrentGizmoOperation, mCurrentGizmoMode, matrix.m, NULL, useSnap ? &snap.x : NULL);
	#endif
}


//called to render the GUI from
void Application::renderDebugGUI(void)
{
#ifndef SKIP_IMGUI //to block this code from compiling if we want

	//System stats
	ImGui::Text(getGPUStats().c_str());					   // Display some text (you can use a format strings too)

	//Changing quality
	bool changed_quality = false;
	changed_quality |= ImGui::Combo("Quality Settings", (int*)&quality, "LOW\0MEDUIM\0HIGH\0ULTRA", 4);
	if (changed_quality) {
		if (renderer->render_mode == GTR::DEFERRED)
			renderer->lights.push_back(renderer->directional_light);

		for (int i = 0; i < renderer->lights.size(); ++i)
		{
			if (renderer->lights[i]->light_type == GTR::SPOT || renderer->lights[i]->light_type == GTR::DIRECTIONAL)
			{
				renderer->lights[i]->shadow_fbo->~FBO();
				renderer->lights[i]->shadow_fbo = new FBO();
				int res = 1024 * pow(2, (int)quality);
				renderer->lights[i]->shadow_fbo->setDepthOnly(res, res);
			}
		}

		if (renderer->atlas) {
			renderer->atlas->~FBO();
			renderer->atlas = NULL;
		}
	}

	//Chaning render_mode
	bool changed_render_mode = false;
	changed_render_mode |= ImGui::Combo("Render Mode", (int*)&renderer->render_mode, "FORWARD\0DEFERRED", 2);
	if (changed_render_mode) {
		if (renderer->render_mode == GTR::DEFERRED)
			renderer->light_mode = GTR::MULTI;
	}

	if (renderer->render_mode == GTR::FORWARD)
	{
		//Changing light_mode
		bool changed_light_mode = false;
		changed_light_mode |= ImGui::Combo("Light Mode", (int*)&renderer->light_mode, "SINGLE\0MULTI", 2);
		if (changed_light_mode)
		{
			if (renderer->light_mode == GTR::MULTI || renderer->atlas)
			{
				renderer->atlas->~FBO();
				renderer->atlas = NULL;
			}
		}
	}

	//Changing light_eq
	bool changed_light_eq = false;
	changed_light_eq |= ImGui::Combo("Light Equation", (int*)&renderer->light_eq, "PHONG\0DIRECT_LAMB\0DIRECT_BURLEY", 3);


	ImGui::SliderFloat("Bloom Threshold", &renderer->bloom_th, 0.0f, 10.0f);
	ImGui::SliderFloat("Bloom Soft Threshold", &renderer->bloom_soft_th, 0.0f, 1.0f);
	ImGui::SliderInt("Blur Iterations", &renderer->blur_iterations, 0, 15);
	ImGui::SliderFloat("Minimum DOF distance", &renderer->min_dist_dof, 0, renderer->max_dist_dof);
	ImGui::SliderFloat("Maximum DOF distance", &renderer->max_dist_dof, renderer->min_dist_dof, 1000);
	ImGui::SliderFloat("Focal distance", &renderer->focal_dist, 0, 5000);
	ImGui::SliderFloat("Grain", &renderer->noise_amount, 0.0f, 1.0f);
	ImGui::SliderFloat("Lens Distortion", &renderer->lens_dist, 0.0f, 1.0f);

	//Enabling depth viewport
	ImGui::Checkbox("Depth Viewport", &renderer->depth_viewport);
	if (renderer->depth_viewport) //We have to convert it to char
	{
		std::string depth_view = "Depth Buffer of light: " + renderer->lights[renderer->depth_light]->name;
		char* char_arr;
		char_arr = &depth_view[0];
		ImGui::Text(char_arr);
	}else{ ImGui::Text("No Depth Buffer light selected for viewport"); }

	//Enabling PCF
	ImGui::Checkbox("PCF", &renderer->pcf);

	//Enabling HDR
	ImGui::Checkbox("HDR", &renderer->hdr_active);
	if (renderer->hdr_active)
	{
		ImGui::SliderFloat("HDR Scale", &renderer->hdr_scale, 0.1f, 5.0f);
		ImGui::SliderFloat("HDR Average Luminance", &renderer->hdr_average_lum, 0.1f, 50.0f);
		ImGui::SliderFloat("HDR White Balance", &renderer->hdr_white_balance, 0.1f, 50.0f);
		ImGui::SliderFloat("HDR Gamma Correction", &renderer->hdr_gamma, 0.25f, 2.5f);
	}

	//Enabling SSAO
	if (renderer->render_mode == GTR::DEFERRED)
	{
		ImGui::Checkbox("dithering", &renderer->dithering);
		ImGui::Checkbox("SSAO", &renderer->activate_ssao);
		if (renderer->activate_ssao)
		{
			bool changed_ssao_plus = false;
			changed_ssao_plus |= ImGui::Checkbox("SSAO+", &renderer->ssao->plus);

			if (renderer->ssao->plus)
				ImGui::Checkbox("SSAO Blur", &renderer->ssao->blur);

			ImGui::SliderFloat("SSAO Factor", &renderer->ssao->intensity, 0.1f, 10.0f);

			bool changed_ssao_samples = false;
			changed_ssao_samples |= ImGui::SliderInt("SSAO Samples", &renderer->ssao->samples, 1, 500);

			ImGui::SliderFloat("SSAO Bias", &renderer->ssao->bias, 0.001, 0.1);

			if (changed_ssao_plus || changed_ssao_samples)
				renderer->ssao->points = generateSpherePoints(renderer->ssao->samples, 1.0f, renderer->ssao->plus);

			if (!renderer->ssao->plus)
				renderer->ssao->blur = false;
		}

		bool changed_irradiance = false;
		changed_irradiance |= ImGui::Checkbox("Irradiance", &renderer->activate_irr);
		if (renderer->activate_irr)
		{
			//update probes if they have never been updated
			if (!renderer->probes_texture || changed_irradiance)
				renderer->updateProbes(scene);
			ImGui::Checkbox("Trilinear", &renderer->irr_3lerp);
			ImGui::Checkbox("Show Irradiance Probes", &renderer->show_probes);
		}

		bool changed_reflections = false;
		changed_reflections |= ImGui::Checkbox("Reflections", &renderer->reflections);
		if (renderer->reflections)
		{
			if (changed_reflections)
			{
				renderer->reflections_calculated = true;
				renderer->updateReflectionProbes(scene);
			}
			ImGui::Checkbox("Reflection Probes", &renderer->show_reflection_probes);

		}

		ImGui::Checkbox("Volumetric Directional Light", &renderer->volumetric);
		if (renderer->volumetric)
		{
			ImGui::SliderFloat("Air Density", &renderer->air_density, 0.001f, 0.005f);
		}
	}

	ImGui::Checkbox("Wireframe", &render_wireframe);
	ImGui::ColorEdit4("BG color", scene->background_color.v);
	ImGui::ColorEdit4("Ambient Light", scene->ambient_light.v);

	//add info to the debug panel about the camera
	if (ImGui::TreeNode(camera, "Camera")) {
		camera->renderInMenu();
		ImGui::TreePop();
	}

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.75f, 0.75f, 1.0f));

	//example to show prefab info: first param must be unique!
	for (int i = 0; i < scene->entities.size(); ++i)
	{
		GTR::BaseEntity* entity = scene->entities[i];

		if(selected_entity == entity)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.0f));

		if (ImGui::TreeNode(entity, entity->name.c_str()))
		{
			entity->renderInMenu();
			ImGui::TreePop();
		}

		if (selected_entity == entity)
			ImGui::PopStyleColor();

		if (ImGui::IsItemClicked(0))
			selected_entity = entity;
	}

	ImGui::PopStyleColor();
#endif
}

//Keyboard event handler (sync input)
void Application::onKeyDown( SDL_KeyboardEvent event )
{
	switch(event.keysym.sym)
	{
		case SDLK_ESCAPE: must_exit = true; break; //ESC key, kill the app
		case SDLK_F1: render_debug = !render_debug; break;
		case SDLK_f: camera->center.set(0, 0, 0); camera->updateViewMatrix(); break;
		case SDLK_F5: Shader::ReloadAll(); break;
		case SDLK_1: if (renderer->lights.size() > 0) { renderer->depth_light = (renderer->depth_light + 1) % renderer->lights.size(); } break; //Changing the light selected for the depth viewport
		case SDLK_F6: scene->clear(); scene->load(scene->filename.c_str()); selected_entity = NULL;  break;
		case SDLK_2: renderer->show_gbuffers = (renderer->show_gbuffers + 1) % 2; break;
		case SDLK_3: renderer->show_omr = !renderer->show_omr; break;
		case SDLK_4: renderer->updateProbes(scene); break;
		case SDLK_5: renderer->updateReflectionProbes(scene); break;
	}
}

void Application::onKeyUp(SDL_KeyboardEvent event)
{
}

void Application::onGamepadButtonDown(SDL_JoyButtonEvent event)
{

}

void Application::onGamepadButtonUp(SDL_JoyButtonEvent event)
{

}

void Application::onMouseButtonDown( SDL_MouseButtonEvent event )
{
	if (event.button == SDL_BUTTON_MIDDLE) //middle mouse
	{
		//Input::centerMouse();
		mouse_locked = !mouse_locked;
		SDL_ShowCursor(!mouse_locked);
	}
}

void Application::onMouseButtonUp(SDL_MouseButtonEvent event)
{
}

void Application::onMouseWheel(SDL_MouseWheelEvent event)
{
	bool mouse_blocked = false;

	#ifndef SKIP_IMGUI
		ImGuiIO& io = ImGui::GetIO();
		if(!mouse_locked)
		switch (event.type)
		{
			case SDL_MOUSEWHEEL:
			{
				if (event.x > 0) io.MouseWheelH += 1;
				if (event.x < 0) io.MouseWheelH -= 1;
				if (event.y > 0) io.MouseWheel += 1;
				if (event.y < 0) io.MouseWheel -= 1;
			}
		}
		mouse_blocked = ImGui::IsAnyWindowHovered();
	#endif

	if (!mouse_blocked && event.y)
	{
		if (mouse_locked)
			cam_speed *= 1 + (event.y * 0.1);
		else
			camera->changeDistance(event.y * 0.5);
	}
}

void Application::onResize(int width, int height)
{
    std::cout << "window resized: " << width << "," << height << std::endl;
	glViewport( 0,0, width, height );
	camera->aspect =  width / (float)height;
	window_width = width;
	window_height = height;
	change_res == true;

	if (renderer->gbuffers_fbo->fbo_id != 0) {
		renderer->gbuffers_fbo->~FBO();
		renderer->gbuffers_fbo = new FBO();

		Texture* albedo = new Texture(window_width, window_height, GL_RGBA, GL_HALF_FLOAT);
		Texture* normals = new Texture(window_width, window_height, GL_RGBA, GL_UNSIGNED_BYTE);
		Texture* extra = new Texture(window_width, window_height, GL_RGBA, GL_HALF_FLOAT);
		Texture* irradiance = new Texture(window_width, window_height, GL_RGB, GL_HALF_FLOAT);
		Texture* depth = new Texture(window_width, window_height, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, false);

		std::vector<Texture*> text = { albedo,normals,extra,irradiance };

		renderer->gbuffers_fbo->setTextures(text, depth);
	}

	renderer->illumination_fbo->~FBO();
	renderer->illumination_fbo = new FBO();
	renderer->illumination_fbo->create(window_width, window_height, 1, GL_RGBA, GL_HALF_FLOAT, true);

	Texture* ssao_texture = new Texture(window_width * 0.5, window_height * 0.5, GL_LUMINANCE, GL_UNSIGNED_BYTE);
	Texture* ssao_texture_blur = new Texture(window_width * 0.5, window_height * 0.5, GL_LUMINANCE, GL_UNSIGNED_BYTE);
	std::vector<Texture*> textures = { ssao_texture, ssao_texture_blur };

	renderer->illumination_fbo_blurred->~FBO();
	renderer->illumination_fbo_blurred = new FBO();
	renderer->illumination_fbo_blurred->create(Application::instance->window_width, Application::instance->window_height,
		1,            //one textures
		GL_RGBA,       //four channels
		GL_FLOAT,//half float
		false);        //add depth_texture)

	renderer->reflections_fbo->~FBO();
	renderer->reflections_fbo = new FBO();
	renderer->reflections_fbo->create(Application::instance->window_width, Application::instance->window_height,
		1,            //one textures
		GL_RGB,       //four channels
		GL_UNSIGNED_BYTE,//half float
		false);

	renderer->bloom_fbo->~FBO();
	renderer->bloom_fbo = new FBO();
	renderer->bloom_fbo->create(Application::instance->window_width, Application::instance->window_height,
		1,            //one textures
		GL_RGBA,       //four channels
		GL_FLOAT,//half float
		false);

	renderer->ping->~Texture();
	renderer->pong->~Texture();
	renderer->ping = new Texture(Application::instance->window_width, Application::instance->window_height, GL_RGBA, GL_FLOAT);
	renderer->pong = new Texture(Application::instance->window_width, Application::instance->window_height, GL_RGBA, GL_FLOAT);

	renderer->decals_fbo->~FBO();
	Texture* albedo_decals = new Texture(Application::instance->window_width, Application::instance->window_height, GL_RGBA, GL_FLOAT);
	Texture* normals_decals = new Texture(Application::instance->window_width, Application::instance->window_height, GL_RGBA, GL_UNSIGNED_BYTE);
	Texture* extra_decals = new Texture(Application::instance->window_width, Application::instance->window_height, GL_RGBA, GL_HALF_FLOAT);
	std::vector<Texture*> text_decals = { albedo_decals,normals_decals,extra_decals };
	renderer->decals_fbo->setTextures(text_decals);
}

