#include "scene.h"
#include "utils.h"
#include "application.h"
#include "shader.h"

#include "prefab.h"
#include "extra/cJSON.h"

GTR::Scene* GTR::Scene::instance = NULL;

GTR::Scene::Scene()
{
	instance = this;
}

void GTR::Scene::clear()
{
	for (int i = 0; i < entities.size(); ++i)
	{
		BaseEntity* ent = entities[i];
		delete ent;
	}
	entities.resize(0);
}


void GTR::Scene::addEntity(BaseEntity* entity)
{
	entities.push_back(entity); entity->scene = this;
}

bool GTR::Scene::load(const char* filename)
{
	std::string content;

	this->filename = filename;
	std::cout << " + Reading scene JSON: " << filename << "..." << std::endl;

	if (!readFile(filename, content))
	{
		std::cout << "- ERROR: Scene file not found: " << filename << std::endl;
		return false;
	}

	//parse json string 
	cJSON* json = cJSON_Parse(content.c_str());
	if (!json)
	{
		std::cout << "ERROR: Scene JSON has errors: " << filename << std::endl;
		return false;
	}

	//read global properties
	background_color = readJSONVector3(json, "background_color", background_color);
	ambient_light = readJSONVector3(json, "ambient_light", ambient_light );
	main_camera.eye = readJSONVector3(json, "camera_position", main_camera.eye);
	main_camera.center = readJSONVector3(json, "camera_target", main_camera.center);
	main_camera.fov = readJSONNumber(json, "camera_fov", main_camera.fov);

	//entities
	cJSON* entities_json = cJSON_GetObjectItemCaseSensitive(json, "entities");
	cJSON* entity_json;
	cJSON_ArrayForEach(entity_json, entities_json)
	{
		std::string type_str = cJSON_GetObjectItem(entity_json, "type")->valuestring;
		BaseEntity* ent = createEntity(type_str);
		if (!ent)
		{
			std::cout << " - ENTITY TYPE UNKNOWN: " << type_str << std::endl;
			//continue;
			ent = new BaseEntity();
		}

		addEntity(ent);

		if (cJSON_GetObjectItem(entity_json, "name"))
		{
			ent->name = cJSON_GetObjectItem(entity_json, "name")->valuestring;
			stdlog(std::string(" + entity: ") + ent->name);
		}

		//read transform
		if (cJSON_GetObjectItem(entity_json, "position"))
		{
			ent->model.setIdentity();
			Vector3 position = readJSONVector3(entity_json, "position", Vector3());
			ent->model.translate(position.x, position.y, position.z);
		}

		if (cJSON_GetObjectItem(entity_json, "angle"))
		{
			float angle = cJSON_GetObjectItem(entity_json, "angle")->valuedouble;
			ent->model.rotate(angle * DEG2RAD, Vector3(0, 1, 0));
		}

		if (cJSON_GetObjectItem(entity_json, "rotation"))
		{
			Vector4 rotation = readJSONVector4(entity_json, "rotation");
			Quaternion q(rotation.x, rotation.y, rotation.z, rotation.w);
			Matrix44 R;
			q.toMatrix(R);
			ent->model = R * ent->model;
		}

		if (cJSON_GetObjectItem(entity_json, "scale"))
		{
			Vector3 scale = readJSONVector3(entity_json, "scale", Vector3(1, 1, 1));
			ent->model.scale(scale.x, scale.y, scale.z);
		}

		ent->configure(entity_json);
	}

	//free memory
	cJSON_Delete(json);

	for (int i = 0; i < 20; ++i)
	{
		LightEntity* light = new LightEntity();

		int x = rand() % (1000 - (-1000) + 1) + (-1000);
		int y = rand() % (30 + 1);
		int z = rand() % (1000 - (-1000) + 1) + (-1000);
		light->model.translate(x, y, z);

		float r = clamp(((double)rand() / (RAND_MAX)) + 0.1, 0.0, 1.0);
		float g = clamp(((double)rand() / (RAND_MAX)) + 0.1, 0.0, 1.0);
		float b = clamp(((double)rand() / (RAND_MAX)) + 0.1, 0.0, 1.0);
		light->color = Vector3(r,g,b);
	
		light->cone_angle = 0.0f;
		light->intensity = 12.5f;
		light->max_distance = 70;
		light->light_type = POINT;
		light->name = "pointlight" + std::to_string(i + 3);

		light->cast_shadows = false;
		light->shadow_fbo = NULL;

		addEntity(light);
	}

	return true;
}

GTR::BaseEntity* GTR::Scene::createEntity(std::string type)
{
	if (type == "PREFAB")
		return new GTR::PrefabEntity();
	if (type == "LIGHT")
		return new GTR::LightEntity();
	if (type == "PROBE")
		return new GTR::ProbeEntity();
	if (type == "REFLECTION_PROBE")
		return new GTR::ReflectionProbeEntity();
	if (type == "IRRADIANCE_GRID")
		return new GTR::IrradianceGrid();
	if (type == "DECAL")
		return new GTR::DecalEntity();
    return NULL;
}

