#include "runtime.h"
#include "vulkan/rendererVK.h"
#include "vulkan/deviceVK.h"
#include "vulkan/utilsVK.h"
#include "frame.h"
#include "entity.h"
#include "engine.h"
#include "scene.h"
#include "vulkan/meshVK.h"

using namespace MiniEngine;


void Runtime::createResources()
{
    for( uint32_t id = 0; id < m_per_frame_buffer.size(); id++ )
    {
        if( VK_NULL_HANDLE == m_per_frame_buffer[ id ] )
        {
            UtilsVK::createBuffer( *m_renderer->getDevice(), sizeof( PerFrameData ), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_per_frame_buffer[ id ], m_per_frame_buffer_memory[ id ] );
        }

        if( VK_NULL_HANDLE == m_per_object_buffer[ id ] )
        {
            UtilsVK::createBuffer( *m_renderer->getDevice(), sizeof( PerObjectData ) * kMAX_NUMBER_OF_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_per_object_buffer[ id ], m_per_object_buffer_memory[ id ] );
        }
    }
    //TLAS creation:
    const std::vector<EntityPtr> entities = Engine::instance().getScene().getMeshes();
    std::vector<Matrix4f> transforms;
    std::vector<VkAccelerationStructureKHR> blas_instances;
    for (EntityPtr entity : entities) {
        transforms.push_back(entity->getTransform().getTransform());
        blas_instances.push_back(entity->getMesh().getBlas());
    }
    UtilsVK::createTLAS( *m_renderer->getDevice(), transforms, blas_instances, m_tlas, m_tlas_buffer, m_tlas_memory,
        m_instances_buffer, m_instances_memory);
    UtilsVK::setObjectName(m_renderer->getDevice()->getLogicalDevice(), (uint64_t)m_tlas_buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "TLAS BUFFER");
}

void Runtime::updateTLAS() {
    const std::vector<EntityPtr> entities = Engine::instance().getScene().getMeshes();
    std::vector<Matrix4f> transforms;
    std::vector<VkAccelerationStructureKHR> blas_instances;
    for (EntityPtr entity : entities) {
        transforms.push_back(entity->getTransform().getTransform());
        blas_instances.push_back(entity->getMesh().getBlas());
    }
    UtilsVK::updateTLAS(*m_renderer->getDevice(), transforms, blas_instances, 
        m_instances_buffer, m_instances_memory, m_tlas);
}


void Runtime::freeResources()
{
    for( uint32_t id = 0; id < m_per_frame_buffer.size(); id++ )
    {
        if( VK_NULL_HANDLE != m_per_frame_buffer[ id ] )
        {
            vkDestroyBuffer( m_renderer->getDevice()->getLogicalDevice(), m_per_frame_buffer       [ id ], nullptr );
            vkFreeMemory   ( m_renderer->getDevice()->getLogicalDevice(), m_per_frame_buffer_memory[ id ], nullptr );

            m_per_frame_buffer[ id ] = VK_NULL_HANDLE;
        }

        if( VK_NULL_HANDLE != m_per_object_buffer[ id ] )
        {
            vkDestroyBuffer( m_renderer->getDevice()->getLogicalDevice(), m_per_object_buffer       [ id ], nullptr );
            vkFreeMemory   ( m_renderer->getDevice()->getLogicalDevice(), m_per_object_buffer_memory[ id ], nullptr );

            m_per_object_buffer[ id ] = VK_NULL_HANDLE;
        }
    }
    if (m_tlas)
    {
        vkDestroyAccelerationStructure(m_renderer->getDevice()->getLogicalDevice(), m_tlas, nullptr);
        vkDestroyBuffer(m_renderer->getDevice()->getLogicalDevice(), m_tlas_buffer, nullptr);
        vkFreeMemory(m_renderer->getDevice()->getLogicalDevice(), m_tlas_memory, nullptr);
    }
}

namespace MiniEngine {
    RuntimeVariables runtimeVariables = {
        true, //ao enabled
        0.2f, //ao radius
        0.001f, //ao bias
        0.2f, //ssdo factor
        AntiAliasingType::TAA, //aa type
        ShadowsType::RTX_Denoised, //Shadows type
        true, //Tonemapping
        1.5f, //Exposure
        false, //Bloom
        false, //Chromatic aberration
        Vector4f(0.0f, //Shadow map min bias
            0.00015f, //Slope bias
            0.0003f, //Max bias
            0.0003f), //tmin (rtx)
        3, //Num cascades
        Vector4f(1.0f, //PCF enabled
            9.0f, //kernel size
            4.0f, //num samples (rtx)
            0.07f), //cone radius (rtx)
        Vector4f(1.0f, //reflections enabled
            1.0f, //reflection strength
            0.0f, //unused
            0.0f) //unused
    };
}
