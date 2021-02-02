
#include "Editor.h"
#include "fixedcamera.h"
#include <iostream>
#include <regex>
#include <filesystem>
#include <glm\glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <vector>
#include <algorithm>
#include <excpt.h>

#include <time.h>
#include "tempARC.h"

using namespace std;

glm::vec4 RayCasting(int16_t* d_vol, glm::vec4 camera, glm::vec3 dir, int width, int height, int depth);
float getInterpolation(int16_t* d_vol, int width, int height, int depth, float x, float y, float z);
float min_val = 2, max_val = 0;
tempARC* tempArc;
char* waitStr = " \n";
double result;
bool MIP = false;

static bool isLoaded = false;

char* fileName = "";

const int v_width = 512;
const int v_height = 512;
int v_depth = 58;

Editor::Editor(uint32_t width, uint32_t height)

	:m_window(nullptr),
	m_context(nullptr),
	m_width(width),
	m_height(height),
	m_isRunning(true),
	m_hasTexture(false) {


}


Editor::~Editor() {
	ImGui_ImplSdlGL3_Shutdown();
	SDL_GL_DeleteContext(m_context);
	SDL_DestroyWindow(m_window);
	SDL_Quit();
}

bool Editor::Initialize() {
	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		std::cout << ("Error: %s\n", SDL_GetError()) << std::endl;
		return false;
	}

	// Setup window
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_DisplayMode current;
	SDL_GetCurrentDisplayMode(0, &current);
	m_window = SDL_CreateWindow("Volume Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_width, m_height, SDL_WINDOW_OPENGL);
	SDL_GLContext glcontext = SDL_GL_CreateContext(m_window);
	glewInit();


	tempArc = new tempARC(516, 516, v_depth);

	// Setup ImGui binding
	ImGui_ImplSdlGL3_Init(m_window);
	//Process();
	return true; // Return initialization result
}

void Editor::Run() {
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	while (m_isRunning) {
		// Handle SDL events
		SDL_Event event;
		if (SDL_PollEvent(&event)) {
			ImGui_ImplSdlGL3_ProcessEvent(&event);
			HandleSDLEvent(&event);
		}
		ImGui_ImplSdlGL3_NewFrame(m_window);
		// Editor
		{
			ControlPanel(m_width - 720, 720);
			Scene(720, 720);
			// Code sample of ImGui (Remove comment when you want to see it)
			ImGui::ShowTestWindow();
		}
		// Rendering
		glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui::Render();
		SDL_GL_SwapWindow(m_window);
	}
}


void Editor::UpdateTexture(const void* buffer, int width, int height) {
	if (!m_hasTexture) {
		auto err = glGetError();
		glGenTextures(1, &m_textureID);
		if (err != GL_NO_ERROR) {
			throw std::runtime_error("Not able to create texture from buffer" + std::to_string(glGetError()));
		}
		else {
			m_hasTexture = true;
		}
	}
	glBindTexture(GL_TEXTURE_2D, m_textureID);
	// set texture sampling methods
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //GL_NEAREST
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	glBindTexture(GL_TEXTURE_2D, 0);
	
}


void Editor::Process() { 	/* TODO : Process volume data & pass raw buffer to UpdateTexture method*/
	time_t start, end;
	const int width = 512;
	const int height = 512;
	//int depth = 58;
	int depth = v_depth;

	const int size = width * height * depth;

	start = time(NULL);

	int16_t* Volume = new int16_t[size];

	FILE* p_file = fopen(fileName, "rb");
	if (p_file == NULL) { cout << "File open error!!"; return; }

	if (p_file) {
		fread(Volume, sizeof(int16_t), size, p_file); //d_vol[(int)floor(z) * width * height + (int)floor(y) * height + (int)floor(x)];
		fclose(p_file);
	}

	unsigned char* buffer = new unsigned char[512 * 512 * 4];

	glm::vec4 now;
	glm::vec4 intensity;


	glm::vec4 screenOrigin = glm::vec4(width / 2.f, height / 2.f, 0, 1);
	glm::vec4 volumeOrigin(width / 2.f, height / 2.f, depth / 2.f, 1);


	glm::vec3 dir = screenOrigin - volumeOrigin; //vector direction

	dir = glm::normalize(dir);

	// RayCasting <<<512*512, 1 >>> (Volume, Now, transformedDir, 512, 512, 58) CUDA

	unsigned int idx = 0;
	for (int height = 0; height < 512; height++) { //start ray casting per pixel
		for (int width = 0; width < 512; width++) {

			now = glm::vec4(width, height, 512, 1); //calculate Translate & Rotation per pixel

			glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3(-256.0f, -256.0f, -depth/2.f)); //Translate to origin
			glm::vec4 transformedNow = translate * now;

			glm::mat4 Rotate = tempArc->Rotation_Mat4(); // get Rotation Mat from Arcball
			transformedNow = Rotate * transformedNow;
			glm::vec3 transformedDir = Rotate * glm::vec4(dir, 0);

			glm::mat4 translate2 = glm::translate(glm::mat4(1.0f), glm::vec3(256.0f, 256.0f, depth/2.f)); //Translate 
			glm::vec4 Now = translate2 * transformedNow;

			transformedDir = glm::normalize(transformedDir);
			intensity = RayCasting(Volume, Now, transformedDir, 512, 512, depth); //ray casting with traslated & rotated vector

			buffer[idx + 0] = intensity.x;
			buffer[idx + 1] = intensity.y;
			buffer[idx + 2] = intensity.z;
			buffer[idx + 3] = intensity.w;
			idx = idx + 4;

			//break;

		}//break;

	}

	UpdateTexture(buffer, 512, 512); //updata texture

	end = time(NULL);


	result = (double)(end - start);
	cout << "Process Time : " << (result/CLOCKS_PER_SEC) << '\n'; //processing time
}


