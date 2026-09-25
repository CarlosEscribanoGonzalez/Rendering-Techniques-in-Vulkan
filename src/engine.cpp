
//engine includes
#include "engine.h"
#include "frame.h"
#include "meshRegistry.h"
#include "shaderRegistry.h"
#include "scene.h"
#include "camera.h"
#include "light.h"
#include "entity.h"
#include "material.h"
#include "diffuse.h"
#include "microfacets.h"
#include "input.h"

// vulkan includes
#include "vulkan/rendererVK.h"
#include "vulkan/renderPassVK.h"
#include "vulkan/deferredPassVK.h"
#include "vulkan/depthPassVK.h"
#include "vulkan/ssaoPassVK.h"
#include "vulkan/shadowPassVK.h"
#include "vulkan/shadowPassVK_RTX.h"
#include "vulkan/motionVectorsPassVK.h"
#include "vulkan/blurPassVK.h"
#include "vulkan/denoisePassVK.h"
#include "vulkan/lightingPassVK.h"
#include "vulkan/compositionPassVK.h"
#include "vulkan/taaPassVK.h"
#include "vulkan/ppPassVK.h"
#include "vulkan/windowVK.h"
#include "vulkan/deviceVK.h"
#include "vulkan/utilsVK.h"

using namespace MiniEngine;

namespace
{
    Engine* m_instance = nullptr;
}

Engine& Engine::instance()
{
    if( !m_instance )
    {
        m_instance = new Engine();
    }

    return *m_instance;
}


Engine::Engine() : 
    m_current_frame( 0     ),
    m_close        ( false ),
    m_resize       ( false )
{

}


Engine::~Engine()
{

}

unsigned int pp_attachment_idx = 0;

void updatePPIdx() {
    pp_attachment_idx = pp_attachment_idx == 0 ? 1 : 0;
}

bool Engine::initialize()
{
    //init vulkan 
    m_runtime.m_renderer = std::make_unique<RendererVK>();
    RendererVK& renderer = *m_runtime.m_renderer;

    renderer.initialize();

    m_runtime.m_mesh_registry   = std::make_unique<MeshRegistry  >( m_runtime );
    m_runtime.m_shader_registry = std::make_unique<ShaderRegistry>( m_runtime );

    m_runtime.m_mesh_registry->initialize();
    m_runtime.m_shader_registry->initialize();

    createSyncObjects ();

    return true;
}


void Engine::run()
{
    RendererVK& renderer = *m_runtime.m_renderer;

    bool loop = true;
    while( loop && m_scene ) 
    {
        uint32_t clamped_idx = m_current_frame % 3;
        renderer.getWindow().prepareFrame( m_frame_semaphore[ clamped_idx ].m_presentation_semaphore );

        vkWaitForFences( renderer.getDevice()->getLogicalDevice(), 1, &m_frame_fence[ clamped_idx ], VK_TRUE, 1000000000 );
        
        processInput(renderer.getWindow().getWindow());
        //update global uniforms buffers 
        updateGlobalBuffers(); 

        //prepare pipeline stages
        VkSubmitInfo submit_info{};
        VkPipelineStageFlags wait_stage     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        submit_info.sType                   = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext                   = nullptr;
        submit_info.pWaitDstStageMask       = &wait_stage;
        submit_info.waitSemaphoreCount      = 1;
        submit_info.pWaitSemaphores         = &m_frame_semaphore[ clamped_idx ].m_presentation_semaphore;
        submit_info.signalSemaphoreCount    = 1;
        submit_info.pSignalSemaphores       = &m_frame_semaphore[ clamped_idx ].m_render_semaphore;

        // draw render passes
        std::vector<VkCommandBuffer> cmds;
        for( auto& pass : m_render_passes )
        {
            auto cmd = pass->draw({});
            if(cmd != VK_NULL_HANDLE)
                cmds.push_back( cmd );
        }

        submit_info.commandBufferCount = static_cast<uint32_t>(cmds.size());
        submit_info.pCommandBuffers    = cmds.data();

        vkResetFences  ( renderer.getDevice()->getLogicalDevice(), 1, &m_frame_fence[ clamped_idx ] );

        vkQueueSubmit( renderer.getDevice()->getGraphicsQueue(), 1, &submit_info, m_frame_fence[ clamped_idx ] );

        uint32_t result = renderer.getWindow().renderFrame( m_frame_semaphore[ clamped_idx ].m_render_semaphore );
        
        vkQueueWaitIdle( renderer.getDevice()->getGraphicsQueue() );

        //
        //check if we need to resize the window               
        // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
        if( ( result == VK_ERROR_OUT_OF_DATE_KHR ) || ( result == VK_SUBOPTIMAL_KHR ) )
        {
           renderer.getWindow().wait();

            vkDeviceWaitIdle( renderer.getDevice()->getLogicalDevice() );
                       
            destroySamplers    ();
            destroyRenderPasses();
            destroyAttachments ();
            destroySyncObjects ();
            renderer.getWindow ().resize();        

            createSyncObjects ();
            createSamplers    ();
            createAttachments ();
            createRenderPasses();
        }                   


        m_current_frame++;
        //check if the window is closed and poll input events
        loop = renderer.getWindow().loop();
        
        //TLAS update
        m_runtime.updateTLAS();
    }
}


