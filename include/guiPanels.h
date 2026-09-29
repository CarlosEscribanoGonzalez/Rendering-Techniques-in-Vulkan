#pragma once
#include "IMGUI/imgui.h"

template <typename T>
void resetIfChanged(T& prev_value, const T& current_value) {
	if (prev_value != current_value) Engine::instance().resetRenderPasses();
	prev_value = current_value;
}

bool prev_ao_state;
void drawAOPanel() {
	ImGui::Begin("Ambient occlusion");
	ImGui::Separator();
	ImGui::Checkbox("Enable ambient occlusion", &runtimeVariables.ao_enabled);
	resetIfChanged(prev_ao_state, runtimeVariables.ao_enabled);
	ImGui::SliderFloat("SSDO factor", &runtimeVariables.ssdo_factor, 0.0f, 1.0f);
	ImGui::SliderFloat("AO radius", &runtimeVariables.ao_radius, 0.01f, 1.0f);
	ImGui::End();
}

AntiAliasingType prev_aa_type;
void drawAAPanel() {
	ImGui::Begin("Anti-aliasing");
	ImGui::Separator();
	ImGui::Text("Anti-aliasing type:");
	if (ImGui::RadioButton("None", runtimeVariables.antiAliasing == AntiAliasingType::None))
		runtimeVariables.antiAliasing = AntiAliasingType::None;
	if (ImGui::RadioButton("TAA", runtimeVariables.antiAliasing == AntiAliasingType::TAA))
		runtimeVariables.antiAliasing = AntiAliasingType::TAA;
	if (ImGui::RadioButton("FXAA", runtimeVariables.antiAliasing == AntiAliasingType::FXAA))
		runtimeVariables.antiAliasing = AntiAliasingType::FXAA;
	if (ImGui::RadioButton("MSAA 2x", runtimeVariables.antiAliasing == AntiAliasingType::MSAA2x))
		runtimeVariables.antiAliasing = AntiAliasingType::MSAA2x;
	if (ImGui::RadioButton("MSAA 4x", runtimeVariables.antiAliasing == AntiAliasingType::MSAA4x))
		runtimeVariables.antiAliasing = AntiAliasingType::MSAA4x;
	if (ImGui::RadioButton("MSAA 8x", runtimeVariables.antiAliasing == AntiAliasingType::MSAA8x))
		runtimeVariables.antiAliasing = AntiAliasingType::MSAA8x;
	resetIfChanged(prev_aa_type, runtimeVariables.antiAliasing);
	ImGui::End();
}

int prev_pp_enabled;
void drawPPPanel() {
	ImGui::Begin("Post-processing");
	ImGui::Separator();
	ImGui::SliderFloat("Exposure", &runtimeVariables.exposure, 0.0f, 10.0f);
	ImGui::Checkbox("Bloom", &runtimeVariables.bloom);
	ImGui::Checkbox("Chromatic aberration", &runtimeVariables.chromaticAberration);
	int currentEnabled = (runtimeVariables.bloom ? 1 : 0) + (runtimeVariables.chromaticAberration ? 1 : 0);
	resetIfChanged(prev_pp_enabled, currentEnabled);
	ImGui::End();
}

ShadowsType prev_shadows_type;
void drawShadowsPanel() {
	ImGui::Begin("Shadows");

	ImGui::Separator();
	ImGui::Text("Shadows type:");
	if (ImGui::RadioButton("Shadow mapping", runtimeVariables.shadowsType == ShadowsType::ShadowMap))
		runtimeVariables.shadowsType = ShadowsType::ShadowMap;
	if (ImGui::RadioButton("Ray traced", runtimeVariables.shadowsType == ShadowsType::RTX_Raw))
		runtimeVariables.shadowsType = ShadowsType::RTX_Raw;
	if (ImGui::RadioButton("Ray traced (denoised)", runtimeVariables.shadowsType == ShadowsType::RTX_Denoised))
		runtimeVariables.shadowsType = ShadowsType::RTX_Denoised;
	resetIfChanged(prev_shadows_type, runtimeVariables.shadowsType);

	bool isShadowMap = runtimeVariables.shadowsType == ShadowsType::ShadowMap;
	ImGui::Separator();
	if (isShadowMap) {
		ImGui::SliderFloat("Bias", &runtimeVariables.shadow_bias.z, 0.0f, 0.001f, "%.6f");
		bool pcf_enabled = runtimeVariables.soft_shadows_config.x == 1.0f;
		ImGui::Checkbox("PCF", &pcf_enabled);
		runtimeVariables.soft_shadows_config.x = pcf_enabled ? 1.0f : 0.0f;
		if (pcf_enabled) {
			int kernelSize = runtimeVariables.soft_shadows_config.y;
			ImGui::SliderInt("PCF Kernel Size", &kernelSize, 3, 21);
			if (kernelSize % 2 == 0) kernelSize++;
			runtimeVariables.soft_shadows_config.y = kernelSize;
		}
		ImGui::SliderInt("Num cascades", &runtimeVariables.num_cascades, 1, kMAX_CASCADES);
	}
	else {
		int numSamples = runtimeVariables.soft_shadows_config.z;
		ImGui::SliderInt("Num samples", &numSamples, 1, 15);
		runtimeVariables.soft_shadows_config.z = numSamples;
		ImGui::SliderFloat("Cone radius", &runtimeVariables.soft_shadows_config.w, 0.0f, 0.3f);
		ImGui::SliderFloat("Bias", &runtimeVariables.shadow_bias.w, 0.0f, 0.001f, "%.6f");
	}
	ImGui::End();
}

void drawReflectionsPanel() {
	ImGui::Begin("Reflections");
	ImGui::Separator();
	bool enabled = runtimeVariables.reflection_config.x == 1.0f;
	ImGui::Checkbox("Reflections enabled", &enabled);
	runtimeVariables.reflection_config.x = enabled ? 1.0f : 0.0f;
	ImGui::SliderFloat("Strength", &runtimeVariables.reflection_config.y, 0.0f, 10.0f);
	ImGui::End();
}

void setupNextPanel(int idx) {
	ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(250, 0), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(10, 10 + idx * 20));
}

void drawGUI() {
	setupNextPanel(0);
	drawAOPanel();
	setupNextPanel(1);
	drawAAPanel();
	setupNextPanel(2);
	drawPPPanel();
	setupNextPanel(3);
	drawShadowsPanel();
	setupNextPanel(4);
	drawReflectionsPanel();
}