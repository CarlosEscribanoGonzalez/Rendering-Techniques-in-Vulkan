#pragma once

#include "common.h"
#include "runtime.h"
#include "camera.h"

using namespace MiniEngine;

std::set<int> pressedKeys; //Globally accesible, could be improved but it doesn't matter given the use case
int inputModeIdx = 0;

bool isKeyDown(GLFWwindow* window, int key) {
	if (glfwGetKey(window, key) == GLFW_PRESS && !pressedKeys.count(key)) {
		pressedKeys.insert(key);
		return true;
	}
	else if (glfwGetKey(window, key) == GLFW_RELEASE && pressedKeys.count(key))
		pressedKeys.erase(key);
	return false;
}

bool isBeingPressed(GLFWwindow* window, int key) {
	if (glfwGetKey(window, key) == GLFW_PRESS)
		return true;
	return false;
}

enum InputMode {
	Cam = 0,
	Movement = 1,
	COUNT
};

int selectedObjIdx = 0;
void HandleMovementInput(GLFWwindow* window) {
	float translationSpeed = 0.0025f;
	const std::vector<EntityPtr> entities = Engine::instance().getScene().getMeshes();
	if (isKeyDown(window, GLFW_KEY_LEFT)) {
		selectedObjIdx--;
		if (selectedObjIdx < 0) selectedObjIdx = entities.size() - 1;
		std::cout << "Selected object ID: " << selectedObjIdx << std::endl;
	}
	if (isKeyDown(window, GLFW_KEY_RIGHT)) {
		selectedObjIdx++;
		if (selectedObjIdx >= entities.size()) selectedObjIdx = 0;
		std::cout << "Selected object ID: " << selectedObjIdx << std::endl;
	}
	Transform& transformToMove = entities[selectedObjIdx]->getTransform();
	Vector3f dir = Vector3f(0.0f);
	if (isBeingPressed(window, GLFW_KEY_W))
		dir += Vector3f(0.0f, 0.0f, -1.0f);
	if (isBeingPressed(window, GLFW_KEY_S))
		dir += Vector3f(0.0f, 0.0f, 1.0f);
	if (isBeingPressed(window, GLFW_KEY_A))
		dir += Vector3f(-1.0f, 0.0f, 0.0f);
	if (isBeingPressed(window, GLFW_KEY_D))
		dir += Vector3f(1.0f, 0.0f, 0.0f);
	if (isBeingPressed(window, GLFW_KEY_E))
		dir += Vector3f(0.0f, 1.0f, 0.0f);
	if (isBeingPressed(window, GLFW_KEY_Q))
		dir += Vector3f(0.0f, -1.0f, 0.0f);
	dir *= translationSpeed;
	transformToMove.translate(dir);
}

void HandleCameraInput(GLFWwindow* window) 
{
	Camera& cam = Engine::instance().getScene().getCamera();
	float translationSpeed = 0.05f;
	Vector3f dir = Vector3f(0.0f);
	if (isBeingPressed(window, GLFW_KEY_W))
		dir += Vector3f(0.0f, 0.0f, -1.0f);
	if (isBeingPressed(window, GLFW_KEY_S))
		dir += Vector3f(0.0f, 0.0f, 1.0f);
	if (isBeingPressed(window, GLFW_KEY_A))
		dir += Vector3f(-1.0f, 0.0f, 0.0f);
	if (isBeingPressed(window, GLFW_KEY_D))
		dir += Vector3f(1.0f, 0.0f, 0.0f);
	if (isBeingPressed(window, GLFW_KEY_E))
		dir += Vector3f(0.0f, 1.0f, 0.0f);
	if (isBeingPressed(window, GLFW_KEY_Q))
		dir += Vector3f(0.0f, -1.0f, 0.0f);
	dir *= translationSpeed;
	cam.setPosition(cam.getCameraPos() + dir);
	if (isBeingPressed(window, GLFW_KEY_UP))
		cam.setClippingPlanes(cam.getNearPlane(), cam.getFarPlane() + translationSpeed);
	if (isBeingPressed(window, GLFW_KEY_DOWN))
		cam.setClippingPlanes(cam.getNearPlane(), cam.getFarPlane() - translationSpeed);
}

void processInput(GLFWwindow* window) {
	InputMode input;
	if (isKeyDown(window, GLFW_KEY_SPACE)) {
		inputModeIdx++;
		if (static_cast<InputMode>(inputModeIdx) == InputMode::COUNT)
			inputModeIdx = 0;
		const char* inputModeNames[] = { "Camera", "Object" };
		const char* name = inputModeNames[inputModeIdx];
		std::cout << "Input switched to: " << name << std::endl;
	}
	input = static_cast<InputMode>(inputModeIdx);
	switch (input) {
	case InputMode::Cam:
		HandleCameraInput(window);
		break;
	case InputMode::Movement:
		HandleMovementInput(window);
		break;
	}
}