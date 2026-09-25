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
} per_frame_data;

struct ObjectData
{
    mat4 m_model;
    mat4 m_prev_model;
    mat4 m_inv_model;
    vec4 m_albedo; 
    vec4 m_metallic_roughness;
};

layout ( set = 0, binding = 1 ) uniform sampler2DMS i_albedo;
layout ( set = 0, binding = 2 ) uniform sampler2DMS i_position_and_depth;
layout ( set = 0, binding = 3 ) uniform sampler2DMS i_normal;
layout ( set = 0, binding = 4 ) uniform sampler2DMS i_material;
layout ( set = 0, binding = 5 ) uniform sampler2D i_ssao;
layout ( set = 0, binding = 6 ) uniform sampler2D i_shadows;
layout ( set = 0, binding = 7 ) uniform accelerationStructureEXT TLAS;
//all object matrices
layout(std140,set = 0, binding = 8) readonly buffer ObjectBufferData
{
    ObjectData objects[];
} per_object_data;


layout(location = 0) out vec4 out_color;

vec3 evalDiffuse(vec3 albedo, vec3 n, vec3 frag_pos);
void evalMicrofacet(vec3 mat, vec3 albedo, vec3 n, vec3 frag_pos, out vec3 diffuse, out vec3 spec, out vec3 ambient);

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

// ========================== RTX ==========================
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
    float cone_radius = per_frame_data.m_soft_shadows_config.w;
    float visibility = 0.0f;
    for(int i = 0; i < num_samples; i++){
        float angle = float(i) * 2.3999; //Golden ratio en radianes
        float r = cone_radius * sqrt(float(i) / float(num_samples));
        vec3 offset = (cos(angle) * tangent + sin(angle) * bitangent) * r;
        vec3 perturbed_dir = normalize(l + offset);
        if(!rayIntersects(frag_pos, n, perturbed_dir, dist))
            visibility += 1.0;
    }
    return visibility / float(num_samples);
}

vec3 computeReflection(vec3 origin, vec3 n, vec3 v)
{
    //Cálculo del vector reflejado:
    vec3 R = normalize(reflect(-v, n));
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(
        rayQuery, 
        TLAS,
        gl_RayFlagsOpaqueEXT, //flags - ignora any-hit shaders
        0xFF, //Mask
        origin + n * per_frame_data.m_shadow_bias.w, //Posición inicial
        per_frame_data.m_shadow_bias.w, //tMin
        R,
        100
    );
    rayQueryProceedEXT(rayQuery);
    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT)
        return vec3(0.0f);
    //Obtención datos intersección:
    uint instanceID = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, true);
    float t = rayQueryGetIntersectionTEXT(rayQuery, true);
    vec3 hit_pos = origin + R * t;
    vec4 mat = per_object_data.objects[instanceID].m_metallic_roughness;
    vec3 albedo = per_object_data.objects[instanceID].m_albedo.rgb;
    //Cálculo de sombras duras:
    float totalEnergy = 0;
    float shadowedEnergy = 0;
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
            l = light.m_light_pos.xyz - hit_pos;
            dist = length(l);
            l = normalize(l);
            float att = 1.0 / (light.m_attenuattion.x + light.m_attenuattion.y * dist + light.m_attenuattion.z * dist * dist );
            radiance = light.m_radiance.rgb * att;
        } 
        else {
            float lum = dot(light.m_radiance.rgb, vec3(0.2126, 0.7152, 0.0722));
            totalEnergy += lum;
            shadowedEnergy += lum;
            continue;
        }
        float L = dot(radiance, vec3(0.2126, 0.7152, 0.0722));
        totalEnergy += L;
        shadowedEnergy += L * evalVisibility(hit_pos, l, l, dist, id_light);
    }
    float visibility = shadowedEnergy / totalEnergy;
    //Cálculo del color final:
    if(floor(mat.x) == 0) return dot(R, n) * evalDiffuse(albedo, R, hit_pos) * visibility;
    vec3 diffuse, spec, ambient;
    evalMicrofacet(mat.rgb, albedo, R, hit_pos, diffuse, spec, ambient);
    return dot(R, n) * (spec + diffuse) * visibility + ambient;
}

