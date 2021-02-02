#include <cstdint>
#include <imgui\imgui.h>
#include "imgui_impl_sdl_gl3.h"
#include <GL\glew.h>  
#include <SDL\SDL.h>
#include <chrono>
#include <cmath>
#include "tempARC.h"

#undef main // undef to remove sdl_main

#pragma once
class VolumeData;
class Editor {
public:
	Editor(uint32_t width, uint32_t height);
	~Editor();
	bool Initialize();
	void Run();
	

private:
	// Volume renderer functionality
	void UpdateTexture(const void* buffer, const int width, const int height);
	void Process();

	// UI
	void ControlPanel(uint32_t width, uint32_t height);
	void Scene(uint32_t width, uint32_t height);
	// SDL event related functions
	void OnResize(uint32_t width, uint32_t height);
	void HandleSDLEvent(SDL_Event* event);
	// SDL & window
	SDL_Window* m_window;
	SDL_GLContext m_context;
	uint32_t m_width, m_height;
	// Status
	bool m_isRunning;
	// Output texture
	bool m_hasTexture;
	GLuint m_textureID;

};


class Vector3 //////////////////////////////
{
public:
	Vector3() = default;
	Vector3(float a);
	Vector3(float x, float y, float z);
	Vector3(const Vector3& V);
	~Vector3() = default;

	Vector3 operator- (void) const;
	void operator= (const Vector3& V);

	Vector3 operator+ (const Vector3& V) const;
	Vector3 operator- (const Vector3& V) const;
	Vector3 operator* (float k) const;
	Vector3 operator* (const Vector3& V) const;
	Vector3 operator/ (const Vector3& V) const;
	Vector3 operator/ (float k) const;

	Vector3& operator+= (const Vector3& V);
	Vector3& operator-= (const Vector3& V);
	Vector3& operator/= (float k);
	Vector3& operator*= (float k);

	friend Vector3 operator* (float k, const Vector3& V) {
		return V * k;
	}
	friend Vector3 operator/ (float k, const Vector3& V) {
		return Vector3(k / V.x, k / V.y, k / V.z);
	}

	float length() const;
	float length2() const;

	float x, y, z;
};

inline float dot(const Vector3& A, const Vector3& B) {
	return A.x * B.x + A.y * B.y + A.z * B.z;
}

inline Vector3 cross(const Vector3& A, const Vector3& B) {
	return Vector3(A.y * B.z - A.z * B.y,
		A.z * B.x - A.x * B.z,
		A.x * B.y - A.y * B.x
	);
}


