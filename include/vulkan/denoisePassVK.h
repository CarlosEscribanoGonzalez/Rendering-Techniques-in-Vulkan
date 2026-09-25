#pragma once

#include "vulkan/renderPassVK.h"

namespace MiniEngine
{
    #define MAX_MASK_SIZE 21

    struct Runtime;
    class MeshVK;
    typedef std::shared_ptr<MeshVK> MeshVKPtr;

    class DenoisePassVK final : public RenderPassVK
    {
    public:
        DenoisePassVK(
            const Runtime& i_runtime,
            const ImageBlock& i_in_color_attachment,
            const ImageBlock& i_in_depth_attachment,
            const ImageBlock& i_in_normal_attachment,
            const ImageBlock& i_in_history_attachment,
            const ImageBlock& i_in_motion_attachment,
            const ImageBlock& i_blur_attachment,
            BlurType type
        );
        virtual ~DenoisePassVK();

        void            createMask(int size, float sigma = -1.0f);
        bool            initialize() override;
        void            shutdown() override;
        VkCommandBuffer draw(const Frame& i_frame) override;

    private:
        DenoisePassVK(const DenoisePassVK&) = delete;
        DenoisePassVK& operator=(const DenoisePassVK&) = delete;

        void createFbo();
        void createRenderPass();
        void createPipelines();
        void createDescriptorLayout();
        void createDescriptors();

        struct DescriptorsSets
        {
            VkDescriptorSet m_textures_descriptor;
        };

        struct MaskData {
            std::array<Vector4f, MAX_MASK_SIZE * MAX_MASK_SIZE / 4> mask;
            unsigned int width;
        };

        VkRenderPass                   m_render_pass;
        std::array<VkCommandBuffer, 3> m_command_buffer;
        std::array<VkFramebuffer, 3> m_fbos;

        // prepare the different render supported depending on the material
        VkPipeline                                                         m_ssao_pipeline;
        VkPipelineLayout                                                   m_pipeline_layouts;
        VkDescriptorSetLayout                                              m_descriptor_set_layout; //2 sets, per frame and per object
        VkDescriptorPool                                                   m_descriptor_pool;
        std::array<DescriptorsSets, kMAX_NUMBER_OF_FRAMES>                 m_descriptor_sets;
        std::array<VkPipelineShaderStageCreateInfo, 2 >                    m_shader_stages;

        MeshVKPtr m_plane;

        ImageBlock m_color_attachment;
        ImageBlock m_depth_attachment;
        ImageBlock m_normal_attachment;
        ImageBlock m_history_attachment;
        ImageBlock m_motion_attachment;
        ImageBlock m_blur_attachment;
        MaskData       m_mask;
        VkBuffer       m_mask_buffer;
        VkDeviceMemory m_mask_memory;
    };
};