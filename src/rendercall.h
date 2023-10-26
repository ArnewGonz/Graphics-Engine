#pragma once
#include "framework.h"
#include "prefab.h"
#include "material.h"
#include "camera.h"

namespace GTR {

	class RenderCall
	{
	public:
		Mesh* mesh;
		Material* material;
		Matrix44 model;
		ReflectionProbeEntity* probe;

		float cam_dist;

		RenderCall(Mesh* mesh, Material* material, Matrix44& model);

		bool operator < (RenderCall& b) const
		{
			if (material->alpha_mode != b.material->alpha_mode)
				return material->alpha_mode <= b.material->alpha_mode;
			else {
				return cam_dist > b.cam_dist;
			}
		}
	};
}
