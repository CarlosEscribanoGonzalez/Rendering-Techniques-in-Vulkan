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
} per_frame_data;

struct ObjectData
{
    mat4 m_model;
    mat4 m_prev_model;
    mat4 m_inv_model;
    vec4 m_albedo; 
    vec4 m_metallic_roughness;
};
layout ( set = 0, binding = 1 ) uniform sampler2D i_positions;
layout ( set = 0, binding = 2 ) uniform sampler2D i_materials;
//all object matrices
layout(std140,set = 0, binding = 3) readonly buffer ObjectBufferData
{
    ObjectData objects[];
} per_object_data;

layout(location = 0) out vec2 out_motions;

void main() {
    int obj_instance = int(floor(texture(i_materials, f_uvs).w));
    vec4 frag_pos = vec4(texture(i_positions, f_uvs).xyz, 1.0);
    if(length(frag_pos.xyz) == 0)
    {
        out_motions = vec2(0.0, 0.0);
        return;
    }
    mat4 prev_model = per_object_data.objects[obj_instance].m_prev_model;
    mat4 inv_model = per_object_data.objects[obj_instance].m_inv_model;
    //Posiciones en clip:
    vec4 prev_clip = per_frame_data.m_projection * per_frame_data.m_prev_view * prev_model * inv_model * frag_pos;
    vec4 current_clip = per_frame_data.m_projection * per_frame_data.m_view * frag_pos;
    //UVs:
    vec2 prev_uv = (prev_clip.xy / prev_clip.w) * 0.5 + 0.5;
    vec2 curr_uv = (current_clip.xy / current_clip.w) * 0.5 + 0.5;
    //Cálculo motion:
    out_motions = curr_uv - prev_uv;
} 