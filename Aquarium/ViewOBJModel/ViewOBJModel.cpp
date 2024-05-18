// ViewOBJModel.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <Windows.h>
#include <locale>
#include <codecvt>

#include <stdlib.h> // necesare pentru citirea shader-elor
#include <stdio.h>
#include <math.h> 

#include <GL/glew.h>

#include <GLM.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include "Shader.h"
#include "Model.h"

#pragma comment (lib, "glfw3dll.lib")
#pragma comment (lib, "glew32.lib")
#pragma comment (lib, "OpenGL32.lib")

// settings
const unsigned int SCR_WIDTH = 1400;
const unsigned int SCR_HEIGHT = 1000;

enum ECameraMovementType
{
	UNKNOWN,
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

class Camera
{
private:
	// Default camera values
	const float zNEAR = 0.1f;
	const float zFAR = 500.f;
	const float YAW = -90.0f;
	const float PITCH = 0.0f;
	const float FOV = 45.0f;
	glm::vec3 startPosition;

public:
	Camera(const int width, const int height, const glm::vec3& position)
	{
		startPosition = position;
		Set(width, height, position);
	}

	void Set(const int width, const int height, const glm::vec3& position)
	{
		this->isPerspective = true;
		this->yaw = YAW;
		this->pitch = PITCH;

		this->FoVy = FOV;
		this->width = width;
		this->height = height;
		this->zNear = zNEAR;
		this->zFar = zFAR;

		this->worldUp = glm::vec3(0, 1, 0);
		this->position = position;

		lastX = width / 2.0f;
		lastY = height / 2.0f;
		bFirstMouseMove = true;

		UpdateCameraVectors();
	}

	void Reset(const int width, const int height)
	{
		Set(width, height, startPosition);
	}

	void Reshape(int windowWidth, int windowHeight)
	{
		width = windowWidth;
		height = windowHeight;

		// define the viewport transformation
		glViewport(0, 0, windowWidth, windowHeight);
	}

	const glm::mat4 GetViewMatrix() const
	{
		// Returns the View Matrix
		return glm::lookAt(position, position + forward, up);
	}

	const glm::vec3 GetPosition() const
	{
		return position;
	}

	const glm::mat4 GetProjectionMatrix() const
	{
		glm::mat4 Proj = glm::mat4(1);
		if (isPerspective) {
			float aspectRatio = ((float)(width)) / height;
			Proj = glm::perspective(glm::radians(FoVy), aspectRatio, zNear, zFar);
		}
		else {
			float scaleFactor = 2000.f;
			Proj = glm::ortho<float>(
				-width / scaleFactor, width / scaleFactor,
				-height / scaleFactor, height / scaleFactor, -zFar, zFar);
		}
		return Proj;
	}

	void ProcessKeyboard(ECameraMovementType direction, float deltaTime)
	{
		float velocity = (float)(cameraSpeedFactor * deltaTime);
		switch (direction) {
		case ECameraMovementType::FORWARD:
			position += forward * velocity;
			break;
		case ECameraMovementType::BACKWARD:
			position -= forward * velocity;
			break;
		case ECameraMovementType::LEFT:
			position -= right * velocity;
			break;
		case ECameraMovementType::RIGHT:
			position += right * velocity;
			break;
		case ECameraMovementType::UP:
			position += up * velocity;
			break;
		case ECameraMovementType::DOWN:
			position -= up * velocity;
			break;
		}
	}

	void MouseControl(float xPos, float yPos)
	{
		if (bFirstMouseMove) {
			lastX = xPos;
			lastY = yPos;
			bFirstMouseMove = false;
		}

		float xChange = xPos - lastX;
		float yChange = lastY - yPos;
		lastX = xPos;
		lastY = yPos;

		if (fabs(xChange) <= 1e-6 && fabs(yChange) <= 1e-6) {
			return;
		}
		xChange *= mouseSensitivity;
		yChange *= mouseSensitivity;

		ProcessMouseMovement(xChange, yChange);
	}

