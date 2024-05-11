#pragma once
#include "Plane.h"

class AquariumPane : public Plane
{

public:
	AquariumPane(float len, float wid, const glm::vec3& pos, TEXSCALE ts = TS_NO_SCALE, ORIENTATION ori = OR_XZ) :
		Plane(len, wid, pos, TS_NO_SCALE, ori)
	{}
};