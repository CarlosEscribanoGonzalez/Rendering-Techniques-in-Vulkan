#version 460

#extension GL_ARB_shader_draw_parameters : enable
#extension GL_KHR_vulkan_glsl : enable

layout( location = 0 ) in vec2 f_uvs;

struct LightData
{
    vec4 m_light_pos;
    vec4 m_radiance;
    vec4 m_attenuattion;
    mat4 m_view_projection [ 6 ];
};
struct SSAOData {
        float enable_ao;
        float radius;
        float bias;
        float ssdo_factor;
};

layout( std140, set = 0, binding = 0 ) uniform PerFrameData
{
    vec4      m_camera_pos;
    mat4      m_view;
    mat4      m_projection;
    mat4      m_view_projection;
    mat4      m_inv_view;
    mat4      m_inv_projection;
    mat4      m_inv_view_projection;
    mat4      m_prev_view;
    vec4      m_clipping_planes;
    LightData m_lights[ 10 ];
    uint      m_number_of_lights;
    SSAOData  m_ssao_data;
    float     m_exposure;
} per_frame_data;

layout ( set = 0, binding = 1 ) uniform sampler2D i_color;
layout(location = 0) out vec4 out_color;

const float A = 2.51;
const float B = 0.03;
const float C = 2.43;
const float D = 0.59;
const float E = 0.14;

void main() { //Implementación con ACES
    vec3 color = texture(i_color, f_uvs).xyz;
    color *= per_frame_data.m_exposure;
    //Tonemapping:
    vec3 numerator = color * (A * color + B);
    vec3 denominator = color * (C * color + D) + E;
    color = clamp(numerator / denominator, 0.0f, 1.0f);
    //Gamma correction:
    color = pow(color, vec3(1.0 / 2.2));

    out_color = vec4(color, 1.0);
}