glm::vec4 RayCasting(int16_t* d_vol, glm::vec4 cur, glm::vec3 dir, int width, int height, int depth) { 

	glm::vec4 bound1 = glm::vec4(-width, -height, -700, 1);
	glm::vec4 bound2 = glm::vec4(width * 2, height * 2, 700, 1);


	float step = 0.1f;
	float temp = 0.0f;
	glm::vec4 rgba;

	

	double cin = 0, cout = 0, csrc = 0, asrc = 0, ain = 0, aout = 0, val = 0;
	
	glm::vec4 now = cur;
	now.x += dir.x ;  now.y += dir.y ;  now.z += dir.z ;
	int count = 0;
	double max_Intensity = 0.0f;

	while (true) {

		if (now.x > bound2.x || now.x < bound1.x || now.y > bound2.y || now.y < bound1.y || now.z > bound2.z || now.z < bound1.z) {
			// ray termination 
			break;
		}

		float x = now.x; float y = now.y; float z = now.z;

		if (x < 0 || y < 0 || z < 0 || x >= width || y >= height || z >= depth) {
			now.x += dir.x ;  now.y += dir.y ;  now.z += dir.z ;
			// 안닿음
			continue;
		}

		float intensity = d_vol[(int)floor(z) * width * height + (int)floor(y) * height + (int)floor(x)]; // volume 에서 xyz 해당하는 intensity
		//float intensity = getInterpolation(d_vol, width, height, depth, now.x, now.y, now.z);
		//val = (intensity - INT16_MIN ) / (INT16_MAX - INT16_MIN); // normalize 0-1.... temp transfer func.. (linear)
		
		val = ((double)intensity * (double)intensity) / ((double)INT16_MIN * (double)INT16_MIN); //(squared) ...temp transfer func...
	
		
		if (max_Intensity < val) { // Max Intensity 
			max_Intensity = val;
		}


		if(val > 0 && val < 0.4) { csrc = val/100000; asrc = val / 5; }
		else if (val >= 0.4 && val < 0.7) { csrc = val  ; asrc = val/5 ; }
		else { csrc = val; asrc = val/3; }
	

		if (ain > 0.99999) {
			break;
		}
	
		cout = cin + (1 - ain) * csrc * asrc; 
		aout = ain + (1 - ain) * asrc;

		ain = aout;
		cin = cout;

		now.x += dir.x; now.y += dir.y; now.z += dir.z;
		count++;
		
	}
	 
	if (MIP) { //temp
		cout = round(max_Intensity*255);
		rgba = glm::vec4(cout, cout, cout, 255-count); }

	else {
		cout = cout * 255;
		rgba = glm::vec4(cout, cout, cout, 255); //front to back > compositing  
	}
	

	return rgba;
}


