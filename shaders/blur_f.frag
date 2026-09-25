#version 460

#extension GL_ARB_shader_draw_parameters : enable
#extension GL_KHR_vulkan_glsl : enable
#define MAX_MASK_SIZE 21

layout( location = 0 ) in vec2 f_uvs;

layout ( set = 0, binding = 0) uniform sampler2D i_ssao;
layout(std140, set = 0, binding = 1) uniform MaskData {
    vec4 mask[MAX_MASK_SIZE * MAX_MASK_SIZE / 4];
    uint width;
} mask_data;

layout(location = 0) out vec4 out_blur;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(i_ssao, 0));
    vec4 result = vec4(0.0);
    int half_width = int(mask_data.width) / 2;
    for (int x = -half_width; x <= half_width; ++x) 
    {
        for (int y = -half_width; y <= half_width; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            int i = x + half_width + (y + half_width) * int(mask_data.width);
            float weight = mask_data.mask[i / 4][i % 4];
            result += texture(i_ssao, f_uvs + offset) * weight;
        }
    }
    
    out_blur = result;
} 