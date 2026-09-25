#version 460

#extension GL_ARB_shader_draw_parameters : enable
#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_ray_query : enable
#define INV_PI 0.31830988618
#define PI   3.14159265358979323846264338327950288

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


layout ( set = 0, binding = 1 ) uniform sampler2DMS i_position_and_depth;
layout ( set = 0, binding = 2 ) uniform sampler2DMS i_normal;
layout ( set = 0, binding = 3 ) uniform accelerationStructureEXT TLAS;

layout(location = 0) out float out_color;

float halton(int index, int base) {
    float result = 0.0;
    float f = 1.0;
    int i = index;
    while(i > 0) {
        f /= float(base);
        result += f * float(i % base);
        i /= base;
    }
    return result;
}

bool rayIntersects(vec3 origin, vec3 n, vec3 dir, float dist)
{
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(
        rayQuery, 
        TLAS,
        gl_RayFlagsOpaqueEXT, //flags - ignora any-hit shaders
        0xFF, //Mask
        origin + n * per_frame_data.m_shadow_bias.w, //Posición inicial
        per_frame_data.m_shadow_bias.w, //tMin
        dir,
        dist
    );
    rayQueryProceedEXT(rayQuery);
    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT)
        return false;
    return true;
}

float evalVisibility(vec3 frag_pos, vec3 n, vec3 l, float dist, uint id_light)
{
    vec3 aux = abs(l.x) > 0.9 ? vec3(0, 1, 0) : vec3(1, 0, 0);
    vec3 tangent = normalize(cross(l, aux));
    vec3 bitangent = normalize(cross(l, tangent));
    int num_samples = int(floor(per_frame_data.m_soft_shadows_config.z));
    float normal_factor = abs(dot(n, l));
    float cone_radius = per_frame_data.m_soft_shadows_config.w * normal_factor;
    float visibility = 0.0f;
    for(int i = 0; i < num_samples; i++){
        int idx = int(per_frame_data.m_frame_idx) * num_samples + i;
        float angle = halton(idx, 2) * 2.0 * PI;
        float r = cone_radius * sqrt(halton(idx, 3));
        vec3 offset = (cos(angle) * tangent + sin(angle) * bitangent) * r;
        vec3 perturbed_dir = normalize(l + offset);
        if(!rayIntersects(frag_pos, n, perturbed_dir, dist))
            visibility += 1.0;
    }
    return visibility / float(num_samples);
}

float luminance(vec3 color) {
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

void main() 
{ 
    ivec2 coord = ivec2(gl_FragCoord.xy);
    int samples = textureSamples(i_normal);
    float receivedEnergy = 0;
    float totalEnergy = 0;
    int numValidLights = 0;
    for(int id_light = 0; id_light < per_frame_data.m_number_of_lights; id_light++)
    {
        uint light_type = uint(floor(per_frame_data.m_lights[id_light].m_light_pos.a));
        if(light_type <= 1) numValidLights++;
    }
    if(numValidLights == 0) 
    { 
        out_color = 1.0f; 
        return; 
    }
    for(int i = 0; i < samples; i++){
        vec3 normal = normalize(texelFetch(i_normal, coord, i).rgb * 2.0 - 1.0);
        vec3 frag_pos = texelFetch(i_position_and_depth, coord, i).rgb;
        for(int id_light = 0; id_light < per_frame_data.m_number_of_lights; id_light++){
            LightData light = per_frame_data.m_lights[id_light];
            uint light_type = uint(floor(light.m_light_pos.a));
            vec3 l; 
            float dist;
            vec3 radiance = vec3(0.0f);
            if(light_type == 0)
            {
                l = normalize(-light.m_light_pos.xyz);
                dist = per_frame_data.m_clipping_planes.y;
                radiance = light.m_radiance.rgb;
            } else if(light_type == 1)
            {
                l = light.m_light_pos.xyz - frag_pos;
                dist = length(l);
                l = normalize(l);
                float att = 1.0 / (light.m_attenuattion.x + light.m_attenuattion.y * dist + light.m_attenuattion.z * dist * dist );
                radiance = light.m_radiance.rgb * att;
            } 
            else {
                float lum = luminance(light.m_radiance.rgb);
                totalEnergy += lum;
                receivedEnergy += lum;
                continue;
            }
            float L = luminance(radiance);
            totalEnergy += L;
            receivedEnergy += L * evalVisibility(frag_pos, normal, l, dist, id_light);
        }
    }
    totalEnergy /= float(samples);
    receivedEnergy /= float(samples);
    out_color = receivedEnergy / totalEnergy;
}