#version 460

#extension GL_ARB_shader_draw_parameters : enable
#extension GL_KHR_vulkan_glsl : enable
#define MAX_MASK_SIZE 21

layout( location = 0 ) in vec2 f_uvs;

//globals
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
    vec4      m_shadow_bias; //min, slope, max y tmin (rtx)
    vec4      m_cascade_splits;
    vec4      m_soft_shadows_config; //enabled, kernel size, num samples (rtx) y cone radius (rtx)
    vec4      m_reflections_config; //enabled, strength, unused y unused
    uint      m_frame_idx;
} per_frame_data;

layout ( set = 0, binding = 1) uniform sampler2D i_visibility;
layout(std140, set = 0, binding = 2) uniform MaskData {
    vec4 mask[MAX_MASK_SIZE * MAX_MASK_SIZE / 4];
    uint width;
} mask_data;
layout ( set = 0, binding = 3) uniform sampler2D i_depth;
layout ( set = 0, binding = 4) uniform sampler2D i_normal;
layout ( set = 0, binding = 5) uniform sampler2D i_history;
layout ( set = 0, binding = 6) uniform sampler2D i_motion;

layout(location = 0) out float out_blur;

const float P = 8.0;
const float SIGMA = 0.01;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(i_visibility, 0));
    float result = 0.0;
    float total_weight = 0;
    vec3 frag_n = texture(i_normal, f_uvs).rgb * 2.0 - 1.0;
    float frag_depth = texture(i_depth, f_uvs).r;
    int half_width = int(mask_data.width) / 2;
    for (int x = -half_width; x <= half_width; ++x) 
    {
        for (int y = -half_width; y <= half_width; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            int i = x + half_width + (y + half_width) * int(mask_data.width);
            vec3 n = texture(i_normal, f_uvs + offset).rgb * 2.0 - 1.0;
            float depth = texture(i_depth, f_uvs + offset).r;
            //Pesos:
            float gauss_weight = mask_data.mask[i / 4][i % 4];
            float normal_weight = pow(max(dot(frag_n, n), 0), P);
            float depth_weight = exp(-abs(frag_depth - depth) / SIGMA);
            float local_weight = gauss_weight * normal_weight * depth_weight;
            result += texture(i_visibility, f_uvs + offset).x * local_weight;
            total_weight += local_weight;
        }
    }

    if(total_weight > 0.0001)
        result /= total_weight;
    else
        result = texture(i_visibility, f_uvs).x;
    vec2 motion = texture(i_motion, f_uvs).xy;
    float prev = texture(i_history, f_uvs - motion).x;
    if(per_frame_data.m_frame_idx == 0) out_blur = result;
    else out_blur = mix(prev, result, 0.05);
} 