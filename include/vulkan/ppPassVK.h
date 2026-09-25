#pragma once

#include "vulkan/renderPassVK.h"

namespace MiniEngine
{
    struct Runtime;
    class MeshVK;
    typedef std::shared_ptr<MeshVK> MeshVKPtr;

    class PPPassVK final : public RenderPassVK
    {
    public:
        PPPassVK(
            const Runtime& i_runtime,
            std::vector<ImageBlock*> i_input_attachments,
            const ImageBlock& i_output_color,
            PostProcessType type
        );
        virtual ~PPPassVK();

        bool            initialize() override;
        void            shutdown() override;
        VkCommandBuffer draw(const Frame& i_frame) override;

    private:
        PPPassVK(const PPPassVK&) = delete;
        PPPassVK& operator=(const PPPassVK&) = delete;

        void getShaderNames(char** out_names);
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
        VkPipeline                                                         m_pp_pipeline;
        VkPipelineLayout                                                   m_pipeline_layouts;
        VkDescriptorSetLayout                                              m_descriptor_set_layout; //2 sets, per frame and per object
        VkDescriptorPool                                                   m_descriptor_pool;
        std::array<DescriptorsSets, kMAX_NUMBER_OF_FRAMES>                 m_descriptor_sets;
        std::array<VkPipelineShaderStageCreateInfo, 2 >                    m_shader_stages;

        MeshVKPtr m_plane;

        std::vector<ImageBlock*> m_input_attachments;
        ImageBlock m_output_attachment;
        PostProcessType type;
    };
};