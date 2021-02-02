#ifndef __TEMPARC_H__
#define __TEMPARC_H__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/common.hpp>

typedef struct XY_COORD {
	float x, y;
}XY_COORD;

typedef struct XYZ_COORD {
	float x, y, z;
}XYZ_COORD;

class tempARC {
public:
	tempARC(int, int,int);
	void MouseDown(int, int); //click
	void DragMotion(int, int); //drag
	void Reset(int, int,int); //spacebar -> reset

	glm::mat4 Rotation_Mat4(); //get rotation
	bool mouse_clicked = false;

	glm::vec3 center;
	float radius;

	glm::quat pre;
	glm::quat now;

	XYZ_COORD origin;

};

#endif // __TEMPARC_H__