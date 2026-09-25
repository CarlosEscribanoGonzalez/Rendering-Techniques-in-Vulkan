    #version 460

    #extension GL_ARB_shader_draw_parameters : enable
    #extension GL_KHR_vulkan_glsl : enable
    #define KERNEL_SIZE 64
    #define NOISE_SIZE 16

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
    } per_frame_data;

    layout ( set = 0, binding = 1 ) uniform sampler2DMS i_position_and_depth;
    layout ( set = 0, binding = 2 ) uniform sampler2DMS i_normal;
    layout ( set = 0, binding = 3 ) uniform sampler2DMS i_color;

    layout ( std140, set = 0, binding = 4 ) uniform KernelSamples
    {
        vec4 samples[KERNEL_SIZE];
    } kernel_samples;
    layout ( std140, set = 0, binding = 5 ) uniform NoiseSamples
    {
        vec4 samples[NOISE_SIZE];
    } noise_samples;

    layout(location = 0) out vec4 out_occlusion;

    const float COLOR_BLEEDING_INTENSITY = 0.2f;

void main() 
{ 
    ivec2 coord = ivec2(gl_FragCoord.xy);
    int samples = textureSamples(i_position_and_depth);
    float occlusion = 0.0;
    float total_weight = 0.0;
    vec3 color = vec3(0.0);
    for(int i = 0; i < samples; i++) {
        //Posición en view:
        vec3 pos_world = texelFetch(i_position_and_depth, coord, i).xyz;
        vec3 pos_view = (per_frame_data.m_view * vec4(pos_world, 1.0)).xyz;
        //Normal:
        vec3 normal_world = texelFetch(i_normal, coord, i).xyz * 2.0f - 1.0f;
        vec3 normal_view = normalize(mat3(per_frame_data.m_view) * normal_world);
        //Noise:
        ivec2 noise_coord = ivec2(mod(gl_FragCoord.xy, 4.0));
        vec3 random_vec = normalize(noise_samples.samples[noise_coord.y * 4 + noise_coord.x].xyz);
        //TBN:
        vec3 tangent = normalize(random_vec - normal_view * dot(random_vec, normal_view));
        vec3 bitangent = cross(normal_view, tangent);
        mat3 TBN = mat3(tangent, bitangent, normal_view);
        //Cálculo oclusión:
        for(int j = 0; j < KERNEL_SIZE; ++j)
        {
            vec3 sampleOffset = TBN * kernel_samples.samples[j].xyz;
            vec4 offset_view = vec4(pos_view + sampleOffset * per_frame_data.m_ssao_data.radius, 1.0);
            vec4 offset_proj = per_frame_data.m_projection * offset_view;
            offset_proj.xyz /= offset_proj.w;
            offset_proj.xyz = offset_proj.xyz * 0.5 + 0.5;
            ivec2 sample_coord = ivec2(offset_proj.xy * vec2(textureSize(i_position_and_depth)));
            sample_coord = clamp(sample_coord, ivec2(0), textureSize(i_position_and_depth) - ivec2(1));
            float sampleDepth = (per_frame_data.m_view * vec4(texelFetch(i_position_and_depth, sample_coord, i).xyz, 1.0)).z;
            float rangeCheck = smoothstep(0.0, 1.0, per_frame_data.m_ssao_data.radius / abs(pos_view.z - sampleDepth));
            float weight = max(0.0, 1.0 - distance(pos_view, offset_view.xyz) / per_frame_data.m_ssao_data.radius);
            float sample_occlusion = (sampleDepth >= offset_view.z - per_frame_data.m_ssao_data.bias ? weight : 0.0) * rangeCheck;
            occlusion += sample_occlusion;
            total_weight += weight;
            //SSDO:
            if(per_frame_data.m_ssao_data.ssdo_factor < 0.001) continue;
            vec3 sample_albedo = texelFetch(i_color, sample_coord, i).xyz;
            vec3 sampleDir = normalize(offset_view.xyz - pos_view);
            sample_albedo *= sample_occlusion * max(dot(normal_view, sampleDir), 0.0) * rangeCheck;
            color += sample_albedo;
        }
    }
    total_weight = max(total_weight, 0.0001);
    color *= per_frame_data.m_ssao_data.ssdo_factor * COLOR_BLEEDING_INTENSITY / total_weight;
    out_occlusion = vec4(color, 1.0 - (occlusion / total_weight));
}