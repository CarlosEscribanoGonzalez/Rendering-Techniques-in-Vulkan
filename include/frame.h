#pragma once

#include "common.h"

namespace MiniEngine
{
    struct LightData
    {
        alignas( 16 ) Vector4f m_light_pos;
        alignas( 16 ) Vector4f m_radiance;
        alignas( 16 ) Vector4f m_attenuattion;
        alignas( 16 ) Matrix4f m_view_projection[ 6 ]; //6 is the max size (for cubemaps)
    };

    struct SSAOData {
        float enable_ao;
        float radius;
        float bias;
        float ssdo_enabled;
    };

    struct PerFrameData
    {
        alignas( 16 ) Vector4f m_camera_pos;
        alignas( 16 ) Matrix4f m_view;
        alignas( 16 ) Matrix4f m_projection;
        alignas( 16 ) Matrix4f m_view_projection;
        alignas( 16 ) Matrix4f m_inv_view;
        alignas( 16 ) Matrix4f m_inv_projection;
        alignas( 16 ) Matrix4f m_inv_view_projection;
        alignas( 16 ) Matrix4f m_prev_view;
        alignas( 16 ) Vector4f m_clipping_planes;
        //light info
        alignas( 16 ) LightData m_lights[ kMAX_NUMBER_LIGHTS ];
        alignas( 4 ) uint32_t  m_number_of_lights;
        //ssao
        alignas( 16 ) SSAOData m_ssao_data;
        //Tonemapping exposure
        alignas( 4 ) float m_exposure;
        //Shadowmap
        alignas( 16 ) Vector4f m_shadow_bias; //min, slope, max and tmin (rtx)
        alignas( 16 ) Vector4f m_cascade_splits; //To simplify, we assume there is 4 cascades max
        alignas( 16 ) Vector4f m_soft_shadows_config; //enabled, kernel size, num samples (rtx) and cone radius (rtx)
        alignas( 16 ) Vector4f m_reflections_config; //enabled, strength, unused and unused
        alignas( 4 ) unsigned int m_frame_index;
    };

    struct PerObjectData
    {
        //for now we only have material data
        alignas( 16 ) Matrix4f m_model;
        alignas( 16 ) Matrix4f m_prev_model;
        alignas( 16 ) Matrix4f m_inv_model;
        alignas( 16 ) Vector4f m_albedo; 
        alignas( 16 ) Vector4f m_metallic_roughness;
    };

    struct Frame
    {};
};