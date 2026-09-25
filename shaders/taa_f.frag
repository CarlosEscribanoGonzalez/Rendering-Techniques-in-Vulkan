#version 460

#extension GL_ARB_shader_draw_parameters : enable
#extension GL_KHR_vulkan_glsl : enable
#define INF 1.0 / 0.0

layout( location = 0 ) in vec2 f_uvs;

//globals
struct LightData
{
    vec4 m_light_pos;
    vec4 m_radiance;
    vec4 m_attenuattion;
    mat4 m_view_projection [ 6 ];
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
} per_frame_data;

layout ( set = 0, binding = 1 ) uniform sampler2D i_color;
layout ( set = 0, binding = 2 ) uniform sampler2D i_history;
layout ( set = 0, binding = 3 ) uniform sampler2D i_motion;

const float HISTORY_WEIGHT = 0.05f; //Debe estar en [0, 1], pero mejor con valores bajos

layout(location = 0) out vec4 out_taa;

void main() {
    vec3 fragment_color = texture(i_color, f_uvs).xyz;
    vec2 motion = texture(i_motion, f_uvs).xy;
    vec3 histColor = texture(i_history, f_uvs - motion).xyz;
    vec2 texelSize = 1.0 / vec2(textureSize(i_history, 0));
    vec3 maxColor = vec3(-INF, -INF, -INF);
    vec3 minColor = vec3(INF, INF, INF);
    for(int x = -1; x <= 1; x++){
        for(int y = -1; y <= 1; y++){
            if(x == 0 && y == 0) continue;
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            vec3 neighborColor = texture(i_color, f_uvs + offset).xyz;
            maxColor = max(maxColor, neighborColor);
            minColor = min(minColor, neighborColor);
        }
    }
    histColor = clamp(histColor, minColor, maxColor);
    out_taa = vec4(mix(histColor, fragment_color, HISTORY_WEIGHT), 1.0);
} 