float getInterpolation(int16_t* d_vol, int width, int height, int depth, float x, float y, float z) {

	if (x <= 1 || y <= 1 || z <= 1 || x >= width - 1 || y >= height - 1 || z >= depth - 1) {
		return d_vol[(int)floor(z) * width * height + (int)floor(y) * height + (int)floor(x)];
	}
	// d_vol[(int)floor(z) * width * height + (int)floor(y) * height + (int)floor(x)];

	int x0 = (int)floor(x);
	int x1 = (int)ceil(x);

	int y0 = (int)floor(y);
	int y1 = (int)ceil(y);

	int z0 = (int)floor(z);
	int z1 = (int)ceil(z);


	double x_d = double(x - (double)x0);
	double y_d = double(y - (double)y0);
	double z_d = double(z - (double)z0);

	/// 3 D interpolation 
	double temp00 = d_vol[(int)z0 * width * height + (int)y0 * height + (int)x0] * (1 - x_d) + d_vol[(int)z0 * width * height + (int)y0 * height + (int)x1] * x_d; // 
	double temp01 = d_vol[(int)z1 * width * height + (int)y0 * height + (int)x0] * (1 - x_d) + d_vol[(int)z1 * width * height + (int)y0 * height + (int)x1] * x_d; // 
	double temp10 = d_vol[(int)z0 * width * height + (int)y1 * height + (int)x0] * (1 - x_d) + d_vol[(int)z0 * width * height + (int)y1 * height + (int)x1] * x_d; // 
	double temp11 = d_vol[(int)z1 * width * height + (int)y1 * height + (int)x0] * (1 - x_d) + d_vol[(int)z1 * width * height + (int)y1 * height + (int)x1] * x_d; // 


	double t0 = temp00 * (1 - y_d) + temp10 * y_d;
	double t1 = temp01 - (1 - y_d) + temp11 * y_d;

	double tmp = t0 * (1 - z_d) + t1 * z_d;

	float intensity = (float)tmp;

	//cout << intensity << '\n';
	return intensity;


}


void Editor::ControlPanel(uint32_t width, uint32_t height) {
	// Control Panel Window
	ImGui::SetNextWindowSize(ImVec2((float)width, (float)height));
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

	/* TODO : Write UI Functions */

	if (ImGui::Button("Volume 1 Load"))
	{
		isLoaded = true;
		fileName = "..\\asset\\data\\volume1.raw";
		std::cout << "Load Image 1...";
		v_depth = 56;
		Process();
		waitStr = "Load Image1...\n";
	}
	if (ImGui::Button("Volume 2 Load"))
	{
		isLoaded = true;
		fileName = "..\\asset\\data\\volume2.raw";
		std::cout << "Load Image 2...";
		v_depth = 58;
		Process();
		waitStr = "Load Image2...\n";
	}


	ImGui::Text("Processing Time %f sec\n\n", result);

	if (ImGui::Button("Reset Arcball"))
	{

		tempArc->Reset(512, 512, v_depth);
		Process();
		waitStr = "Reset Arcball...\n";

	}
	ImGui::Text("Width = %d\nHeight = %d\nDepth = %d\n\n", v_width,v_height,v_depth);

	if (ImGui::Button("Max Intensity"))
	{
		MIP = true;
		Process();
		waitStr = "Max Intensity...\n";

	}

	if (ImGui::Button("Transfer func"))
	{
		MIP = false;
		Process();
		waitStr = "Transfer func...\n";

	}
	ImGui::Text("\n\n%s", waitStr);
	ImGui::End();
}

void Editor::Scene(uint32_t width, uint32_t height) {
	// Scene Window
	ImGui::SetNextWindowSize(ImVec2((float)width, (float)height));
	ImGui::SetNextWindowPos(ImVec2((float)(m_width - width), 0.f));
	ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
	// Draw texture if there is one
	if (m_hasTexture) {
		ImGui::Image(ImTextureID(m_textureID), ImGui::GetContentRegionAvail());
		
	}
	ImGui::End();
}

void Editor::OnResize(uint32_t width, uint32_t height) {
	m_width = width;
	m_height = height;
}


void Editor::HandleSDLEvent(SDL_Event* event) {
	// SDL_Event wiki : https://wiki.libsdl.org/SDL_Event
	static bool mouseIsDown = false;
	static bool isDragging = false;
	static bool isDrag = false;
	int degreeStep = 5;

	switch (event->type) {
	case SDL_QUIT:
		m_isRunning = false;
		break;
	case SDL_KEYDOWN: // initialize Arcball
		if (event->key.keysym.sym == SDLK_SPACE) {
			tempArc->Reset(512, 512,v_depth);
			std::cout << "Reset Arcball .. \n";
			Process();
		}
		break;
	case SDL_MOUSEWHEEL:
		break;
	case SDL_MOUSEMOTION:
		if (tempArc->mouse_clicked && (isLoaded)) {
			tempArc->DragMotion(event->motion.x, event->motion.y);
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
		if (event->button.button == SDL_BUTTON_LEFT && (isLoaded)) {
			tempArc->mouse_clicked = true;
			tempArc->MouseDown(event->button.x, event->button.y);
		}
		break;
	case SDL_MOUSEBUTTONUP:
		if (event->button.button == SDL_BUTTON_LEFT && (isLoaded)) {
				Process();
				tempArc->mouse_clicked = false;		
		}
		mouseIsDown = false;
		break;
	case SDL_WINDOWEVENT:
		switch (event->window.event) {
		case SDL_WINDOWEVENT_RESIZED:
			OnResize(event->window.data1, event->window.data2);
			break;
		default:
			break;
		}
	default:
		break;
	}
}