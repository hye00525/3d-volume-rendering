#include "tempARC.h"
#include <algorithm>
#include <iostream>

//code for Arcball

using namespace std;

tempARC::tempARC(int width, int height,int depth) {  // sphere coord »ý°¢

	radius = std::min(width, height) / 2.0f;
	center = glm::vec3(width / 2.0, height / 2.0, depth / 2.0); // same with volume center

	pre = glm::quat(1, 0, 0, 0);
	now = glm::quat(1, 0, 0, 0);

}

glm::mat4 tempARC::Rotation_Mat4() {
	return glm::mat4_cast(now); //get rotation for ray casting
}

void tempARC::Reset(int width, int height, int depth) { //space bar -> reset

	radius = std::min(width, height) / 2.0f;
	center = glm::vec3(width / 2.0, height / 2.0, depth / 2.0);
	
	pre = glm::quat(1, 0, 0, 0);
	now = glm::quat(1, 0, 0, 0);

}

void tempARC::MouseDown(int x, int y) { 
	x = x - 384;
	y = y - 104;

	XY_COORD Scr;
	Scr.x = x; Scr.y = y;


	float Arc_x = (x - center.x) / radius;
	float Arc_y = (y - center.y) / radius;
	float Arc_z = 0.0f;

	float Sqared = (Arc_x * Arc_x) + (Arc_y * Arc_y);

	if (Sqared > 1.0f) {
		Arc_x = Arc_x * (float)(1.0f / sqrt(Sqared));
		Arc_y = Arc_y * (float)(1.0f / sqrt(Sqared));
		Arc_z = 0.0f;
	}
	else {
		Arc_z = sqrt(1.0f - Sqared);
	}
	origin.x = Arc_x;
	origin.y = Arc_y;
	origin.z = Arc_z;

	pre = now;
}

void tempARC::DragMotion(int cur_x, int cur_y) { //rotation
	cur_x = cur_x - 384;
	cur_y = cur_y - 104;
	XY_COORD cur_screen;
	cur_screen.x = cur_x; cur_screen.y = cur_y;

	XYZ_COORD prev, cur;


	float Arc_x = (cur_x - center.x) / radius;
	float Arc_y = (cur_y - center.y) / radius;
	float Arc_z = 0.0f;

	float Sqared = (Arc_x * Arc_x) + (Arc_y * Arc_y);

	if (Sqared > 1.0f) {
		Arc_x = Arc_x * (float)(1.0f / sqrt(Sqared));
		Arc_y = Arc_y * (float)(1.0f / sqrt(Sqared));
		Arc_z = 0.0f;
	}
	else {
		Arc_z = sqrt(1.0f - Sqared);
	}

	cur.x = Arc_x;
	cur.y = Arc_y;
	cur.z = Arc_z;

	prev = origin;

	//printf("\n Rotation %f %f %f to %f %f %f \n ", prev.x, prev.y, prev.z, cur.x, cur.y, cur.z);

	glm::vec3 prev3 = glm::normalize(glm::vec3(prev.x, prev.y, prev.z));
	glm::vec3 cur3 = glm::normalize(glm::vec3(cur.x, cur.y, cur.z));

	float Product = glm::dot(prev3, cur3);
	float angleAxis = 2 * acos(std::min(1.0f, Product));


	glm::vec3 cross = glm::cross(prev3, cur3);
	cross = glm::normalize(cross);


	glm::quat rotation;
	rotation = glm::angleAxis(angleAxis, -cross);

	now = rotation * pre;

	printf("\n Rotation %f %f %f to %f %f %f \n ", prev3.x, prev3.y, prev3.z, cur3.x, cur3.y, cur3.z);

}