void GTR::BaseEntity::renderInMenu()
{
#ifndef SKIP_IMGUI
	ImGui::Text("Name: %s", name.c_str()); // Edit 3 floats representing a color
	ImGui::Checkbox("Visible", &visible); // Edit 3 floats representing a color
	//Model edit
	ImGuiMatrix44(model, "Model");
	
#endif
}


GTR::PrefabEntity::PrefabEntity()
{
	entity_type = PREFAB;
	prefab = NULL;
}

void GTR::PrefabEntity::configure(cJSON* json)
{
	if (cJSON_GetObjectItem(json, "filename"))
	{
		filename = cJSON_GetObjectItem(json, "filename")->valuestring;
		prefab = GTR::Prefab::Get( (std::string("data/") + filename).c_str());
	}
}

void GTR::PrefabEntity::renderInMenu()
{
	BaseEntity::renderInMenu();

#ifndef SKIP_IMGUI
	ImGui::Text("filename: %s", filename.c_str()); // Edit 3 floats representing a color
	if (prefab && ImGui::TreeNode(prefab, "Prefab Info"))
	{
		prefab->root.renderInMenu();
		ImGui::TreePop();
	}
#endif
}

GTR::ProbeEntity::ProbeEntity() 
{
	entity_type = PROBE;
	local = Vector3(); //its ijk pos in the matrix
	index = 0;
	//sh = SphericalHarmonics();
}

void GTR::ProbeEntity::configure(cJSON* json)
{

}

void GTR::ProbeEntity::renderInMenu()
{
	BaseEntity::renderInMenu();
}

GTR::IrradianceGrid::IrradianceGrid()
{
	entity_type = IRRADIANCE_GRID;
	inv_model = model;
	inv_model.inverse();
}

void GTR::IrradianceGrid::updateGrid()
{
	inv_model = model;
	inv_model.inverse();
}

void GTR::IrradianceGrid::updateProbe(ProbeEntity* p)
{ 
	Vector3 global = model * p->local;
	p->model.setTranslation(global.x, global.y, global.z);
	p->model.scale(probe_scale, probe_scale, probe_scale);
}

void GTR::IrradianceGrid::configure(cJSON* json)
{
	dim = readJSONVector3(json, "dim", Vector3());
	probe_scale = readJSONNumber(json, "probe_scale", 5);
	//add probes to std::vector (indexes already ordered)
	for (int k = 0; k < dim.z; ++k)
		for (int j = 0; j < dim.y; ++j)
			for (int i = 0; i < dim.x; ++i) {
				ProbeEntity* p = new ProbeEntity();
				Vector3 local = Vector3(i/dim.x, j/dim.y, k/dim.z);
				p->local = local;
				p->index = i + dim.x * (j + dim.y * k);
				updateProbe(p);
				probes.push_back(p);
			}
	//for (int i = 0; i < probes.size(); ++i)
	//{
	//	std::cout << probes[i]->index << " ";
	//}
}

void GTR::IrradianceGrid::renderInMenu()
{
	BaseEntity::renderInMenu();
}


GTR::LightEntity::LightEntity()
{
	entity_type = LIGHT;

	camera = new Camera();
	cast_shadows = false;
	uvs = Vector3();

	shadow_fbo = NULL;
}

void GTR::LightEntity::configure(cJSON* json)
{
	if (cJSON_GetObjectItem(json, "light_type"))
	{
		std::string type_str = cJSON_GetObjectItem(json, "light_type")->valuestring;
		if (type_str == "POINT") {
			light_type = POINT;
		}
		if (type_str == "SPOT") {
			light_type = SPOT;
			bias = 0.03;

			shadow_fbo = new FBO();
			int res = 1024 * pow(2, (int)Application::instance->quality);
			shadow_fbo->setDepthOnly(res, res);
		}
		if (type_str == "DIRECTIONAL") {
			light_type = DIRECTIONAL;
			bias = 0.005;

			shadow_fbo = new FBO();
			int res = 1024 * pow(2, (int)Application::instance->quality);
			shadow_fbo->setDepthOnly(res, res);
		}
	}
	if (cJSON_GetObjectItem(json, "color"))
	{
		color = color = readJSONVector3(json, "color", Vector3());
	}
	if (cJSON_GetObjectItem(json, "max_dist"))
	{
		max_distance = readJSONNumber(json, "max_dist", 0);
	}
	if (cJSON_GetObjectItem(json, "intensity"))
	{
		intensity= readJSONNumber(json, "intensity", 0);
	}
	if (cJSON_GetObjectItem(json, "target"))
	{
		Vector3 target = readJSONVector3(json, "target", Vector3());
		Vector3 pos = model.getTranslation();
		model.setFrontAndOrthonormalize(target - pos);
	}
	if (cJSON_GetObjectItem(json, "cone_angle"))
	{
		cone_angle = readJSONNumber(json, "cone_angle", 0);
	}
	if (cJSON_GetObjectItem(json, "area_size"))
	{
		area_size = readJSONNumber(json, "area_size", 0);
	}
	if (cJSON_GetObjectItem(json, "cone_exp"))
	{
		spot_exp = readJSONNumber(json, "cone_exp", 0);
	}
	if (cJSON_GetObjectItem(json, "shadow_bias"))
	{
		bias = readJSONNumber(json, "shadow_bias", 0);
	}
	if (cJSON_GetObjectItem(json, "cast_shadows"))
	{
		cast_shadows = readJSONBool(json, "cast_shadows", false);
	}
}

