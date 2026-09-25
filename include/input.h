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
	AO = 0,
	Tonemapping = 1,
	Shadows = 2,
	Movement = 3,
	Reflections = 4,
	Cam = 5,
	COUNT
};

void HandleAOInput(GLFWwindow* window) {
	if (isKeyDown(window, GLFW_KEY_A)) {
		runtimeVariables.ao_enabled = !runtimeVariables.ao_enabled;
		Engine::instance().resetRenderPasses();
		std::cout << "AO " << (runtimeVariables.ao_enabled ? "enabled" : "disabled") << std::endl;
	}
	if (isKeyDown(window, GLFW_KEY_W)) {
		runtimeVariables.ssdo_factor += 0.1f;
		runtimeVariables.ssdo_factor = std::min(std::max(runtimeVariables.ssdo_factor, 0.0f), 1.0f);
		std::cout << "SSDO factor: " << runtimeVariables.ssdo_factor << std::endl;
	}
	if (isKeyDown(window, GLFW_KEY_S)) {
		runtimeVariables.ssdo_factor -= 0.1f;
		runtimeVariables.ssdo_factor = std::min(std::max(runtimeVariables.ssdo_factor, 0.0f), 1.0f);
		std::cout << "SSDO factor: " << runtimeVariables.ssdo_factor << std::endl;
	}
	if (isKeyDown(window, GLFW_KEY_UP)) {
		runtimeVariables.ao_radius += 0.1f;
		std::cout << "AO radius: " << runtimeVariables.ao_radius << std::endl;
	}
	if (isKeyDown(window, GLFW_KEY_DOWN)) {
		runtimeVariables.ao_radius -= 0.1f;
		runtimeVariables.ao_radius = std::min(std::max(runtimeVariables.ao_radius, 0.0f), 1.0f);
		std::cout << "AO radius: " << runtimeVariables.ao_radius << std::endl;
	}
}

void HandlePPInput(GLFWwindow* window) {
	if (isKeyDown(window, GLFW_KEY_UP)) {
		runtimeVariables.exposure += 0.05f;
		std::cout << "Exposure: " << runtimeVariables.exposure << std::endl;
	}
	if (isKeyDown(window, GLFW_KEY_DOWN)) {
		runtimeVariables.exposure -= 0.05f;
		runtimeVariables.exposure = std::max(runtimeVariables.exposure, 0.0f);
		std::cout << "Exposure: " << runtimeVariables.exposure << std::endl;
	}
	if (isKeyDown(window, GLFW_KEY_B)) {
		runtimeVariables.bloom = !runtimeVariables.bloom;
		Engine::instance().resetRenderPasses();
		std::cout << "Bloom " << (runtimeVariables.bloom ? "enabled" : "disabled") << std::endl;
	}
	if (isKeyDown(window, GLFW_KEY_C)) {
		runtimeVariables.chromaticAberration = !runtimeVariables.chromaticAberration;
		Engine::instance().resetRenderPasses();
		std::cout << "Chromatic aberration " << (runtimeVariables.chromaticAberration ? "enabled" : "disabled") << std::endl;
	}
	if (isKeyDown(window, GLFW_KEY_T)) {
		runtimeVariables.antiAliasing = AntiAliasingType::TAA;
		Engine::instance().resetRenderPasses();
		std::cout << "Antialiasing: TAA" << std::endl;
	}
	if (isKeyDown(window, GLFW_KEY_F)) {
		runtimeVariables.antiAliasing = AntiAliasingType::FXAA;
		Engine::instance().resetRenderPasses();
		std::cout << "Antialiasing: FXAA" << std::endl;
	}
	if (isKeyDown(window, GLFW_KEY_M)) {
		runtimeVariables.antiAliasing = AntiAliasingType::MSAA8x;
		Engine::instance().resetRenderPasses();
		std::cout << "Antialiasing: MSAA" << std::endl;
	}
	if (isKeyDown(window, GLFW_KEY_N)) {
		runtimeVariables.antiAliasing = AntiAliasingType::None;
		Engine::instance().resetRenderPasses();
		std::cout << "Antialiasing: None" << std::endl;
	}
}