	void ProcessMouseScroll(float yOffset)
	{
		if (FoVy >= 1.0f && FoVy <= 90.0f) {
			FoVy -= yOffset;
		}
		if (FoVy <= 1.0f)
			FoVy = 1.0f;
		if (FoVy >= 90.0f)
			FoVy = 90.0f;
	}

public:
	void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true)
	{
		yaw += xOffset;
		pitch += yOffset;

		//std::cout << "yaw = " << yaw << std::endl;
		//std::cout << "pitch = " << pitch << std::endl;

		// Avem grijã sã nu ne dãm peste cap
		if (constrainPitch) {
			if (pitch > 89.0f)
				pitch = 89.0f;
			if (pitch < -89.0f)
				pitch = -89.0f;
		}

		// Se modificã vectorii camerei pe baza unghiurilor Euler
		UpdateCameraVectors();
	}

	void UpdateCameraVectors()
	{
		// Calculate the new forward vector
		this->forward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		this->forward.y = sin(glm::radians(pitch));
		this->forward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		this->forward = glm::normalize(this->forward);
		// Also re-calculate the Right and Up vector
		right = glm::normalize(glm::cross(forward, worldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		up = glm::normalize(glm::cross(right, forward));
	}

protected:
	const float cameraSpeedFactor = 12.5f;
	const float mouseSensitivity = 0.1f;

	// Perspective properties
	float zNear;
	float zFar;
	float FoVy;
	int width;
	int height;
	bool isPerspective;

	glm::vec3 position;
	glm::vec3 forward;
	glm::vec3 right;
	glm::vec3 up;
	glm::vec3 worldUp;

	// Euler Angles
	float yaw;
	float pitch;

	bool bFirstMouseMove = true;
	float lastX = 0.f, lastY = 0.f;
};

GLuint ProjMatrixLocation, ViewMatrixLocation, WorldMatrixLocation;
Camera* pCamera = nullptr;

void Cleanup()
{
	delete pCamera;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// timing
double deltaTime = 0.0f;	// time between current frame and last frame
double lastFrame = 0.0f;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_A && action == GLFW_PRESS) {
		//
	}
}

int main()
{
	// glfw: initialize and configure
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// glfw window creation
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Aquarium", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// tell GLFW to capture our mouse
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glewInit();

	glEnable(GL_DEPTH_TEST);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	float vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
	};
	// first, configure the cube's VAO (and VBO)
	unsigned int VBO, cubeVAO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(cubeVAO);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
	unsigned int lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// note that we update the lamp's position attribute's stride to reflect the updated buffer data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Create camera
	pCamera = new Camera(SCR_WIDTH, SCR_HEIGHT, glm::vec3(0.0, 0.0, 3.0));

	//x y z
	//y in sus
	//z in departare
	glm::vec3 lightPos(0.0f, 8.2f, 1.0f);

	wchar_t buffer[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, buffer);

	std::wstring executablePath(buffer);
	std::wstring wscurrentPath = executablePath.substr(0, executablePath.find_last_of(L"\\/"));

	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	std::string currentPath = converter.to_bytes(wscurrentPath);

	Shader lightingShader((currentPath + "\\Shaders\\PhongLight.vs").c_str(), (currentPath + "\\Shaders\\PhongLight.fs").c_str());
	Shader lampShader((currentPath + "\\Shaders\\Lamp.vs").c_str(), (currentPath + "\\Shaders\\Lamp.fs").c_str());
	Shader bubbleShader((currentPath + "\\Shaders\\Bubble.vs").c_str(), (currentPath + "\\Shaders\\Bubble.fs").c_str());

	std::string aquarium = (currentPath + "\\Models\\aquarium\\12987_Saltwater_Aquarium_v1_l1.obj");
	Model aquariumModel(aquarium, false);

	std::string bubbleObj = (currentPath + "\\Models\\Bubble\\bubble.obj");
	Model bubble1ObjModel(bubbleObj, false);
	Model bubble2ObjModel(bubbleObj, false);
	Model bubble3ObjModel(bubbleObj, false);
	Model bubble4ObjModel(bubbleObj, false);
	Model bubble5ObjModel(bubbleObj, false);
	Model bubble6ObjModel(bubbleObj, false);
	Model bubble7ObjModel(bubbleObj, false);
	Model bubble8ObjModel(bubbleObj, false);
	Model bubble9ObjModel(bubbleObj, false);
	Model bubble10ObjModel(bubbleObj, false);
	Model bubble11ObjModel(bubbleObj, false);
	Model bubble12ObjModel(bubbleObj, false);
	Model bubble13ObjModel(bubbleObj, false);
	Model bubble14ObjModel(bubbleObj, false);