void Engine::shutdown()
{
    RendererVK& renderer = *m_runtime.m_renderer;


    vkDeviceWaitIdle( renderer.getDevice()->getLogicalDevice() );
    
    m_runtime.freeResources();

    if( m_scene )
    {
        m_scene->shutdown();
    }

    destroyRenderPasses();
    destroyAttachments ();
    destroySamplers    ();
    destroySyncObjects ();

    m_runtime.m_mesh_registry->shutdown();
    m_runtime.m_shader_registry->shutdown();

    m_runtime.m_renderer->shutdown();
}


void Engine::loadScene( const std::string& i_path )
{
    m_scene = Scene::loadScene( m_runtime, i_path );

    assert( m_scene );

    if( !m_render_passes.empty() )
    {
        destroySamplers    ();
        destroyAttachments ();
        destroyRenderPasses();
    }
    else //create uniform buffers just once
    {
        m_runtime.createResources();
    }

    createSamplers    ();
    createAttachments ();
    createRenderPasses();

    RendererVK& renderer = *m_runtime.m_renderer;
    renderer.getWindow().resize( m_scene->getCamera().getWidth(), m_scene->getCamera().getHeight() );

}


void Engine::createSyncObjects()                                  
{
    RendererVK& renderer = *m_runtime.m_renderer;

    //create sync objects
    for( uint32_t idx = 0; idx < 3; idx++ )
    {
        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if( vkCreateSemaphore( renderer.getDevice()->getLogicalDevice(), &semaphore_info, nullptr, &m_frame_semaphore[ idx ].m_render_semaphore ) )
        {
            throw MiniEngineException( "Cannot create render semaphore" );
        }
        
        if( vkCreateSemaphore( renderer.getDevice()->getLogicalDevice(), &semaphore_info, nullptr, &m_frame_semaphore[ idx ].m_presentation_semaphore ) )
        {
            throw MiniEngineException( "Cannot create presentation semaphore" );
        }

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        if( vkCreateFence( renderer.getDevice()->getLogicalDevice(), &fence_info, nullptr, &m_frame_fence[ idx ] ) )
        {
            throw MiniEngineException( "Cannot create fence" );
        }
    }
}


void Engine::destroySyncObjects()
{
    RendererVK& renderer = *m_runtime.m_renderer;

    for( uint32_t idx = 0; idx < 3; idx++ )
    {
        vkDestroySemaphore( renderer.getDevice()->getLogicalDevice(), m_frame_semaphore[ idx ].m_render_semaphore      , nullptr );
        vkDestroySemaphore( renderer.getDevice()->getLogicalDevice(), m_frame_semaphore[ idx ].m_presentation_semaphore, nullptr );
        vkDestroyFence    ( renderer.getDevice()->getLogicalDevice(), m_frame_fence[ idx ]                             , nullptr );
    }
}


