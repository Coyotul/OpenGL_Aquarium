#pragma once

#include <iostream>
#include <fstream>
#include <sstream>

#include "Mesh.h"
#include "Vertex.h"

enum ORIENTATION
{
	OR_XZ,
	OR_XY,
	OR_ZY
};

class Plane : public Mesh {
protected:
	float length;
	float width;

	void initCommon();
	void init(float len, float wid, const glm::vec3& pos, TEXSCALE ts, ORIENTATION ori);

	void init(glm::vec3 fs, glm::vec3 sc, TEXSCALE ts);

public:
	Plane(float len, float wid, const glm::vec3& pos, TEXSCALE ts = TS_NO_SCALE, ORIENTATION ori = OR_XZ)
	{
		init(len, wid, pos, ts, ori);
	}

	Plane(glm::vec3 fs, glm::vec3 sc, TEXSCALE ts)
	{
		init(fs, sc, ts);
	}

	void updateBillboardRotation(glm::vec3 right, glm::vec3 up);
};