// ========================== SHADING ==========================
vec3 capSpecular(vec3 spec, float maxLum) 
{
    float lum = dot(spec, vec3(0.2126, 0.7152, 0.0722));
    return lum > maxLum ? spec * (maxLum / lum) : spec;
}

void evalMicrofacet(vec3 mat, vec3 albedo, vec3 n, vec3 frag_pos, out vec3 diffuse, out vec3 spec, out vec3 ambient)
{
    vec3 v = normalize(per_frame_data.m_camera_pos.xyz - frag_pos);
    vec3 baseDiffuse = albedo / PI;
    diffuse = vec3(0.0f);
    spec = vec3(0.0f);
    ambient = vec3(0.0f);
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
                spec += evalSpec(l, v, n, h, albedo, mat) * radiance;
                vec3 kd = (vec3(1.0) - F(mat.y, albedo, v, h)) * (1.0 - mat.y);
                diffuse += kd * baseDiffuse * max(dot(n, l), 0.0) * radiance;
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
                spec += evalSpec(l, v, n, h, albedo, mat) * radiance;
                vec3 kd = (vec3(1.0) - F(mat.y, albedo, v, h)) * (1.0 - mat.y);
                diffuse += kd * baseDiffuse * max(dot(n, l), 0.0) * radiance;
                break;
            }
            case 2:
            {
                if(per_frame_data.m_ssao_data.enable_ao >= 0.5f){
                    vec4 ssao = texture(i_ssao, f_uvs);
                    vec3 direct = light.m_radiance.rgb * albedo.rgb * ssao.w;
                    vec3 indirect = ssao.rgb * albedo.rgb;
                    ambient += direct + indirect;
                }
                else
                    ambient += light.m_radiance.rgb * albedo.rgb;
                break;
            }
        }
    }
    spec = capSpecular(spec, 10.0); //Evita sobre saturaciones cuando metálico es muy alto y roughness muy bajo
}

vec3 evalMicrofacet_reflection(vec3 mat, vec3 albedo, vec3 n, vec3 frag_pos)
{   
    float shadows = texture(i_shadows, f_uvs).x;
    vec3 diffuse, spec, ambient;
    evalMicrofacet(mat, albedo, n, frag_pos, diffuse, spec, ambient);
    if(int(floor(per_frame_data.m_reflections_config.x)) != 0)
    {
        vec3 v = normalize(per_frame_data.m_camera_pos.xyz - frag_pos);
        float t = mat.y * (1.0 - mat.z) * (1 - dot(v, n)) * per_frame_data.m_reflections_config.y;
        spec = mix(spec, computeReflection(frag_pos, n, v), clamp(t, 0, 1));
    }
    return (diffuse + spec) * shadows + ambient;
}

vec3 evalDiffuse(vec3 albedo, vec3 n, vec3 frag_pos)
{
    vec3  shading = vec3( 0.0 );
    vec3  ambient = vec3( 0.0 );
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
                shading += max( dot( n, l ), 0.0 ) * albedo.rgb * radiance;
                break;
            }
            case 1: //point
            {
                vec3 l = light.m_light_pos.xyz - frag_pos;
                float dist = length( l );
                float att = 1.0 / (light.m_attenuattion.x + light.m_attenuattion.y * dist + light.m_attenuattion.z * dist * dist );
                vec3 radiance = light.m_radiance.rgb * att;
                l = normalize(l);
                shading += max( dot( n, l ), 0.0 ) * albedo.rgb * radiance;
                break;
            }
            case 2: //ambient
            {
                if(per_frame_data.m_ssao_data.enable_ao >= 0.5f){
                    vec4 ssao = texture(i_ssao, f_uvs);
                    vec3 direct = light.m_radiance.rgb * albedo.rgb * ssao.w;
                    vec3 indirect = ssao.rgb * albedo.rgb;
                    ambient += direct + indirect;
                }
                else
                    ambient += light.m_radiance.rgb * albedo.rgb;
                break;
            }
        }
    }
    float shadows = texture(i_shadows, f_uvs).x;
    return shading * shadows + ambient;
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
        else color += evalMicrofacet_reflection(mat, albedo, normal, frag_pos);
    }
    color /= samples;

    out_color = vec4(color, 1.0);
}