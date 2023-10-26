#ifndef SCENE_H
#define SCENE_H

#include "framework.h"
#include <string>
#include "fbo.h"
#include "camera.h"
#include "sphericalharmonics.h"

//forward declaration
class cJSON; 
class HDRE;


//our namespace
namespace GTR {

	enum eEntityType {
		NONE = 0,
		PREFAB = 1,
		LIGHT = 2,
		CAMERA = 3,
		IRRADIANCE_GRID = 4,
		PROBE = 5,
		DECAL = 6,
		REFLECTION_PROBE = 7
	};

	enum eLightType {
		POINT = 0,
		SPOT = 1,
		DIRECTIONAL = 2,
		NO_LIGHT
	};

	class Scene;
	class Prefab;

	//represents one element of the scene (could be lights, prefabs, cameras, etc)
	class BaseEntity
	{
	public:
		Scene* scene;
		std::string name;
		eEntityType entity_type;
		Matrix44 model;
		bool visible;

		BaseEntity() { entity_type = NONE; visible = true; }
		virtual ~BaseEntity() {}
		virtual void renderInMenu();
		virtual void configure(cJSON* json) {}
	};

	//represents one prefab in the scene
	class PrefabEntity : public GTR::BaseEntity
	{
	public:
		std::string filename;
		Prefab* prefab;
		
		PrefabEntity();
		virtual void renderInMenu();
		virtual void configure(cJSON* json);
	};

	class ProbeEntity : public GTR::BaseEntity 
	{
	public:
		Vector3 local; //its ijk pos in the matrix
		int index; //its index in the linear array
		SphericalHarmonics sh; //coeffs

		ProbeEntity();
		virtual void renderInMenu();
		virtual void configure(cJSON* json);

	};

	class IrradianceGrid : public GTR::BaseEntity
	{
	public:
		std::vector<ProbeEntity*> probes;

		Matrix44 inv_model;
		Vector3 dim; //number of points in each axis of the grid
		int probe_scale;
		

		IrradianceGrid();
		virtual void renderInMenu();
		virtual void configure(cJSON* json);
		void updateProbe(ProbeEntity* p);
		void updateGrid();
	};

	class ReflectionProbeEntity : public GTR::BaseEntity
	{
	public:
		Texture* cubemap;

		ReflectionProbeEntity();
		virtual void renderInMenu();
		virtual void configure(cJSON* json);

	};

	class LightEntity : public GTR::BaseEntity
	{
	public:
		Vector3 color;
		float intensity;
		eLightType light_type;
		float max_distance;
		float cone_angle;
		float area_size;
		float spot_exp;
		float bias;
		bool cast_shadows;
		Vector3 uvs; // to know where to read in atlas (xy = uv, z = lenght)

		Camera* camera;
		FBO* shadow_fbo;

		LightEntity();
		virtual void renderInMenu();
		virtual void configure(cJSON* json);
		bool lightBounding(Camera* camera);
		void uploadLightParams(Shader* sh, bool linearize, float& hdr_gamma);
	};

	class DecalEntity : public GTR::BaseEntity
	{
	public:
		Texture* albedo;

		DecalEntity();
		virtual void configure(cJSON* json);
	};

	//contains all entities of the scene
	class Scene
	{
	public:
		static Scene* instance;

		Vector3 background_color;
		Vector3 ambient_light;
		Camera main_camera;

		Texture* environment;

		Scene();

		std::string filename;
		std::vector<BaseEntity*> entities;

		void clear();
		void addEntity(BaseEntity* entity);

		bool load(const char* filename);
		BaseEntity* createEntity(std::string type);
	};

};

#endif