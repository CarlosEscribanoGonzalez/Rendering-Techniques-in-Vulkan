#version 460

#extension GL_ARB_shader_draw_parameters : enable
#extension GL_KHR_vulkan_glsl : enable
#define INV_PI 0.31830988618
#define PI   3.14159265358979323846264338327950288

layout( location = 0 ) in vec2 f_uvs;

//Han de coincidir con las constantes en CPU:
const int MAX_CASCADES = 4;
const int LAYERS = 6;

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
} per_frame_data;

layout ( set = 0, binding = 1 ) uniform sampler2DMS i_albedo;
layout ( set = 0, binding = 2 ) uniform sampler2DMS i_position_and_depth;
layout ( set = 0, binding = 3 ) uniform sampler2DMS i_normal;
layout ( set = 0, binding = 4 ) uniform sampler2DMS i_material;
layout ( set = 0, binding = 5 ) uniform sampler2D i_ssao;
layout ( set = 0, binding = 6 ) uniform sampler2DArray i_shadows;

layout(location = 0) out vec4 out_color;

// ========================== MICROFACETS ==========================
float D(float roughness, vec3 n, vec3 h)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float NdotH = max(dot(n, h), 0.0);
    float t = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * t * t);
}

float G1(vec3 n, vec3 x, float k)
{
    float NdotX = max(dot(n, x), 0.0);
    return NdotX / (NdotX * (1 - k) + k);
}

float G(float roughness, vec3 n, vec3 v, vec3 l)
{
    float k = (roughness + 1) * (roughness + 1) / 8;
    return G1(n, l, k) * G1(n, v, k);
}

vec3 F(float metallic, vec3 albedo, vec3 v, vec3 h)
{
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    float VdotH = max(dot(v, h), 0.0);
    float exponent = (-5.55473 * VdotH - 6.98316) * VdotH;
    return F0 + (1.0 - F0) * pow(2.0, exponent);
}

vec3 evalSpec(vec3 l, vec3 v, vec3 n, vec3 h, vec3 albedo, vec3 mat)
{
    float D = D(mat.z, n, h);
    float G = G(mat.z, n, v, l);
    vec3 F = F(mat.y, albedo, v, h);
    float NdotL = max(dot(n, l), 0);
    float NdotV = max(dot(n, v), 0);
    return D * G * F / max(4.0 * NdotL * NdotV, 0.001) * NdotL;
}

// ========================== SHADOW MAPPING ==========================
int getCascade(vec3 frag_pos)
{  
    vec3 view_pos = (per_frame_data.m_view * vec4(frag_pos, 1.0)).xyz;
    for(int i = 0; i < MAX_CASCADES - 1; i++){
        if(abs(view_pos.z) < per_frame_data.m_cascade_splits[i]) return i; //Hay que recordar que en Vulkan z avanza en negativo
    }
    return MAX_CASCADES - 1;
}

int getFace(vec3 frag_pos, uint id_light)
{
    vec3 light_pos = per_frame_data.m_lights[id_light].m_light_pos.xyz;
    vec3 l = frag_pos - light_pos;
    vec3 abs_l = abs(l);
    if(abs_l.x >= abs_l.y && abs_l.x >= abs_l.z)
        return l.x > 0 ? 0 : 1; // +X o -X
    else if(abs_l.y >= abs_l.x && abs_l.y >= abs_l.z)
        return l.y > 0 ? 2 : 3; // +Y o -Y
    else
        return l.z > 0 ? 4 : 5; // +Z o -Z
    return 0;
}

float evalVisibility(vec3 frag_pos, vec3 n, vec3 l, uint id_light, uint light_type, bool usePCF)
{
    //Obtención del layer:
    int idx = light_type == 0 ? getCascade(frag_pos) : getFace(frag_pos, id_light);
    int layer = int(id_light) * LAYERS + idx;
    //Proyección a la luz:
    mat4 lightViewProj = per_frame_data.m_lights[id_light].m_view_projection[idx];
    vec4 projected_pos = lightViewProj * vec4(frag_pos, 1.0);
    //Obtención de las UVs proyectadas:
    projected_pos /= projected_pos.w;
    vec2 uv = projected_pos.xy * 0.5f + 0.5f; //Projected_pos está en NDC [-1, 1]
    float depth_frag = projected_pos.z;
    //Bias dependiente de inclinación:
    float NdotL = max(dot(n, l), 0.0001);
    float tanTheta = sqrt(1 - NdotL * NdotL) / NdotL;
    float bias = min(per_frame_data.m_shadow_bias.x + per_frame_data.m_shadow_bias.y * tanTheta, 
        per_frame_data.m_shadow_bias.z);
    //PCF:
    if(!usePCF){
        float depth_stored = texture(i_shadows, vec3(uv, float(layer))).x;
        return (depth_frag - bias) > depth_stored ? 0.0f : 1.0f;
    }
    vec2 texelSize = 1.0 / vec2(textureSize(i_shadows, 0));
    int result = 0;
    int size = int(floor(per_frame_data.m_soft_shadows_config.y));
    int halfSize = size / 2;
    for(int i = -halfSize; i <= halfSize; i++){
        for(int j = -halfSize; j <= halfSize; j++){
            vec2 offset = vec2(texelSize.x * i, texelSize.y * j);
            float depth_stored = texture(i_shadows, vec3(uv + offset, float(layer))).x;
            if (depth_frag - bias <= depth_stored) result++;
        }
    }
    return float(result) / (size * size);
}

// ========================== SHADING ==========================
vec3 capSpecular(vec3 spec, float maxLum) 
{
    float lum = dot(spec, vec3(0.2126, 0.7152, 0.0722));
    return lum > maxLum ? spec * (maxLum / lum) : spec;
}