void Engine::createRenderPasses ()
{ 
    pp_attachment_idx = 0;
    //temp
    for( auto pass : m_render_passes )
    {
        pass->shutdown();
    }
    m_render_passes.clear();

    //Geometry:
    auto depth_pass = std::make_shared<DepthPassVK>(
        m_runtime,
        m_render_target_attachments.m_depth_attachment);
    depth_pass->initialize();
    m_render_passes.push_back(depth_pass);

    auto gbuffer_pass = std::make_shared<DeferredPassVK>(
        m_runtime,
        m_render_target_attachments.m_depth_attachment,
        m_render_target_attachments.m_color_attachment,
        m_render_target_attachments.m_normal_attachment,    
        m_render_target_attachments.m_position_depth_attachment,
        m_render_target_attachments.m_material_attachment);
    gbuffer_pass->initialize();
    m_render_passes.push_back(gbuffer_pass); 

    auto motion_pass = std::make_shared<MotionVectorsPassVK>(
        m_runtime,
        m_render_target_attachments.m_position_depth_attachment,
        m_render_target_attachments.m_material_attachment,
        m_render_target_attachments.m_motionVectors_attachment);
    motion_pass->initialize();
    m_render_passes.push_back(motion_pass);

    //AO:
    if (runtimeVariables.ao_enabled) {
        auto ssao_pass = std::make_shared<SSAOPassVK>(m_runtime,
            m_render_target_attachments.m_position_depth_attachment,
            m_render_target_attachments.m_normal_attachment,
            m_render_target_attachments.m_color_attachment,
            m_render_target_attachments.m_ssao_attachment);
        ssao_pass->initialize();
        m_render_passes.push_back(ssao_pass);

        auto ssao_blur_pass = std::make_shared<BlurPassVK>(m_runtime,
            m_render_target_attachments.m_ssao_attachment,
            m_render_target_attachments.m_ssao_blur_attachment,
            BlurType::Gauss3x3);
        ssao_blur_pass->initialize();
        m_render_passes.push_back(ssao_blur_pass);
    }
    
    //Shadows:
    if (runtimeVariables.shadowsType == ShadowsType::ShadowMap) 
    {
        auto shadow_pass = std::make_shared<ShadowPassVK>(m_runtime,
            m_render_target_attachments.m_shadow_attachment);
        shadow_pass->initialize();
        m_render_passes.push_back(shadow_pass);
    }
    else
    {
        bool isDenoised = runtimeVariables.shadowsType == ShadowsType::RTX_Denoised;
        auto shadow_pass = std::make_shared<ShadowPassVK_RTX>(m_runtime,
            m_render_target_attachments.m_position_depth_attachment,
            m_render_target_attachments.m_normal_attachment,
            isDenoised ? m_render_target_attachments.m_visibility_attachment : 
                m_render_target_attachments.m_shadow_attachment
        );
        shadow_pass->initialize();
        m_render_passes.push_back(shadow_pass);

        if (isDenoised) {
            auto denoise_pass = std::make_shared<DenoisePassVK>(m_runtime,
                m_render_target_attachments.m_visibility_attachment,
                m_render_target_attachments.m_position_depth_attachment,
                m_render_target_attachments.m_normal_attachment,
                m_render_target_attachments.m_denoise_history_attachment,
                m_render_target_attachments.m_motionVectors_attachment,
                m_render_target_attachments.m_shadow_attachment,
                BlurType::Gauss13x13);
            denoise_pass->initialize();
            m_render_passes.push_back(denoise_pass);
        }
    }

    //Lighting:
    auto lighting_pass = std::make_shared<LightingPassVK>(m_runtime,
        m_render_target_attachments.m_color_attachment,
        m_render_target_attachments.m_position_depth_attachment,
        m_render_target_attachments.m_normal_attachment,
        m_render_target_attachments.m_material_attachment,
        m_render_target_attachments.m_ssao_blur_attachment,
        m_render_target_attachments.m_shadow_attachment,
        m_render_target_attachments.m_lighting_attachment);
    lighting_pass->initialize();
    m_render_passes.push_back(lighting_pass);
    
    ImageBlock* lastPassOutput = &m_render_target_attachments.m_lighting_attachment;

    //Post processing:
    if (runtimeVariables.bloom) {
        auto bloom_pass = std::make_shared<PPPassVK>(m_runtime,
            std::vector<ImageBlock*>{ lastPassOutput },
            m_render_target_attachments.m_pp_attachments[pp_attachment_idx],
            PostProcessType::Bloom);
        bloom_pass->initialize();
        m_render_passes.push_back(bloom_pass);
        lastPassOutput = &m_render_target_attachments.m_pp_attachments[pp_attachment_idx];
        updatePPIdx();

        auto bloom_blur_pass = std::make_shared<BlurPassVK>(m_runtime,
            *lastPassOutput,
            m_render_target_attachments.m_pp_attachments[pp_attachment_idx],
            BlurType::Gauss13x13);
        bloom_blur_pass->initialize();
        m_render_passes.push_back(bloom_blur_pass);
        lastPassOutput = &m_render_target_attachments.m_pp_attachments[pp_attachment_idx];
        updatePPIdx();

        auto bloom_add_pass = std::make_shared<PPPassVK>(m_runtime,
            std::vector<ImageBlock*>{ lastPassOutput,
            & m_render_target_attachments.m_lighting_attachment},
            m_render_target_attachments.m_pp_attachments[pp_attachment_idx],
            PostProcessType::ColorAdd);
        bloom_add_pass->initialize();
        m_render_passes.push_back(bloom_add_pass);
        lastPassOutput = &m_render_target_attachments.m_pp_attachments[pp_attachment_idx];
        updatePPIdx();
    }

    if (runtimeVariables.tonemapping) {
        auto tonemapping_pass = std::make_shared<PPPassVK>(m_runtime,
            std::vector<ImageBlock*>{ lastPassOutput },
            m_render_target_attachments.m_pp_attachments[pp_attachment_idx],
            PostProcessType::ToneMapping);
        tonemapping_pass->initialize();
        m_render_passes.push_back(tonemapping_pass);
        lastPassOutput = &m_render_target_attachments.m_pp_attachments[pp_attachment_idx];
        updatePPIdx();
    }

    if(runtimeVariables.chromaticAberration) {
        auto aberration_pass = std::make_shared<PPPassVK>(m_runtime,
            std::vector<ImageBlock*>{ lastPassOutput },
            m_render_target_attachments.m_pp_attachments[pp_attachment_idx],
            PostProcessType::ChromaticAberration);
        aberration_pass->initialize();
        m_render_passes.push_back(aberration_pass);
        lastPassOutput = &m_render_target_attachments.m_pp_attachments[pp_attachment_idx];
        updatePPIdx();
    }

    //Anti aliasing:
    if (runtimeVariables.antiAliasing == AntiAliasingType::TAA) {
        auto taa_pass = std::make_shared<TAAPassVK>(m_runtime,
            *lastPassOutput,
            m_render_target_attachments.m_taa_history_attachment,
            m_render_target_attachments.m_motionVectors_attachment,
            m_render_target_attachments.m_pp_attachments[pp_attachment_idx]);
        taa_pass->initialize();
        m_render_passes.push_back(taa_pass);
        lastPassOutput = &m_render_target_attachments.m_pp_attachments[pp_attachment_idx];
        updatePPIdx();
    }
    else if (runtimeVariables.antiAliasing == AntiAliasingType::FXAA) {
        auto fxaa_pass = std::make_shared<PPPassVK>(m_runtime,
            std::vector<ImageBlock*>{ lastPassOutput },
            m_render_target_attachments.m_pp_attachments[pp_attachment_idx],
            PostProcessType::FXAA);
        fxaa_pass->initialize();
        m_render_passes.push_back(fxaa_pass);
        lastPassOutput = &m_render_target_attachments.m_pp_attachments[pp_attachment_idx];
        updatePPIdx();
    }

    //Composition:
    auto composition_pass = std::make_shared<CompositionPassVK>(m_runtime,
        *lastPassOutput,
        m_runtime.m_renderer->getWindow().getSwapChainImages()
    );
    composition_pass->initialize();
    m_render_passes.push_back(composition_pass);

    if( m_scene )
    {
        for( auto pass : m_render_passes )
        {
            for( auto entity : m_scene->getMeshes() )
            {
                pass->addEntityToDraw( entity );
            }
        }
    }
}


