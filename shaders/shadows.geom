#version 460

#extension GL_ARB_shader_draw_parameters : enable
const int MAX_CASCADES = 4;
const int MAX_LAYERS = 6; //Omnidireccionales
const int MAX_LIGHTS = 10;

layout(triangles) in;
layout(triangle_strip, max_vertices = 3 * MAX_LAYERS * MAX_LIGHTS) out;

layout( location = 0 ) in vec3 g_position[];

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
    LightData m_lights[ MAX_LIGHTS ];
    uint      m_number_of_lights;
} per_frame_data;

void main() 
{    
    //Por cada luz:
    for (int i = 0; i < per_frame_data.m_number_of_lights; ++i) {
        int num_proj = int(floor(per_frame_data.m_lights[i].m_light_pos.w)) == 0 ? MAX_CASCADES : MAX_LAYERS;
        //Por cada layer:
        for(int c = 0; c < num_proj; c++){
            gl_Layer = i * MAX_LAYERS + c;
            //Por cada vértice:
            for(int v = 0; v < 3; v++){
                gl_Position = per_frame_data.m_lights[i].m_view_projection[c] * vec4(g_position[v], 1.0);
                EmitVertex();
            }
            EndPrimitive();
        }

    }
}