vec3 evalMicrofacet(vec3 mat, vec3 albedo, vec3 n, vec3 frag_pos)
{
    vec3 v = normalize(per_frame_data.m_camera_pos.xyz - frag_pos);
    vec3 baseDiffuse = albedo / PI;
    vec3 diffuse = vec3(0.0);
    vec3 spec = vec3( 0.0 );
    bool pcf_enabled = per_frame_data.m_soft_shadows_config.x >= 0.5f;
    for( uint id_light = 0; id_light < per_frame_data.m_number_of_lights; id_light++ )
    {
        LightData light = per_frame_data.m_lights[ id_light ];
        uint light_type = uint( floor( light.m_light_pos.a ) );
        switch( light_type )
        {
            case 0: //directional
            {
                vec3 l = normalize( -light.m_light_pos.xyz );
                vec3 h = normalize(v + l);
                vec3 radiance = light.m_radiance.rgb;
                float visibility = evalVisibility(frag_pos, n, l, id_light, light_type, pcf_enabled);
                spec += evalSpec(l, v, n, h, albedo, mat) * radiance * visibility;
                vec3 kd = (vec3(1.0) - F(mat.y, albedo, v, h)) * (1.0 - mat.y);
                diffuse += kd * baseDiffuse * max(dot(n, l), 0.0) * radiance * visibility;
                break;
            }
            case 1: //point
            {
                vec3 l = light.m_light_pos.xyz - frag_pos;
                float dist = length( l );
                float att = 1.0 / (light.m_attenuattion.x + light.m_attenuattion.y * dist + light.m_attenuattion.z * dist * dist );
                vec3 radiance = light.m_radiance.rgb * att;
                l = normalize(l);
                vec3 h = normalize(v + l);
                float visibility = evalVisibility(frag_pos, n, l, id_light, light_type, pcf_enabled);
                spec += evalSpec(l, v, n, h, albedo, mat) * radiance * visibility;
                vec3 kd = (vec3(1.0) - F(mat.y, albedo, v, h)) * (1.0 - mat.y);
                diffuse += kd * baseDiffuse * max(dot(n, l), 0.0) * radiance * visibility;
                break;
            }
            case 2:
            {
                if(per_frame_data.m_ssao_data.enable_ao >= 0.5f){
                    vec4 ssao = texture(i_ssao, f_uvs);
                    vec3 direct = light.m_radiance.rgb * albedo.rgb * ssao.w;
                    vec3 indirect = ssao.rgb * albedo.rgb;
                    diffuse += direct + indirect;
                }
                else
                    diffuse += light.m_radiance.rgb * albedo.rgb;
                break;
            }
        }
    }
    spec = capSpecular(spec, 10.0); //Evita sobre saturaciones cuando metálico es muy alto y roughness muy bajo
    return diffuse + spec;
}

vec3 evalDiffuse(vec3 albedo, vec3 n, vec3 frag_pos)
{
    vec3  shading = vec3( 0.0 );
    bool pcf_enabled = per_frame_data.m_soft_shadows_config.x >= 0.5f;
    for( uint id_light = 0; id_light < per_frame_data.m_number_of_lights; id_light++ )
    {
        LightData light = per_frame_data.m_lights[ id_light ];
        uint light_type = uint( floor( light.m_light_pos.a ) );
        switch( light_type )
        {
            case 0: //directional
            {
                vec3 l = normalize( -light.m_light_pos.xyz );
                vec3 radiance = light.m_radiance.rgb;
                float visibility = evalVisibility(frag_pos, n, l, id_light, light_type, pcf_enabled);
                shading += max( dot( n, l ), 0.0 ) * albedo.rgb * radiance * visibility;
                break;
            }
            case 1: //point
            {
                vec3 l = light.m_light_pos.xyz - frag_pos;
                float dist = length( l );
                float att = 1.0 / (light.m_attenuattion.x + light.m_attenuattion.y * dist + light.m_attenuattion.z * dist * dist );
                vec3 radiance = light.m_radiance.rgb * att;
                l = normalize(l);
                float visibility = evalVisibility(frag_pos, n, l, id_light, light_type, pcf_enabled);
                shading += max( dot( n, l ), 0.0 ) * albedo.rgb * radiance * visibility;
                break;
            }
            case 2: //ambient
            {
                if(per_frame_data.m_ssao_data.enable_ao >= 0.5f){
                    vec4 ssao = texture(i_ssao, f_uvs);
                    vec3 direct = light.m_radiance.rgb * albedo.rgb * ssao.w;
                    vec3 indirect = ssao.rgb * albedo.rgb;
                    shading += direct + indirect;
                }
                else
                    shading += light.m_radiance.rgb * albedo.rgb;
                break;
            }
        }
    }
    return shading;
}

// ========================== MAIN ==========================
void main() 
{ 
    //Lectura de texturas multisample:
    ivec2 coord = ivec2(gl_FragCoord.xy);
    int   samples = textureSamples(i_albedo);
    vec3 albedo   = vec3(0.0);
    vec3 normal   = vec3(0.0);
    vec3 frag_pos = vec3(0.0);
    vec3 mat      = vec3(0.0);
    vec3 color    = vec3(0.0);
    for(int i = 0; i < samples; i++)
    {
        albedo   = texelFetch(i_albedo, coord, i).rgb;
        normal   = normalize(texelFetch(i_normal, coord, i).rgb * 2.0 - 1.0);
        frag_pos = texelFetch(i_position_and_depth, coord, i).rgb;
        mat      = texelFetch(i_material, coord, i).rgb;
        if(mat.x == 0) color += evalDiffuse(albedo, normal, frag_pos);
        else color += evalMicrofacet(mat, albedo, normal, frag_pos);
    }
    color /= samples;

    out_color = vec4(color, 1.0);
}