void Engine::destroyRenderPasses()
{
    RendererVK& renderer = *m_runtime.m_renderer;

    for( auto pass : m_render_passes )
    {
        pass->shutdown();
        pass = nullptr;
    }

    m_render_passes.clear();
}

float halton(int index, int base) {
    float f = 1.0f;
    float r = 0.0f;
    while (index > 0) {
        f /= base;
        r += f * (index & base);
        index /= base;
    }
    return r;
}

void Engine::updateGlobalBuffers()
{
    assert( m_runtime.m_per_frame_buffer[ m_current_frame % 3 ] );
    assert( m_scene );

    //global settings
    PerFrameData perframe_data;
    Vector3f cam_pos = m_scene->getCamera().getCameraPos();
    perframe_data.m_camera_pos          = Vector4f( cam_pos.x, cam_pos.y, cam_pos.z, 0.0f );
    perframe_data.m_projection          = const_cast< Camera& >( m_scene->getCamera() ).getProjection();
    if (runtimeVariables.antiAliasing == AntiAliasingType::TAA) { //Camera jitter for TAA
        uint32_t width, height;
        m_runtime.m_renderer->getWindow().getWindowSize(width, height);
        float jitterX = (halton(m_current_frame, 2) - 0.5f) / width;
        float jitterY = (halton(m_current_frame, 3) - 0.5f) / height;
        perframe_data.m_projection[2][0] += jitterX;
        perframe_data.m_projection[2][1] += jitterY;
    }
    Camera& cam = const_cast<Camera&>(m_scene->getCamera());
    perframe_data.m_view                = cam.getView();
    perframe_data.m_prev_view           = m_current_frame == 0 ? cam.getView() : m_prev_view;
    perframe_data.m_view_projection     = cam.getViewProjection();
    perframe_data.m_inv_projection      = glm::inverse( perframe_data.m_projection          );
    perframe_data.m_inv_view            = glm::inverse( perframe_data.m_view                );
    perframe_data.m_inv_view_projection = glm::inverse( perframe_data.m_view_projection     );
    perframe_data.m_clipping_planes     = Vector4f( m_scene->getCamera().getNearPlane(), m_scene->getCamera().getFarPlane(), 0.0f, 0.0f );
    perframe_data.m_number_of_lights    = 0;
    m_prev_view = perframe_data.m_view;

    for( perframe_data.m_number_of_lights = 0; perframe_data.m_number_of_lights < m_scene->getLights().size() && perframe_data.m_number_of_lights < kMAX_NUMBER_LIGHTS; perframe_data.m_number_of_lights++ )
    {
        assert( perframe_data.m_number_of_lights < kMAX_NUMBER_LIGHTS );
       
        auto light = m_scene->getLights()[ perframe_data.m_number_of_lights ];

        perframe_data.m_lights[ perframe_data.m_number_of_lights ].m_light_pos    = Vector4f( light->m_data.m_position.x   , light->m_data.m_position.y   , light->m_data.m_position.z   , light->m_data.m_type );
        perframe_data.m_lights[ perframe_data.m_number_of_lights ].m_radiance     = Vector4f( light->m_data.m_radiance.x   , light->m_data.m_radiance.y   , light->m_data.m_radiance.z   , 0.0f                 );
        perframe_data.m_lights[ perframe_data.m_number_of_lights ].m_attenuattion = Vector4f( light->m_data.m_attenuation.x, light->m_data.m_attenuation.y, light->m_data.m_attenuation.z, light->m_data.m_far  );
        //CSM:
        Vector4f cascade_splits = cam.getCascadeSplits();
        perframe_data.m_cascade_splits = cascade_splits;
        if (light->m_data.m_type == Light::LightType::Directional) {
            float n = cam.getNearPlane(); 
            float f = cam.getFarPlane();
            float prevSplit = n;
            for (int i = 0; i < runtimeVariables.num_cascades; i++) {
                //Last split should reach far plane
                cam.setClippingPlanes(prevSplit, i == runtimeVariables.num_cascades - 1 ? f : cascade_splits[i]);
                prevSplit = cascade_splits[i];
                perframe_data.m_lights[perframe_data.m_number_of_lights].m_view_projection[i] =
                    Light::getLightSpaceMatrix_Dir(light, cam);
            }
            //Camera configuration resets
            cam.setClippingPlanes(n, f);
        }
        //Omnidirectional:
        else if (light->m_data.m_type == Light::LightType::Point) {
            std::array<Matrix4f, 6> matrix_cube = Light::getLightSpaceMatrix_Point(light, cam);
            for (int i = 0; i < 6; i++) {
                perframe_data.m_lights[perframe_data.m_number_of_lights].m_view_projection[i] = matrix_cube[i];
            }
        }
    }

    //AO
    perframe_data.m_ssao_data.enable_ao = runtimeVariables.ao_enabled ? 1 : 0;
    perframe_data.m_ssao_data.radius = runtimeVariables.ao_radius;
    perframe_data.m_ssao_data.bias = runtimeVariables.ao_bias;
    perframe_data.m_ssao_data.ssdo_enabled = runtimeVariables.ssdo_factor;
    perframe_data.m_exposure = runtimeVariables.exposure;
    //Shadows:
    perframe_data.m_shadow_bias = runtimeVariables.shadow_bias;
    perframe_data.m_soft_shadows_config = runtimeVariables.soft_shadows_config;
    //Reflections:
    perframe_data.m_reflections_config = runtimeVariables.reflection_config;

    perframe_data.m_frame_index = m_current_frame;

    //material buffers
    void* data;
    vkMapMemory( m_runtime.m_renderer->getDevice()->getLogicalDevice(), m_runtime.m_per_frame_buffer_memory[ m_current_frame  % 3 ], 0, sizeof( PerFrameData ), 0, &data );

    memcpy( data, &perframe_data, sizeof( PerFrameData ) );

    vkUnmapMemory( m_runtime.m_renderer->getDevice()->getLogicalDevice(), m_runtime.m_per_frame_buffer_memory[ m_current_frame % 3 ] );
    
    for( uint32_t idx = 0; idx < m_scene->getMeshes().size(); idx++ )
    {
        PerObjectData* data_object;
        std::shared_ptr<Entity> entity = m_scene->getMeshes()[ idx ];
        
        vkMapMemory( m_runtime.m_renderer->getDevice()->getLogicalDevice(), m_runtime.m_per_object_buffer_memory[ m_current_frame % 3 ], sizeof( PerObjectData ) * idx, sizeof( PerObjectData ), 0, reinterpret_cast<void**>( &data_object ) );

        data_object->m_prev_model = m_current_frame == 0 ? 
            entity->getTransform().getTransform() : entity->getPrevTransform();
        data_object->m_model = entity->getTransform().getTransform();
        data_object->m_inv_model = entity->getTransform().getInverseTransform();
        entity->advanceFrame();

        switch( entity->getMaterial().getType() )
        {
            case Material::TMaterial::Diffuse:
            {
                Diffuse& diffuse = reinterpret_cast<Diffuse&>( entity->getMaterial() );
                data_object->m_albedo  = Vector4f( diffuse.getData().m_albedo.x, diffuse.getData().m_albedo.y, diffuse.getData().m_albedo.z, 0.0f );

                break;
            }
            case Material::TMaterial::Microfacets: 
            {
                Microfacets& microfacets = reinterpret_cast<Microfacets&>( entity->getMaterial() );
                data_object->m_albedo             = Vector4f( microfacets.getData().m_albedo.x, microfacets.getData().m_albedo.y , microfacets.getData().m_albedo.z, 0.0f );
                data_object->m_metallic_roughness = Vector4f( microfacets.getData().m_metallic, microfacets.getData().m_roughness,                             0.0f, 0.0f );
                break;
            }
        }        

        vkUnmapMemory( m_runtime.m_renderer->getDevice()->getLogicalDevice(), m_runtime.m_per_object_buffer_memory[ m_current_frame % 3 ] );
    }
}