	std::string fishObjFileName = (currentPath + "\\Models\\Fish\\fish.obj");
	Model fishObjModel(fishObjFileName, false);

	std::string goldFishObjFileName = (currentPath + "\\Models\\BlackGoldfish\\12990_Black_Moor_Goldfish_v1_l2.obj");
	Model goldFishObjModel(goldFishObjFileName, false);

	std::string blueFishObjFileName = (currentPath + "\\Models\\BlueFish\\13006_Blue_Tang_v1_l3.obj");
	Model blueFishObjModel(blueFishObjFileName, false);

	std::string reefFishObjFileName = (currentPath + "\\Models\\ReefFish\\13007_Blue-Green_Reef_Chromis_v2_l3.obj");
	Model reefFishObjModel(reefFishObjFileName, false);

	std::string fairyFishObjFileName = (currentPath + "\\Models\\FairyFish\\13013_Red_Head_Solon_Fairy_Wrasse_v1_l3.obj");
	Model fairyFishObjModel(fairyFishObjFileName, false);

	std::string diverObjFileName = (currentPath + "\\Models\\DeepSeaDiver\\13018_Aquarium_Deep_Sea_Diver_v1_L1.obj");
	Model diverObjModel(diverObjFileName, false);

	float bubbleHeight = 5.0f;
	// for bubble 1
	float bubble1Y = -2.0f;
	float bubble1Speed = 1.2f;

	// for bubble 2
	float bubble2Y = -2.0f;
	float bubble2Speed = 0.7f;

	// for bubble 3
	float bubble3Y = -2.0f;
	float bubble3Speed = 0.5f;

	// for bubble 4
	float bubble4Y = -2.0f;
	float bubble4Speed = 0.9f;

	// for bubble 5
	float bubble5Y = -2.0f;
	float bubble5Speed = 0.8f;

	// for bubble 6
	float bubble6Y = -2.0f;
	float bubble6Speed = 0.9f;

	// for bubble 7
	float bubble7Y = -2.0f;
	float bubble7Speed = 1.9f;

	// for bubble 8
	float bubble8Y = -2.0f;
	float bubble8Speed = 1.0f;

	// for bubble 9
	float bubble9Y = -2.0f;
	float bubble9Speed = 0.5f;

	// for bubble 10
	float bubble10Y = -2.0f;
	float bubble10Speed = 1.2f;

	float bubble11Y = -2.0f;
	float bubble11Speed = 0.9f;

	float bubble12Y = -2.0f;
	float bubble12Speed = 1.6f;

	float bubble13Y = -2.0f;
	float bubble13Speed = 0.6f;

