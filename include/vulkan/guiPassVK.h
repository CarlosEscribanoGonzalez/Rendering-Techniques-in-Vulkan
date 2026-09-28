#pragma once

#include "vulkan/renderPassVK.h"

namespace MiniEngine
{
    struct Runtime;
    class MeshVK;
    typedef std::shared_ptr<MeshVK> MeshVKPtr;

    class GuiPassVK final : public RenderPassVK
    {
    public:
        GuiPassVK(
            const Runtime& i_runtime,
            std::array<ImageBlock, 3> i_output_swap_images
        );
        virtual ~GuiPassVK();

        bool            initialize() override;
        void            shutdown() override;
        VkCommandBuffer draw(const Frame& i_frame) override;

    private:
        GuiPassVK(const GuiPassVK&) = delete;
        GuiPassVK& operator=(const GuiPassVK&) = delete;

        void createFbo();
        void createRenderPass();

        VkRenderPass                   m_render_pass;
        std::array<VkCommandBuffer, 3> m_command_buffer;
        std::array<VkFramebuffer, 3>   m_fbos;
        VkDescriptorPool               m_imgui_pool;

        std::array<ImageBlock, 3> m_output_swap_images;
    };
};