void Engine::createAttachments()
{
    uint32_t width, height;

    m_runtime.m_renderer->getWindow().getWindowSize( width, height );

    int sampleCount = UtilsVK::getMultisampleCount_AA();
    UtilsVK::createImage( *m_runtime.m_renderer->getDevice(), VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT        , width, height, m_render_target_attachments.m_color_attachment          , sampleCount);
    UtilsVK::createImage( *m_runtime.m_renderer->getDevice(), VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT        , width, height, m_render_target_attachments.m_normal_attachment         , sampleCount);
    UtilsVK::createImage( *m_runtime.m_renderer->getDevice(), VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT        , width, height, m_render_target_attachments.m_position_depth_attachment , sampleCount);
    UtilsVK::createImage( *m_runtime.m_renderer->getDevice(), VK_FORMAT_R16G16_SFLOAT      , VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT        , width, height, m_render_target_attachments.m_motionVectors_attachment  );
    UtilsVK::createImage( *m_runtime.m_renderer->getDevice(), VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT        , width, height, m_render_target_attachments.m_material_attachment       , sampleCount);
    UtilsVK::createImage( *m_runtime.m_renderer->getDevice(), VK_FORMAT_D32_SFLOAT_S8_UINT , VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, width, height, m_render_target_attachments.m_depth_attachment          , sampleCount);
    UtilsVK::createImage( *m_runtime.m_renderer->getDevice(), VK_FORMAT_R8G8B8A8_UNORM     , VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT        , width, height, m_render_target_attachments.m_ssao_attachment           );
    UtilsVK::createImage( *m_runtime.m_renderer->getDevice(), VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT        , width, height, m_render_target_attachments.m_ssao_blur_attachment      );
    UtilsVK::createImage( *m_runtime.m_renderer->getDevice(), VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT        , width, height, m_render_target_attachments.m_lighting_attachment       );
    UtilsVK::createImage( *m_runtime.m_renderer->getDevice(), VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT        , width, height, m_render_target_attachments.m_taa_history_attachment    );
    UtilsVK::createImage( *m_runtime.m_renderer->getDevice(), VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT        , width, height, m_render_target_attachments.m_pp_attachments[0]         );
    UtilsVK::createImage( *m_runtime.m_renderer->getDevice(), VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT        , width, height, m_render_target_attachments.m_pp_attachments[1]         );

    if (runtimeVariables.shadowsType == ShadowsType::ShadowMap) {
        UtilsVK::createImage(*m_runtime.m_renderer->getDevice(), VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, kSHADOWS_RES, kSHADOWS_RES, kMAX_NUMBER_LIGHTS * kLAYERS_PER_LIGHT, 1,
            ImageBlockType::IMAGE_BLOCK_2D_ARRAY, m_render_target_attachments.m_shadow_attachment);
    }
    else {
        UtilsVK::createImage(*m_runtime.m_renderer->getDevice(), VK_FORMAT_R16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, width, height, m_render_target_attachments.m_shadow_attachment);
        UtilsVK::createImage(*m_runtime.m_renderer->getDevice(), VK_FORMAT_R16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, width, height, m_render_target_attachments.m_visibility_attachment);
        UtilsVK::createImage(*m_runtime.m_renderer->getDevice(), VK_FORMAT_R16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, width, height, m_render_target_attachments.m_denoise_history_attachment);
    }

    m_render_target_attachments.m_color_attachment.m_sampler            = m_global_samplers[ 0 ];         
    m_render_target_attachments.m_normal_attachment.m_sampler           = m_global_samplers[ 0 ];        
    m_render_target_attachments.m_position_depth_attachment.m_sampler   = m_global_samplers[ 0 ];
    m_render_target_attachments.m_motionVectors_attachment.m_sampler   = m_global_samplers[ 0 ];
    m_render_target_attachments.m_material_attachment.m_sampler         = m_global_samplers[ 0 ];      
    m_render_target_attachments.m_depth_attachment.m_sampler            = m_global_samplers[ 0 ];         
    m_render_target_attachments.m_ssao_attachment.m_sampler             = m_global_samplers[ 0 ];          
    m_render_target_attachments.m_ssao_blur_attachment.m_sampler        = m_global_samplers[ 0 ]; 
    m_render_target_attachments.m_lighting_attachment.m_sampler         = m_global_samplers[ 0 ];
    m_render_target_attachments.m_taa_history_attachment.m_sampler      = m_global_samplers[ 0 ];
    m_render_target_attachments.m_pp_attachments[0].m_sampler           = m_global_samplers[ 0 ];
    m_render_target_attachments.m_pp_attachments[1].m_sampler           = m_global_samplers[ 0 ];
    m_render_target_attachments.m_shadow_attachment.m_sampler           = m_global_samplers[ 0 ];
    m_render_target_attachments.m_visibility_attachment.m_sampler       = m_global_samplers[ 0 ];
    m_render_target_attachments.m_denoise_history_attachment.m_sampler  = m_global_samplers[ 0 ];

    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_color_attachment.m_image          ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Image Color Attachment"    );
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_normal_attachment.m_image         ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Image Normal Attachment "  );
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_position_depth_attachment.m_image ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Image Position Attachment ");
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_motionVectors_attachment.m_image ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,  "Motion Vectors Attachment ");
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_material_attachment.m_image       ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Image Material Attachment ");
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_depth_attachment.m_image          ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Image Depth Buffer"        );
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_ssao_attachment.m_image           ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Image SSAO attachment"     );
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_ssao_blur_attachment.m_image      ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Image SSAO blur "          );
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_lighting_attachment.m_image           ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Image lighting output "    );
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_taa_history_attachment.m_image               ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Image TAA history "        );
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_pp_attachments[0].m_image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Post process output ");
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_pp_attachments[1].m_image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Post process output ");
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_shadow_attachment.m_image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Shadow output ");
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_visibility_attachment.m_image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Visibility output ");
    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)( m_render_target_attachments.m_denoise_history_attachment.m_image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Denoise history ");
}