	float bubble14Y = -2.0f;
	float bubble14Speed = 0.8f;
	// render loop
	while (!glfwWindowShouldClose(window)) {
		// per-frame time logic
		double currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		processInput(window);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float radius = 10.0f; // Raza cercului pe care peștele înoată
		float angularSpeed = 0.5f; // Viteza de rotație a peștelui în jurul cercului

		lightPos.x = radius * cos(glfwGetTime() * angularSpeed);
		lightPos.z = radius * sin(glfwGetTime() * angularSpeed);

		lightingShader.use();
		lightingShader.SetVec3("lightColor", 0.6f, 0.6f, 1.0f);
		lightingShader.SetVec3("lightPos", lightPos);
		lightingShader.SetVec3("viewPos", pCamera->GetPosition());

		lightingShader.setMat4("projection", pCamera->GetProjectionMatrix());
		lightingShader.setMat4("view", pCamera->GetViewMatrix());
		
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ FAIRYFISH ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Calculăm unghiul curent bazat pe timp și pe viteza de rotație dorită
		float fairyangle = glfwGetTime() * angularSpeed;

		// Calculăm noile coordonate x și z ale poziției peștelui pe baza unghiului și razei
		float fairynewX = radius * cos(fairyangle);
		float fairynewZ = radius * sin(fairyangle);

		//lightingShader.SetVec3("objectColor", 1.f, 1.0f, 1.f);
		glm::mat4 fairyfishModelMatrix = glm::mat4(0.03f);
		fairyfishModelMatrix = glm::scale(fairyfishModelMatrix, glm::vec3(3.0f));
		fairyfishModelMatrix = glm::translate(fairyfishModelMatrix, glm::vec3(fairynewX - 20.0f, 40.0f, fairynewZ - 10.0f));
		fairyfishModelMatrix = glm::rotate(fairyfishModelMatrix, -glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotim axa y în jurul axei z

		// Setăm matricea de model în shader-ul de iluminare
		lightingShader.setMat4("model", fairyfishModelMatrix);

		// Desenăm peștele folosind shader-ul de iluminare
		fairyFishObjModel.Draw(lightingShader);

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ REEFFISH ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Define oscillation parameters
		float reefFishOscillationSpeed = 1.0f; // Adjust the speed of oscillation as needed
		float reefFishOscillationRange = 1.0f; // Adjust the range of oscillation as needed
		float reefFishVerticalSpeed = 0.5f; // Adjust the speed of vertical movement as needed
		float reefFishVerticalRange = 2.0f; // Adjust the range of vertical movement as needed

		// Calculate oscillation and vertical movement
		float reefFishOscillationOffset = reefFishOscillationRange * sin(glfwGetTime() * reefFishOscillationSpeed);
		float reefFishVerticalOffset = reefFishVerticalRange * sin(glfwGetTime() * reefFishVerticalSpeed);

		// Calculate new position of the reef fish
		float reefFishNewX = -5.0f + reefFishOscillationOffset;
		float reefFishNewY = 4.0f + reefFishVerticalOffset;
		float reefFishNewZ = 0.0f;

		// Calculate rotation angle based on movement along the X-axis
		float reefFishRotationAngle = atan2(0.0f - reefFishNewZ, -5.0f + reefFishNewX);

		// Set the model matrix for the reef fish
		glm::mat4 reefFishModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(reefFishNewX, reefFishNewY - 2.0f, reefFishNewZ));
		reefFishModelMatrix = glm::rotate(reefFishModelMatrix, reefFishRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
		reefFishModelMatrix = glm::rotate(reefFishModelMatrix, -glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		reefFishModelMatrix = glm::scale(reefFishModelMatrix, glm::vec3(0.2f));

		// Set the model matrix in the shader
		lightingShader.setMat4("model", reefFishModelMatrix);

		// Draw the reef fish
		reefFishObjModel.Draw(lightingShader);



		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BLUEFISH ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Define circular motion parameters
		float blueFishRadius = 0.6f;  // Adjust the radius of the circular motion
		float blueFishAngularSpeed = 0.8f;  // Adjust the angular speed of the circular motion

		// Calculate the angle for circular motion based on time and angular speed
		float blueFishAngle = glfwGetTime() * blueFishAngularSpeed;

		// Calculate the new position of the bluefish based on circular motion
		float blueFishNewX = blueFishRadius * cos(blueFishAngle) + 5.0f;
		float blueFishNewZ = blueFishRadius * sin(blueFishAngle);

		// Calculate the angle between the bluefish and the center of the circle
		float rotationAngle2 = atan2(0.0f - blueFishNewZ, 5.0f - blueFishNewX);

		// Set the Y position of the bluefish
		float blueFishY = 0.0f; // Adjust the Y position as needed

		// Set the model matrix for the bluefish
		glm::mat4 blueFishModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(blueFishNewX, blueFishY, blueFishNewZ));

		// Rotate the bluefish to face the center of the circle
		blueFishModelMatrix = glm::rotate(blueFishModelMatrix, rotationAngle2, glm::vec3(0.0f, 1.0f, 0.0f));
		blueFishModelMatrix = glm::rotate(blueFishModelMatrix, -glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

		// Scale the bluefish if needed
		blueFishModelMatrix = glm::scale(blueFishModelMatrix, glm::vec3(0.1f));

		// Set the model matrix in the shader
		lightingShader.setMat4("model", blueFishModelMatrix);

		// Draw the bluefish
		blueFishObjModel.Draw(lightingShader);


		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ DIVER ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Calculate the position of the diver
		float diverX = 1.0f; // Adjust the X position as needed
		float diverY = 0.67f; // Adjust the Y position as needed
		float diverZ = 0.0f; // Adjust the Z position as needed

		// Set the model matrix for the diver
		glm::mat4 diverModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(diverX, diverY, diverZ));

		// Rotate the diver to align Z-axis with Y-axis
		diverModelMatrix = glm::rotate(diverModelMatrix, -glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

		// Scale the diver if needed
		diverModelMatrix = glm::scale(diverModelMatrix, glm::vec3(0.07f));

		// Set the model matrix in the shader
		lightingShader.setMat4("model", diverModelMatrix);

		// Draw the diver
		diverObjModel.Draw(lightingShader);
		
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ CLOWNFISH ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Calculăm unghiul curent bazat pe timp și pe viteza de rotație dorită
		float angle = glfwGetTime() * angularSpeed;

		// Calculăm noile coordonate x și z ale poziției peștelui pe baza unghiului și razei
		float newX = radius * cos(angle);
		float newZ = radius * sin(angle);

		//lightingShader.SetVec3("objectColor", 1.f, 1.0f, 1.f);
		glm::mat4 fishModelMatrix = glm::mat4(0.03f);
		fishModelMatrix = glm::translate(fishModelMatrix, glm::vec3(newX - 20.0f, -9.0f, newZ - 10.0f));
		fishModelMatrix = glm::rotate(fishModelMatrix, -glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotim axa y în jurul axei z
		
		// Setăm matricea de model în shader-ul de iluminare
		lightingShader.setMat4("model", fishModelMatrix);
		
		// Desenăm peștele folosind shader-ul de iluminare
		fishObjModel.Draw(lightingShader);

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ GOLDFISH ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		float oscillationSpeed = 1.0f; // Adjust the speed of oscillation as needed
		float oscillationRange = 15.0f; // Adjust the range of oscillation as needed

		float oscillationOffset = oscillationRange * sin(glfwGetTime() * oscillationSpeed);
		float goldFishX = 5.0f - oscillationOffset; // Offset the X position to oscillate around 5.0f

		// Calculate the rotation angle based on the movement along the X-axis
		float rotationAngle = atan2(-2.0f * oscillationSpeed * cos(glfwGetTime() * oscillationSpeed), 1.0f) * 180.0f / glm::pi<float>();

		glm::mat4 goldFishModelMatrix = glm::mat4(0.10f);
		goldFishModelMatrix = glm::scale(goldFishModelMatrix, glm::vec3(1.0f));
		goldFishModelMatrix = glm::translate(goldFishModelMatrix, glm::vec3(goldFishX, 30.0f, 0.0f));
		goldFishModelMatrix = glm::rotate(goldFishModelMatrix, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate the fish to face its movement
		goldFishModelMatrix = glm::rotate(goldFishModelMatrix, -glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate the fish

		// Set the model matrix in the shader
		lightingShader.setMat4("model", goldFishModelMatrix);

		// Draw the goldfish
		goldFishObjModel.Draw(lightingShader);

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ AQUARIUM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		glm::mat4 aquariumModelMatrix = glm::scale(glm::mat4(15.0), glm::vec3(0.03f));
		aquariumModelMatrix = glm::translate(aquariumModelMatrix, glm::vec3(0.0f, -5.0f, 0.0f));
		aquariumModelMatrix = glm::rotate(aquariumModelMatrix, -glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		// Desenăm acvariul folosind aceeași matrice de model ca peștele
		lightingShader.setMat4("model", aquariumModelMatrix); // Folosim aceeași matrice de model pentru acvariu ca și pentru pește
		aquariumModel.Draw(lightingShader);

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BUBBLE LOGIC ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BUBBLE 1 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		bubble1Y += bubble1Speed * deltaTime;

		if (bubble1Y > bubbleHeight) {
			bubble1Y = -2.0f; // Reset to the bottom
		}

		// Set the new position of the bubble
		glm::mat4 bubble1ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, bubble1Y, 2.0f));

		// Scale the bubble to make it smaller
		bubble1ModelMatrix = glm::scale(bubble1ModelMatrix, glm::vec3(0.05f));

		// Set the model matrix in the shader
		bubbleShader.use();
		bubbleShader.setMat4("model", bubble1ModelMatrix);
		bubbleShader.setMat4("view", pCamera->GetViewMatrix());
		bubbleShader.setMat4("projection", pCamera->GetProjectionMatrix());

		// Draw the bubble object
		bubble1ObjModel.Draw(bubbleShader);

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BUBBLE 2 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		bubble2Y += bubble2Speed * deltaTime;

		if (bubble2Y > bubbleHeight) {
			bubble2Y = -2.0f; // Reset to the bottom
		}

		// Set the new position of the bubble
		glm::mat4 bubble2ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, bubble2Y, 1.0f));

		// Scale the bubble to make it smaller
		bubble2ModelMatrix = glm::scale(bubble2ModelMatrix, glm::vec3(0.05f));

		// Set the model matrix in the shader
		bubbleShader.setMat4("model", bubble2ModelMatrix);
		bubbleShader.setMat4("view", pCamera->GetViewMatrix());
		bubbleShader.setMat4("projection", pCamera->GetProjectionMatrix());

		// Draw the bubble object
		bubble2ObjModel.Draw(bubbleShader);

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BUBBLE 3 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		bubble3Y += bubble3Speed * deltaTime;

		if (bubble3Y > bubbleHeight) {
			bubble3Y = -2.0f; // Reset to the bottom
		}

		// Set the new position of the bubble
		glm::mat4 bubble3ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, bubble3Y, -1.0f));

		// Scale the bubble to make it smaller
		bubble3ModelMatrix = glm::scale(bubble3ModelMatrix, glm::vec3(0.1f));

		// Set the model matrix in the shader
		bubbleShader.setMat4("model", bubble3ModelMatrix);
		bubbleShader.setMat4("view", pCamera->GetViewMatrix());
		bubbleShader.setMat4("projection", pCamera->GetProjectionMatrix());

		// Draw the bubble object
		bubble3ObjModel.Draw(bubbleShader);

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BUBBLE 4 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		bubble4Y += bubble4Speed * deltaTime;

		if (bubble4Y > bubbleHeight) {
			bubble4Y = -2.0f; // Reset to the bottom
		}

		// Set the new position of the bubble
		glm::mat4 bubble4ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, bubble4Y, -2.0f));

		// Scale the bubble to make it smaller
		bubble4ModelMatrix = glm::scale(bubble4ModelMatrix, glm::vec3(0.09f));

		// Set the model matrix in the shader
		bubbleShader.setMat4("model", bubble4ModelMatrix);
		bubbleShader.setMat4("view", pCamera->GetViewMatrix());
		bubbleShader.setMat4("projection", pCamera->GetProjectionMatrix());

		// Draw the bubble object
		bubble4ObjModel.Draw(bubbleShader);

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BUBBLE 5 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		bubble5Y += bubble5Speed * deltaTime;

		if (bubble5Y > bubbleHeight) {
			bubble5Y = -2.0f; // Reset to the bottom
		}

		// Set the new position of the bubble
		glm::mat4 bubble5ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, bubble5Y, 0.0f));

		// Scale the bubble to make it smaller
		bubble5ModelMatrix = glm::scale(bubble5ModelMatrix, glm::vec3(0.08f));

		// Set the model matrix in the shader
		bubbleShader.setMat4("model", bubble5ModelMatrix);
		bubbleShader.setMat4("view", pCamera->GetViewMatrix());
		bubbleShader.setMat4("projection", pCamera->GetProjectionMatrix());

		// Draw the bubble object
		bubble5ObjModel.Draw(bubbleShader);

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BUBBLE 6 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		bubble6Y += bubble6Speed * deltaTime;

		if (bubble6Y > bubbleHeight) {
			bubble6Y = -2.0f; // Reset to the bottom
		}

		// Set the new position of the bubble
		glm::mat4 bubble6ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, bubble6Y, -1.0f));

		// Scale the bubble to make it smaller
		bubble6ModelMatrix = glm::scale(bubble6ModelMatrix, glm::vec3(0.05f));

		// Set the model matrix in the shader
		bubbleShader.setMat4("model", bubble6ModelMatrix);
		bubbleShader.setMat4("view", pCamera->GetViewMatrix());
		bubbleShader.setMat4("projection", pCamera->GetProjectionMatrix());

		// Draw the bubble object
		bubble6ObjModel.Draw(bubbleShader);

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BUBBLE 7 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		bubble7Y += bubble7Speed * deltaTime;

		if (bubble7Y > bubbleHeight) {
			bubble7Y = -2.0f; // Reset to the bottom
		}

		// Set the new position of the bubble
		glm::mat4 bubble7ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, bubble7Y, 1.0f));

		// Scale the bubble to make it smaller
		bubble7ModelMatrix = glm::scale(bubble7ModelMatrix, glm::vec3(0.05f));

		// Set the model matrix in the shader
		bubbleShader.setMat4("model", bubble7ModelMatrix);
		bubbleShader.setMat4("view", pCamera->GetViewMatrix());
		bubbleShader.setMat4("projection", pCamera->GetProjectionMatrix());

		// Draw the bubble object
		bubble7ObjModel.Draw(bubbleShader);

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BUBBLE 8 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		bubble8Y += bubble8Speed * deltaTime;

		if (bubble8Y > bubbleHeight) {
			bubble8Y = -2.0f; // Reset to the bottom
		}

		// Set the new position of the bubble
		glm::mat4 bubble8ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(4.0f, bubble8Y, -1.0f));

		// Scale the bubble to make it smaller
		bubble8ModelMatrix = glm::scale(bubble8ModelMatrix, glm::vec3(0.05f));

		// Set the model matrix in the shader
		bubbleShader.setMat4("model", bubble8ModelMatrix);
		bubbleShader.setMat4("view", pCamera->GetViewMatrix());
		bubbleShader.setMat4("projection", pCamera->GetProjectionMatrix());

		// Draw the bubble object
		bubble8ObjModel.Draw(bubbleShader);

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BUBBLE 9 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		bubble9Y += bubble9Speed * deltaTime;

		if (bubble9Y > bubbleHeight) {
			bubble9Y = -2.0f; // Reset to the bottom
		}

		// Set the new position of the bubble
		glm::mat4 bubble9ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-3.0f, bubble9Y, 1.0f));

		// Scale the bubble to make it smaller
		bubble9ModelMatrix = glm::scale(bubble9ModelMatrix, glm::vec3(0.05f));

		// Set the model matrix in the shader
		bubbleShader.setMat4("model", bubble9ModelMatrix);
		bubbleShader.setMat4("view", pCamera->GetViewMatrix());
		bubbleShader.setMat4("projection", pCamera->GetProjectionMatrix());

		// Draw the bubble object
		bubble9ObjModel.Draw(bubbleShader);

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BUBBLE 10 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		bubble10Y += bubble10Speed * deltaTime;

		if (bubble10Y > bubbleHeight) {
			bubble10Y = -2.0f; // Reset to the bottom
		}

		// Set the new position of the bubble
		glm::mat4 bubble10ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(6.0f, bubble10Y, 0.0f));

		// Scale the bubble to make it smaller
		bubble10ModelMatrix = glm::scale(bubble10ModelMatrix, glm::vec3(0.05f));

		// Set the model matrix in the shader
		bubbleShader.setMat4("model", bubble10ModelMatrix);
		bubbleShader.setMat4("view", pCamera->GetViewMatrix());
		bubbleShader.setMat4("projection", pCamera->GetProjectionMatrix());

		// Draw the bubble object
		bubble10ObjModel.Draw(bubbleShader);

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BUBBLE 11 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		bubble11Y += bubble11Speed * deltaTime;

		if (bubble11Y > bubbleHeight) {
			bubble11Y = -2.0f; // Reset to the bottom
		}

		// Set the new position of the bubble
		glm::mat4 bubble11ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-3.0f, bubble11Y, 2.0f));

		// Scale the bubble to make it smaller
		bubble11ModelMatrix = glm::scale(bubble11ModelMatrix, glm::vec3(0.05f));

		// Set the model matrix in the shader
		bubbleShader.setMat4("model", bubble11ModelMatrix);
		bubbleShader.setMat4("view", pCamera->GetViewMatrix());
		bubbleShader.setMat4("projection", pCamera->GetProjectionMatrix());

		// Draw the bubble object
		bubble11ObjModel.Draw(bubbleShader);

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BUBBLE 12 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		bubble12Y += bubble12Speed * deltaTime;

		if (bubble12Y > bubbleHeight) {
			bubble12Y = -2.0f; // Reset to the bottom
		}

		// Set the new position of the bubble
		glm::mat4 bubble12ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, bubble12Y, -2.0f));

		// Scale the bubble to make it smaller
		bubble12ModelMatrix = glm::scale(bubble12ModelMatrix, glm::vec3(0.05f));

		// Set the model matrix in the shader
		bubbleShader.setMat4("model", bubble12ModelMatrix);
		bubbleShader.setMat4("view", pCamera->GetViewMatrix());
		bubbleShader.setMat4("projection", pCamera->GetProjectionMatrix());

		// Draw the bubble object
		bubble12ObjModel.Draw(bubbleShader);

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BUBBLE 13 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		bubble13Y += bubble13Speed * deltaTime;

		if (bubble13Y > bubbleHeight) {
			bubble13Y = -2.0f; // Reset to the bottom
		}

		// Set the new position of the bubble
		glm::mat4 bubble13ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, bubble13Y, 1.5f));

		// Scale the bubble to make it smaller
		bubble13ModelMatrix = glm::scale(bubble13ModelMatrix, glm::vec3(0.05f));

		// Set the model matrix in the shader
		bubbleShader.setMat4("model", bubble13ModelMatrix);
		bubbleShader.setMat4("view", pCamera->GetViewMatrix());
		bubbleShader.setMat4("projection", pCamera->GetProjectionMatrix());

		// Draw the bubble object
		bubble13ObjModel.Draw(bubbleShader);

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BUBBLE 14 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		bubble14Y += bubble14Speed * deltaTime;

		if (bubble14Y > bubbleHeight) {
			bubble14Y = -2.0f; // Reset to the bottom
		}

		// Set the new position of the bubble
		glm::mat4 bubble14ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(4.0f, bubble14Y, 2.0f));

		// Scale the bubble to make it smaller
		bubble14ModelMatrix = glm::scale(bubble14ModelMatrix, glm::vec3(0.05f));

		// Set the model matrix in the shader
		bubbleShader.setMat4("model", bubble14ModelMatrix);
		bubbleShader.setMat4("view", pCamera->GetViewMatrix());
		bubbleShader.setMat4("projection", pCamera->GetProjectionMatrix());

		// Draw the bubble object
		bubble14ObjModel.Draw(bubbleShader);


		glDisable(GL_BLEND);

		// also draw the lamp object
		lampShader.use();
		lampShader.setMat4("projection", pCamera->GetProjectionMatrix());
		lampShader.setMat4("view", pCamera->GetViewMatrix());
		glm::mat4 lightModel = glm::translate(glm::mat4(1.0), lightPos);
		lightModel = glm::scale(lightModel, glm::vec3(0.05f)); // a smaller cube
		lampShader.setMat4("model", lightModel);

		glBindVertexArray(lightVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	Cleanup();

	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &VBO);

	// glfw: terminate, clearing all previously allocated GLFW resources
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		pCamera->ProcessKeyboard(FORWARD, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		pCamera->ProcessKeyboard(BACKWARD, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		pCamera->ProcessKeyboard(LEFT, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		pCamera->ProcessKeyboard(RIGHT, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		pCamera->ProcessKeyboard(UP, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		pCamera->ProcessKeyboard(DOWN, (float)deltaTime);

	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		pCamera->Reset(width, height);

	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	pCamera->Reshape(width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    // Calculate the center of the window
    float windowCenterX = width / 2.0f;
    float windowCenterY = height / 2.0f;

    // Calculate the offset from the center to the cursor position
    float xOffset = xpos - windowCenterX;
    float yOffset = windowCenterY - ypos; // Invert the yOffset

    // Set the cursor position back to the center
    glfwSetCursorPos(window, windowCenterX, windowCenterY);

    // Adjust camera rotation based on cursor offset
    float sensitivity = 0.1f;
    pCamera->ProcessMouseMovement(xOffset * sensitivity, yOffset * sensitivity);
}


void scroll_callback(GLFWwindow* window, double xoffset, double yOffset)
{
	pCamera->ProcessMouseScroll((float)yOffset);
}