void HandleShadowsInput(GLFWwindow* window) {
	bool isShadowMap = runtimeVariables.shadowsType == ShadowsType::ShadowMap;
	if (isShadowMap) {
		if (isKeyDown(window, GLFW_KEY_S)) {
			runtimeVariables.shadow_bias.z -= 0.0001f;
			runtimeVariables.shadow_bias.z = std::max(runtimeVariables.shadow_bias.z, 0.0f);
			std::cout << "Shadow bias: " << runtimeVariables.shadow_bias.z << std::endl;
		}
		if (isKeyDown(window, GLFW_KEY_W)) {
			runtimeVariables.shadow_bias.z += 0.0001f;
			std::cout << "Shadow bias: " << runtimeVariables.shadow_bias.z << std::endl;
		}
		if (isKeyDown(window, GLFW_KEY_P)) {
			runtimeVariables.soft_shadows_config.x = runtimeVariables.soft_shadows_config.x == 0.0f ? 1.0f : 0.0f;
			if (runtimeVariables.soft_shadows_config.x == 1.0f)
				std::cout << "PCF enabled." << std::endl;
			else
				std::cout << "PCF disabled." << std::endl;
		}
		if (isKeyDown(window, GLFW_KEY_UP)) {
			runtimeVariables.soft_shadows_config.y += 2;
			std::cout << "PCF kernel size: " << runtimeVariables.soft_shadows_config.y << std::endl;
		}
		if (isKeyDown(window, GLFW_KEY_DOWN)) {
			runtimeVariables.soft_shadows_config.y -= 2;
			runtimeVariables.soft_shadows_config.y = std::max(runtimeVariables.soft_shadows_config.y, 1.0f);
			std::cout << "PCF kernel size: " << runtimeVariables.soft_shadows_config.y << std::endl;
		}
		if (isKeyDown(window, GLFW_KEY_RIGHT)) {
			runtimeVariables.num_cascades += 1;
			runtimeVariables.num_cascades = std::min(runtimeVariables.num_cascades, (int)kMAX_CASCADES);
			std::cout << "Num cascades: " << runtimeVariables.num_cascades << std::endl;
		}
		if (isKeyDown(window, GLFW_KEY_LEFT)) {
			runtimeVariables.num_cascades -= 1;
			runtimeVariables.num_cascades = std::max(runtimeVariables.num_cascades, 1);
			std::cout << "Num cascades: " << runtimeVariables.num_cascades << std::endl;
		}
	}
	else
	{
		if (isKeyDown(window, GLFW_KEY_W)) {
			runtimeVariables.soft_shadows_config.z += 1;
			std::cout << "Soft shadows samples: " << runtimeVariables.soft_shadows_config.z << std::endl;
		}
		if (isKeyDown(window, GLFW_KEY_S)) {
			runtimeVariables.soft_shadows_config.z -= 1;
			runtimeVariables.soft_shadows_config.z = std::max(runtimeVariables.soft_shadows_config.z, 1.0f);
			std::cout << "Soft shadows samples: " << runtimeVariables.soft_shadows_config.z << std::endl;
		}

		if (isKeyDown(window, GLFW_KEY_UP)) {
			runtimeVariables.soft_shadows_config.w += 0.005f;
			std::cout << "Cone radius: " << runtimeVariables.soft_shadows_config.w << std::endl;
		}
		if (isKeyDown(window, GLFW_KEY_DOWN)) {
			runtimeVariables.soft_shadows_config.w -= 0.005f;
			runtimeVariables.soft_shadows_config.w = std::max(runtimeVariables.soft_shadows_config.w, 0.0f);
			std::cout << "Cone radius: " << runtimeVariables.soft_shadows_config.w << std::endl;
		}

		if (isKeyDown(window, GLFW_KEY_L)) {
			runtimeVariables.shadow_bias.w -= 0.0001f;
			runtimeVariables.shadow_bias.w = std::max(runtimeVariables.shadow_bias.w, 0.0f);
			std::cout << "Shadow bias: " << runtimeVariables.shadow_bias.w << std::endl;
		}
		if (isKeyDown(window, GLFW_KEY_O)) {
			runtimeVariables.shadow_bias.w += 0.0001f;
			std::cout << "Shadow bias: " << runtimeVariables.shadow_bias.w << std::endl;
		}
	}
	if (isKeyDown(window, GLFW_KEY_C)) {
		runtimeVariables.shadowsType = isShadowMap ? ShadowsType::RTX_Denoised : ShadowsType::ShadowMap;
		Engine::instance().resetRenderPasses();
		std::cout << "Shadows type: " << (isShadowMap ? "RTX" : "Shadow mapping") << std::endl;
	}
}

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

void HandleReflectionsInput(GLFWwindow* window) {
	if (isKeyDown(window, GLFW_KEY_R)) {
		runtimeVariables.reflection_config.x = runtimeVariables.reflection_config.x == 0.0f ? 1.0f : 0.0f;
		if (runtimeVariables.reflection_config.x == 0.0f)
			std::cout << "Reflections disabled." << std::endl;
		else
			std::cout << "Reflections enabled." << std::endl;
	}
	if (isKeyDown(window, GLFW_KEY_W)) {
		runtimeVariables.reflection_config.y += 0.1f;
		std::cout << "Reflections strengh: " << runtimeVariables.reflection_config.y << std::endl;
	}
	if (isKeyDown(window, GLFW_KEY_S)) {
		runtimeVariables.reflection_config.y -= 0.1f;
		runtimeVariables.reflection_config.y = std::max(runtimeVariables.reflection_config.y, 0.0f);
		std::cout << "Reflections strengh: " << runtimeVariables.reflection_config.y << std::endl;
	}
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
		const char* inputModeNames[] = { "AO", "Post processing", "Shadows", "Object movement", "Reflections", "Camera"};
		const char* name = inputModeNames[inputModeIdx];
		std::cout << "Input switched to: " << name << std::endl;
	}
	input = static_cast<InputMode>(inputModeIdx);
	switch (input) {
	case InputMode::AO:
		HandleAOInput(window);
		break;
	case InputMode::Tonemapping:
		HandlePPInput(window);
		break;
	case InputMode::Shadows:
		HandleShadowsInput(window);
		break;
	case InputMode::Movement:
		HandleMovementInput(window);
		break;
	case InputMode::Reflections:
		HandleReflectionsInput(window);
		break;
	case InputMode::Cam:
		HandleCameraInput(window);
		break;
	}
}