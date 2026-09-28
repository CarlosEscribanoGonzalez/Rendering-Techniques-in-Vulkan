#include "common.h"
#include "vulkan/utilsVK.h"
#include "vulkan/guiPassVK.h"
#include "vulkan/rendererVK.h"
#include "vulkan/deviceVK.h"
#include "vulkan/windowVK.h"
#include "runtime.h"
#include "frame.h"
#include "shaderRegistry.h"
#include "meshRegistry.h"
#include "entity.h"
#include "vulkan/meshVK.h"
#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_vulkan.h"

using namespace MiniEngine;


GuiPassVK::GuiPassVK(
    const Runtime& i_runtime,
    std::array<ImageBlock, 3> i_output_swap_images
) :
    RenderPassVK(i_runtime),
    m_output_swap_images(i_output_swap_images)
{
    for (auto cmd : m_command_buffer)
    {
        cmd = VK_NULL_HANDLE;
    }
}


GuiPassVK::~GuiPassVK()
{
}

bool GuiPassVK::initialize()
{
    RendererVK& renderer = *m_runtime.m_renderer;
    createRenderPass();
    createFbo();
    VkDevice device = renderer.getDevice()->getLogicalDevice();
    //ImGUI descriptor pool
    std::array<VkDescriptorPoolSize, 11> pool_sizes =
    { {
        { VK_DESCRIPTOR_TYPE_SAMPLER,                1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000 }
    } };
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * static_cast<uint32_t>(pool_sizes.size());
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    VkResult result = vkCreateDescriptorPool(
        device,
        &pool_info,
        nullptr,
        &m_imgui_pool
    );
    if (result != VK_SUCCESS)
    {
        throw MiniEngineException("Failed to create ImGui descriptor pool");
    }
    //ImGUI initialization
    ImGui_ImplVulkan_InitInfo init{};
    init.Instance = renderer.getInstance();
    init.PhysicalDevice = renderer.getDevice()->getPhysicalDevice();
    init.Device = device;
    init.QueueFamily = renderer.getDevice()->getQueueFamily();
    init.Queue = renderer.getDevice()->getGraphicsQueue();
    init.DescriptorPool = m_imgui_pool;
    init.MinImageCount = 2;
    init.ImageCount = renderer.getWindow().getImageCount();

    init.PipelineInfoMain.RenderPass = m_render_pass;
    init.PipelineInfoMain.Subpass = 0;
    init.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    if (!ImGui_ImplVulkan_Init(&init))
    {
        throw MiniEngineException("Failed to initialize ImGui Vulkan backend");
    }
    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType =
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool =
        renderer.getDevice()->getCommandPool();
    commandBufferAllocateInfo.level =
        VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 3;

    result = vkAllocateCommandBuffers(
        device,
        &commandBufferAllocateInfo,
        m_command_buffer.data()
    );
    if (result != VK_SUCCESS)
    {
        throw MiniEngineException("Failed to allocate ImGui command buffers");
    }
    return true;
}


void GuiPassVK::shutdown()
{
    RendererVK& renderer = *m_runtime.m_renderer;

    vkDeviceWaitIdle(renderer.getDevice()->getLogicalDevice());
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(renderer.getDevice()->getLogicalDevice(), m_imgui_pool, nullptr);

    vkFreeCommandBuffers(renderer.getDevice()->getLogicalDevice(), renderer.getDevice()->getCommandPool(), m_command_buffer.size(), m_command_buffer.data());

    for (uint32 id = 0; id < static_cast<uint32>(renderer.getWindow().getImageCount()); id++)
    {
        vkDestroyFramebuffer(renderer.getDevice()->getLogicalDevice(), m_fbos[id], nullptr);
    }

    vkDestroyRenderPass(renderer.getDevice()->getLogicalDevice(), m_render_pass, nullptr);
}


VkCommandBuffer GuiPassVK::draw(const Frame& i_frame)
{
    RendererVK& renderer = *m_runtime.m_renderer;

    VkCommandBuffer& current_cmd = m_command_buffer[renderer.getWindow().getCurrentImageId()];

    if (current_cmd != VK_NULL_HANDLE)
    {
        VkCommandBufferResetFlags flags{};
        vkResetCommandBuffer(current_cmd, flags);
    }

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    uint32_t width = 0, height = 0;
    renderer.getWindow().getWindowSize(width, height);

    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = m_render_pass;
    render_pass_info.framebuffer = m_fbos[renderer.getWindow().getCurrentImageId()];
    render_pass_info.renderArea.offset = { 0, 0 };
    render_pass_info.renderArea.extent = { width, height };
    render_pass_info.clearValueCount = 0;
    if (vkBeginCommandBuffer(current_cmd, &begin_info) != VK_SUCCESS)
    {
        throw MiniEngineException("failed to begin recording command buffer!");
    }
    UtilsVK::beginRegion(current_cmd, "ImGUI pass", Vector4f(0.5f, 0.0f, 0.0f, 1.0f));
    vkCmdBeginRenderPass(current_cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    if (ImDrawData* dd = ImGui::GetDrawData())
        ImGui_ImplVulkan_RenderDrawData(dd, current_cmd);
    vkCmdEndRenderPass(current_cmd);
    UtilsVK::endRegion(current_cmd);
    if (vkEndCommandBuffer(current_cmd) != VK_SUCCESS)
    {
        throw MiniEngineException("failed to record command buffer!");
    }
    return current_cmd;
}


void GuiPassVK::createFbo()
{
    RendererVK& renderer = *m_runtime.m_renderer;

    uint32_t width = 0, height = 0;
    renderer.getWindow().getWindowSize(width, height);

    for (size_t i = 0; i < m_fbos.size(); i++)
    {
        std::array<VkImageView, 1> attachments;
        attachments[0] = m_output_swap_images[i].m_image_view;

        VkFramebufferCreateInfo framebuffer_create_info = {};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        // All frame buffers use the same renderpass setup
        framebuffer_create_info.renderPass = m_render_pass;
        framebuffer_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebuffer_create_info.pAttachments = attachments.data();
        framebuffer_create_info.width = width;
        framebuffer_create_info.height = height;
        framebuffer_create_info.layers = 1;
        // Create the framebuffer

        if (vkCreateFramebuffer(renderer.getDevice()->getLogicalDevice(), &framebuffer_create_info, nullptr, &m_fbos[i]))
        {
            throw MiniEngineException("failed to create fbos");
        }
    }
}


void GuiPassVK::createRenderPass()
{
    RendererVK& renderer = *m_runtime.m_renderer;

    std::array<VkAttachmentDescription, 1> attachments = {};
    // Output attachment
    attachments[0].format = m_output_swap_images[0].m_format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference output_reference = {};
    output_reference.attachment = 0;
    output_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description = {};
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &output_reference;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = nullptr;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = nullptr;
    subpass_description.pResolveAttachments = nullptr;

    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 1> dependencies = { {} };

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = dependencies.size();
    render_pass_info.pSubpasses = &subpass_description;
    render_pass_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
    render_pass_info.pDependencies = dependencies.data();

    if (vkCreateRenderPass(renderer.getDevice()->getLogicalDevice(), &render_pass_info, nullptr, &m_render_pass))
    {
        throw MiniEngineException("Failed to create empty render pass");
    }
}