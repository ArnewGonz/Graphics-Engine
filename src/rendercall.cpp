#include "rendercall.h"

#include <iostream>
#include <algorithm>
#include <vector>

using namespace GTR;

RenderCall::RenderCall(Mesh* mesh, Material* material, Matrix44& model)
{
	this->mesh = mesh;
	this->material = material;
	this->model = model;
	probe = NULL;

	cam_dist = 0;
}