bool GTR::LightEntity::lightBounding(Camera* camera)
{
	Vector4 sphere;

	if (light_type == SPOT) {
		sphere = boundingSphere(model * Vector3(0, 0, 0), model.rotateVector(Vector3(0, 0, -1)), max_distance, cone_angle * PI / 180);
	}
	else {
		Vector3 pos = model * Vector3(0, 0, 0);
		sphere = Vector4(pos.x, pos.y, pos.z, max_distance);
	}

	return camera->testSphereInFrustum(sphere.xyz(), sphere.w);
}

void GTR::LightEntity::renderInMenu()
{
	BaseEntity::renderInMenu();

#ifndef SKIP_IMGUI
	if (light_type == DIRECTIONAL)
	{
		ImGui::Text("DIRECTIONAL LIGHT"); // Edit 3 floats representing a color
		ImGui::SliderFloat("Light area", &area_size, 0.0f, 5000.0f);
		ImGui::Checkbox("Cast Shadows", &cast_shadows);
	}
	if (light_type == SPOT)
	{
		ImGui::Text("SPOT LIGHT"); // Edit 3 floats representing a color
		ImGui::SliderFloat("Cone angle", &cone_angle, 1.0f, 80.0f);
		ImGui::SliderFloat("Spot Exponential", &spot_exp, 0.0f, 100.0f);
		ImGui::Checkbox("Cast Shadows", &cast_shadows);
	}
	if (light_type == POINT)
	{
		ImGui::Text("POINT LIGHT"); // Edit 3 floats representing a color
	}
	ImGui::ColorEdit4("Color", color.v); // Edit 4 floats representing a color + alpha
	ImGui::SliderFloat("Intensity", &intensity, 0.0f, 2000.0f);
	ImGui::SliderFloat("Bias", &bias, 0.0f, 0.1f);
	ImGui::SliderFloat("Max distance", &max_distance, 0.0f, 20000.0f);
#endif
}

void GTR::LightEntity::uploadLightParams(Shader* sh, bool linearize, float& hdr_gamma)
{
	if (cast_shadows) {
		//If shadows are enabled, pass the shadowmap
		Texture* shadowmap = shadow_fbo->depth_texture;
		sh->setTexture("shadowmap", shadowmap, 8);
		Matrix44 shadow_proj = camera->viewprojection_matrix;
		sh->setUniform("u_shadow_viewproj", shadow_proj);
		sh->setUniform("u_shadow_bias", bias);
	}

	if (linearize) {
		Vector3 l_color = Vector3(pow(color.x, hdr_gamma), pow(color.y, hdr_gamma), pow(color.z, hdr_gamma));
		sh->setVector3("u_light_color", l_color);
	}else{ sh->setVector3("u_light_color", color); }

	float cos_angle = cos(cone_angle * PI / 180);
	sh->setUniform("u_light_cutoff", cos_angle);
	sh->setUniform("u_light_exp", spot_exp);

	sh->setVector3("u_light_vector", model.frontVector());
	sh->setUniform("u_shadows", cast_shadows);
	sh->setVector3("u_light_position", model.getTranslation());
	sh->setUniform("u_light_maxdist", max_distance);
	sh->setUniform("u_light_type", (int)light_type);
	sh->setUniform("u_light_intensity", intensity);
}

GTR::ReflectionProbeEntity::ReflectionProbeEntity()
{
	entity_type = REFLECTION_PROBE;

	cubemap = new Texture();
	cubemap->createCubemap(
		512, 512,
		NULL,
		GL_RGB, GL_UNSIGNED_INT, false);
}

void GTR::ReflectionProbeEntity::renderInMenu()
{
	BaseEntity::renderInMenu();
}

void GTR::ReflectionProbeEntity::configure(cJSON* json)
{
}

GTR::DecalEntity::DecalEntity()
{
	entity_type = DECAL;
	albedo = NULL;
}

void GTR::DecalEntity::configure(cJSON* json)
{
	std::string filename = readJSONString(json, "albedo", "");
	if (filename.size())
		albedo = Texture::Get((std::string("data/") + filename).c_str());
}