void Engine::destroyAttachments()
{
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_color_attachment          );
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_normal_attachment         );
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_position_depth_attachment );
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_motionVectors_attachment );
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_material_attachment       );
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_depth_attachment          );
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_ssao_attachment           );
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_ssao_blur_attachment      );
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_lighting_attachment           );
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_taa_history_attachment               );
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_pp_attachments[0]);
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_pp_attachments[1]);
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_shadow_attachment);
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_visibility_attachment);
    UtilsVK::freeImageBlock( *m_runtime.m_renderer->getDevice(), m_render_target_attachments.m_denoise_history_attachment);
}


void Engine::createSamplers()
{
    VkSamplerCreateInfo sampler{};
    sampler.sType           = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.magFilter       = VK_FILTER_LINEAR;
	sampler.minFilter       = VK_FILTER_LINEAR;
	sampler.mipmapMode      = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU    = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV    = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeW    = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.mipLodBias      = 0.0f;
	sampler.maxAnisotropy   = 1.0f;
	sampler.minLod          = 0.0f;
	sampler.maxLod          = 1.0f;
	sampler.borderColor     = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	
    if( VK_SUCCESS != vkCreateSampler( m_runtime.m_renderer->getDevice()->getLogicalDevice(), &sampler, nullptr, &m_global_samplers[ 0 ] ) )
    {
        throw MiniEngineException( "Error creating sampler" );
    }

    UtilsVK::setObjectName( m_runtime.m_renderer->getDevice()->getLogicalDevice(), (uint64_t)m_global_samplers[ 0 ], VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, "Global Sampler"  );
}


void Engine::destroySamplers()
{
    for( VkSampler sampler : m_global_samplers )
    {
        vkDestroySampler( m_runtime.m_renderer->getDevice()->getLogicalDevice(), sampler, nullptr );
    }
}