#pragma once

#include "vulkan/renderPassVK.h"

namespace MiniEngine
{
    struct Runtime;
    class MeshVK;
    typedef std::shared_ptr<MeshVK> MeshVKPtr;

    class TAAPassVK final : public RenderPassVK
    {
    public:
        TAAPassVK(
            const Runtime& i_runtime,
            const ImageBlock& i_color_attachment,
            const ImageBlock& i_history_attachment,
            const ImageBlock& i_motion_attachment,
            const ImageBlock& i_aa_output
        );
        virtual ~TAAPassVK();

        bool            initialize() override;
        void            shutdown() override;
        VkCommandBuffer draw(const Frame& i_frame) override;

    private:
        TAAPassVK(const TAAPassVK&) = delete;
        TAAPassVK& operator=(const TAAPassVK&) = delete;

        void createFbo();
        void createRenderPass();
        void createPipelines();
        void createDescriptorLayout();
        void createDescriptors();

        struct DescriptorsSets
        {
            VkDescriptorSet m_textures_descriptor;
        };

        VkRenderPass                   m_render_pass;
        std::array<VkCommandBuffer, 3> m_command_buffer;
        std::array<VkFramebuffer, 3> m_fbos;

        // prepare the different render supported depending on the material
        VkPipeline                                                         m_taa_pipeline;
        VkPipelineLayout                                                   m_pipeline_layouts;
        VkDescriptorSetLayout                                              m_descriptor_set_layout; //2 sets, per frame and per object
        VkDescriptorPool                                                   m_descriptor_pool;
        std::array<DescriptorsSets, kMAX_NUMBER_OF_FRAMES>                 m_descriptor_sets;
        std::array<VkPipelineShaderStageCreateInfo, 2 >                    m_shader_stages;

        MeshVKPtr m_plane;

        ImageBlock m_color_attachment;
        ImageBlock m_history_attachment;
        ImageBlock m_motion_attachment;
        ImageBlock m_output_attachment;
    };
};