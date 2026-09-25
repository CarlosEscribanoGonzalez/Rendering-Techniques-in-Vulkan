#include "vulkan/utilsVK.h"
#include "vulkan/deviceVK.h"
#include "vulkan/rendererVK.h"
#include "runtime.h"
#include <vector>
#include <fstream>
#include <iostream>

using namespace MiniEngine;

#ifdef DEBUG
PFN_vkDebugMarkerSetObjectTagEXT  vkDebugMarkerSetObjectTag = VK_NULL_HANDLE;
PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectName = VK_NULL_HANDLE;
PFN_vkCmdDebugMarkerBeginEXT      vkCmdDebugMarkerBegin = VK_NULL_HANDLE;
PFN_vkCmdDebugMarkerEndEXT        vkCmdDebugMarkerEnd = VK_NULL_HANDLE;
PFN_vkCmdDebugMarkerInsertEXT     vkCmdDebugMarkerInsert = VK_NULL_HANDLE;
bool                              is_active = false;
#endif

void UtilsVK::setupCallbacks(DeviceVK& i_device)
{
#ifdef DEBUG
    assert(false == is_active);

    vkDebugMarkerSetObjectTag = (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(i_device.getLogicalDevice(), "vkDebugMarkerSetObjectTagEXT");
    vkDebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(i_device.getLogicalDevice(), "vkDebugMarkerSetObjectNameEXT");
    vkCmdDebugMarkerBegin = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(i_device.getLogicalDevice(), "vkCmdDebugMarkerBeginEXT");
    vkCmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(i_device.getLogicalDevice(), "vkCmdDebugMarkerEndEXT");
    vkCmdDebugMarkerInsert = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(i_device.getLogicalDevice(), "vkCmdDebugMarkerInsertEXT");

    is_active = (vkDebugMarkerSetObjectName != VK_NULL_HANDLE);
#endif
}

void UtilsVK::setObjectName(VkDevice i_device, uint64_t i_object, VkDebugReportObjectTypeEXT i_object_type, const char* i_name)
{
#ifdef DEBUG
    if (is_active)
    {
        VkDebugMarkerObjectNameInfoEXT name_info = {};
        name_info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
        name_info.objectType = i_object_type;
        name_info.object = i_object;
        name_info.pObjectName = i_name;
        vkDebugMarkerSetObjectName(i_device, &name_info);
    }
#endif
}

// Set the tag for an object
void UtilsVK::setObjectTag(VkDevice i_device, uint64_t i_object, VkDebugReportObjectTypeEXT i_object_type, uint64_t i_name, size_t i_tag_size, const void* i_tag)
{
#ifdef DEBUG
    if (is_active)
    {
        VkDebugMarkerObjectTagInfoEXT tag_info = {};
        tag_info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT;
        tag_info.objectType = i_object_type;
        tag_info.object = i_object;
        tag_info.tagName = i_name;
        tag_info.tagSize = i_tag_size;
        tag_info.pTag = i_tag;
        vkDebugMarkerSetObjectTag(i_device, &tag_info);
    }
#endif
}

// Start a new debug marker region
void UtilsVK::beginRegion(VkCommandBuffer i_cmd_buffer, const char* i_pmarker_name, glm::vec4 i_color)
{
#ifdef DEBUG
    if (is_active)
    {
        VkDebugMarkerMarkerInfoEXT marker_info = {};
        marker_info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;

        memcpy(marker_info.color, &i_color[0], sizeof(float) * 4);
        marker_info.pMarkerName = i_pmarker_name;
        vkCmdDebugMarkerBegin(i_cmd_buffer, &marker_info);
    }
#endif
}

// Insert a new debug marker into the command buffer
void UtilsVK::insert(VkCommandBuffer i_cmd_buffer, std::string i_marker_name, glm::vec4 i_color)
{
#ifdef DEBUG
    if (is_active)
    {
        VkDebugMarkerMarkerInfoEXT marker_info = {};
        marker_info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;

        memcpy(marker_info.color, &i_color[0], sizeof(float) * 4);
        marker_info.pMarkerName = i_marker_name.c_str();

        vkCmdDebugMarkerInsert(i_cmd_buffer, &marker_info);
    }
#endif
}

// End the current debug marker region
void UtilsVK::endRegion(VkCommandBuffer cmdBuffer)
{
#ifdef DEBUG
    if (vkCmdDebugMarkerEnd)
    {
        vkCmdDebugMarkerEnd(cmdBuffer);
    }
#endif
}


bool UtilsVK::getSupportedDepthFormat(VkPhysicalDevice i_physical_device, VkFormat* i_depth_format)
{
    std::vector<VkFormat> depth_formats =
    {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };

    for (auto& format : depth_formats)
    {
        VkFormatProperties format_props;
        vkGetPhysicalDeviceFormatProperties(i_physical_device, format, &format_props);
        if (format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            *i_depth_format = format;
            return true;
        }
    }

    return false;
}


VkShaderModule UtilsVK::loadShader(const std::string i_fileName, VkDevice i_device)
{
    std::ifstream is(i_fileName.c_str(), std::ios::binary | std::ios::in | std::ios::ate);

    if (is.is_open())
    {
        size_t size = static_cast<size_t>(is.tellg());
        is.seekg(0, std::ios::beg);
        char* shaderCode = new char[size];
        is.read(shaderCode, size);
        is.close();

        if (size == 0)
        {
            return VK_NULL_HANDLE;
        }

        VkShaderModule shader_module;
        VkShaderModuleCreateInfo module_create_info{};
        module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        module_create_info.codeSize = size;
        module_create_info.pCode = (uint32_t*)shaderCode;

        if (vkCreateShaderModule(i_device, &module_create_info, NULL, &shader_module))
        {
            throw MiniEngineException("Error creating the shader");
        }

        delete[] shaderCode;

        return shader_module;
    }
    else
    {
        std::cerr << "Error: Could not open shader file \"" << i_fileName.c_str() << "\"" << "\n";
        return VK_NULL_HANDLE;
    }
}


void UtilsVK::createBuffer(const DeviceVK& i_device, VkDeviceSize i_size, VkBufferUsageFlags i_usage, VkMemoryPropertyFlags i_properties, VkBuffer& o_buffer, VkDeviceMemory& o_buffer_memory)
{
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = i_size;
    buffer_info.usage = i_usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(i_device.getLogicalDevice(), &buffer_info, nullptr, &o_buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(i_device.getLogicalDevice(), o_buffer, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = i_device.getMemoryTypeIndex(mem_requirements.memoryTypeBits, i_properties);

    VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
    memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    alloc_info.pNext = &memoryAllocateFlagsInfo;

    if (vkAllocateMemory(i_device.getLogicalDevice(), &alloc_info, nullptr, &o_buffer_memory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(i_device.getLogicalDevice(), o_buffer, o_buffer_memory, 0);
}


void UtilsVK::copyBuffer(const DeviceVK& i_device, VkBuffer i_src_buffer, VkBuffer i_dst_buffer, VkDeviceSize i_size)
{
    VkCommandBuffer command_buffer = initOneTimeCommandBuffer(i_device);

    VkBufferCopy copy_region{};
    copy_region.size = i_size;
    vkCmdCopyBuffer(command_buffer, i_src_buffer, i_dst_buffer, 1, &copy_region);

    endOneTimeCommandBuffer(i_device, command_buffer);
}


void UtilsVK::setImageLayout(
    VkCommandBuffer cmdbuffer,
    VkImage image,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkImageSubresourceRange subresourceRange,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask)
{
    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    switch (oldImageLayout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        imageMemoryBarrier.srcAccessMask = 0;
        break;
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        break;
    }

    switch (newImageLayout)
    {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        if (imageMemoryBarrier.srcAccessMask == 0)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        break;
    }

    vkCmdPipelineBarrier(
        cmdbuffer,
        srcStageMask,
        dstStageMask,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageMemoryBarrier);
}


void UtilsVK::createImage(const DeviceVK& i_device, VkFormat i_format, VkImageUsageFlagBits i_usage_bits, uint32_t i_width, uint32_t i_height, ImageBlock& o_image_block, int sampleCount)
{
    VkImageAspectFlags aspect_mask = 0;
    VkImageLayout      image_layout;

    o_image_block.m_format = i_format;

    if (i_usage_bits & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    if (i_usage_bits & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    assert(aspect_mask > 0);

    VkImageCreateInfo image{};
    image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = i_format;
    image.extent.width = i_width;
    image.extent.height = i_height;
    image.extent.depth = 1;
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = getCountFlag(sampleCount);
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = i_usage_bits | VK_IMAGE_USAGE_SAMPLED_BIT;

    VkMemoryAllocateInfo mem_alloc{};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements mem_reqs;

    if (VK_SUCCESS != vkCreateImage(i_device.getLogicalDevice(), &image, nullptr, &o_image_block.m_image))
    {
        throw MiniEngineException("Issue creating an image");
    }

    vkGetImageMemoryRequirements(i_device.getLogicalDevice(), o_image_block.m_image, &mem_reqs);
    mem_alloc.allocationSize = mem_reqs.size;
    mem_alloc.memoryTypeIndex = i_device.getMemoryTypeIndex(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (VK_SUCCESS != vkAllocateMemory(i_device.getLogicalDevice(), &mem_alloc, nullptr, &o_image_block.m_memory))
    {
        throw MiniEngineException("Issue creating an image");
    }

    if (VK_SUCCESS != vkBindImageMemory(i_device.getLogicalDevice(), o_image_block.m_image, o_image_block.m_memory, 0))
    {
        throw MiniEngineException("Issue creating an image");
    }

    VkImageViewCreateInfo image_view{};
    image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view.format = o_image_block.m_format;
    image_view.subresourceRange = {};
    image_view.subresourceRange.aspectMask = aspect_mask;
    image_view.subresourceRange.baseMipLevel = 0;
    image_view.subresourceRange.levelCount = 1;
    image_view.subresourceRange.baseArrayLayer = 0;
    image_view.subresourceRange.layerCount = 1;
    image_view.image = o_image_block.m_image;

    if (VK_SUCCESS != vkCreateImageView(i_device.getLogicalDevice(), &image_view, nullptr, &o_image_block.m_image_view))
    {
        throw MiniEngineException("Issue creating an image");
    }
}


void UtilsVK::createImage(const DeviceVK& i_device, VkFormat i_format, VkImageUsageFlagBits i_usage_bits, 
    uint32_t i_width, uint32_t i_height, uint32_t i_depth, uint32_t i_mip_levels, 
    ImageBlockType i_image_type, ImageBlock& o_image_block)
{
    VkImageAspectFlags aspect_mask = 0;
    VkImageLayout      image_layout;

    o_image_block.m_format = i_format;
    o_image_block.m_type = i_image_type;

    if (i_usage_bits & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    if (i_usage_bits & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
        image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    assert(aspect_mask > 0);

    VkImageCreateInfo image{};
    image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image.imageType = i_image_type != IMAGE_BLOCK_3D ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D;
    image.format = i_format;
    image.extent.width = i_width;
    image.extent.height = i_height;
    image.extent.depth = i_image_type != IMAGE_BLOCK_3D ? 1 : i_depth;
    image.mipLevels = i_mip_levels;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = i_usage_bits | VK_IMAGE_USAGE_SAMPLED_BIT;

    switch (i_image_type) {
    case IMAGE_BLOCK_2D:
        image.arrayLayers = 1;
        break;
    case IMAGE_BLOCK_2D_ARRAY:
        image.arrayLayers = i_depth;
        break;
    case IMAGE_BLOCK_3D:
        image.arrayLayers = 1;
        break;
    case IMAGE_BLOCK_CUBE:
        image.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        image.arrayLayers = i_depth;
        break;
    }

    VkMemoryAllocateInfo mem_alloc{};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements mem_reqs;

    if (VK_SUCCESS != vkCreateImage(i_device.getLogicalDevice(), &image, nullptr, &o_image_block.m_image))
    {
        throw MiniEngineException("Issue creating an image");
    }

    vkGetImageMemoryRequirements(i_device.getLogicalDevice(), o_image_block.m_image, &mem_reqs);
    mem_alloc.allocationSize = mem_reqs.size;
    mem_alloc.memoryTypeIndex = i_device.getMemoryTypeIndex(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (VK_SUCCESS != vkAllocateMemory(i_device.getLogicalDevice(), &mem_alloc, nullptr, &o_image_block.m_memory))
    {
        throw MiniEngineException("Issue creating an image");
    }

    if (VK_SUCCESS != vkBindImageMemory(i_device.getLogicalDevice(), o_image_block.m_image, o_image_block.m_memory, 0))
    {
        throw MiniEngineException("Issue creating an image");
    }

    VkImageViewCreateInfo image_view{};
    image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view.format = o_image_block.m_format;
    image_view.subresourceRange = {};
    image_view.subresourceRange.aspectMask = aspect_mask;
    image_view.subresourceRange.baseMipLevel = 0;
    image_view.subresourceRange.levelCount = i_mip_levels;
    image_view.subresourceRange.baseArrayLayer = 0;
    image_view.image = o_image_block.m_image;

    switch (i_image_type)
    {
    case IMAGE_BLOCK_2D:
        image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view.subresourceRange.layerCount = 1;
        break;
    case IMAGE_BLOCK_2D_ARRAY:
        image_view.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        image_view.subresourceRange.layerCount = i_depth;
        break;
    case IMAGE_BLOCK_3D:
        image_view.viewType = VK_IMAGE_VIEW_TYPE_3D;
        image_view.subresourceRange.layerCount = 1;
        break;
    case IMAGE_BLOCK_CUBE:
        image_view.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        image_view.subresourceRange.layerCount = i_depth;
        break;
    }

    if (VK_SUCCESS != vkCreateImageView(i_device.getLogicalDevice(), &image_view, nullptr, &o_image_block.m_image_view))
    {
        throw MiniEngineException("Issue creating an image");
    }
}


void UtilsVK::freeImageBlock(const DeviceVK& i_device, ImageBlock& io_free_image_block)
{
    vkDestroyImageView(i_device.getLogicalDevice(), io_free_image_block.m_image_view, nullptr);
    vkDestroyImage(i_device.getLogicalDevice(), io_free_image_block.m_image, nullptr);
    vkFreeMemory(i_device.getLogicalDevice(), io_free_image_block.m_memory, nullptr);

    io_free_image_block.m_image_view = VK_NULL_HANDLE;
    io_free_image_block.m_image = VK_NULL_HANDLE;
    io_free_image_block.m_memory = VK_NULL_HANDLE;
}


void UtilsVK::TextureFromBuffer(const DeviceVK& device, void* i_buffer, VkDeviceSize i_buffer_size, VkFormat i_format, uint32_t i_tex_width, uint32_t i_tex_height, ImageBlock& o_new_image, VkFilter i_filter, VkImageUsageFlags i_image_usage_flags, VkImageLayout i_image_layout)
{
    assert(i_buffer != nullptr);
    uint32_t mip_levels = 1;

    VkMemoryAllocateInfo mem_alloc_info{};
    mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements mem_reqs;

    VkCommandBuffer command_buffer = initOneTimeCommandBuffer(device);

    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;

    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = i_buffer_size;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (VK_SUCCESS != vkCreateBuffer(device.getLogicalDevice(), &buffer_create_info, nullptr, &staging_buffer))
    {
        throw MiniEngineException("Error Creating Buffer");
    }

    vkGetBufferMemoryRequirements(device.getLogicalDevice(), staging_buffer, &mem_reqs);

    mem_alloc_info.allocationSize = mem_reqs.size;
    mem_alloc_info.memoryTypeIndex = device.getMemoryTypeIndex(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (VK_SUCCESS != vkAllocateMemory(device.getLogicalDevice(), &mem_alloc_info, nullptr, &staging_memory))
    {
        throw MiniEngineException("Error Creating Buffer");
    }

    if (VK_SUCCESS != vkBindBufferMemory(device.getLogicalDevice(), staging_buffer, staging_memory, 0))
    {
        throw MiniEngineException("Error Creating Buffer");
    }

    uint8_t* data;
    if (VK_SUCCESS != vkMapMemory(device.getLogicalDevice(), staging_memory, 0, mem_reqs.size, 0, (void**)&data))
    {
    }

    memcpy(data, i_buffer, static_cast<size_t>(i_buffer_size));
    vkUnmapMemory(device.getLogicalDevice(), staging_memory);

    VkBufferImageCopy buffer_copy_region = {};
    buffer_copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    buffer_copy_region.imageSubresource.mipLevel = 0;
    buffer_copy_region.imageSubresource.baseArrayLayer = 0;
    buffer_copy_region.imageSubresource.layerCount = 1;
    buffer_copy_region.imageExtent.width = i_tex_width;
    buffer_copy_region.imageExtent.height = i_tex_height;
    buffer_copy_region.imageExtent.depth = 1;
    buffer_copy_region.bufferOffset = 0;

    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = i_format;
    image_create_info.mipLevels = mip_levels;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.extent = { i_tex_width, i_tex_height, 1 };
    image_create_info.usage = i_image_usage_flags;

    if (!(image_create_info.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
    {
        image_create_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    if (VK_SUCCESS != vkCreateImage(device.getLogicalDevice(), &image_create_info, nullptr, &o_new_image.m_image))
    {
        throw MiniEngineException("Error Creating Buffer");
    }

    vkGetImageMemoryRequirements(device.getLogicalDevice(), o_new_image.m_image, &mem_reqs);

    mem_alloc_info.allocationSize = mem_reqs.size;
    mem_alloc_info.memoryTypeIndex = device.getMemoryTypeIndex(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (VK_SUCCESS != vkAllocateMemory(device.getLogicalDevice(), &mem_alloc_info, nullptr, &o_new_image.m_memory))
    {
        throw MiniEngineException("Error Creating Buffer");
    }

    if (VK_SUCCESS != vkBindImageMemory(device.getLogicalDevice(), o_new_image.m_image, o_new_image.m_memory, 0))
    {
        throw MiniEngineException("Error Creating Buffer");
    }

    VkImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = mip_levels;
    subresource_range.layerCount = 1;

    setImageLayout(
        command_buffer,
        o_new_image.m_image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        subresource_range);

    vkCmdCopyBufferToImage(
        command_buffer,
        staging_buffer,
        o_new_image.m_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &buffer_copy_region
    );

    setImageLayout(
        command_buffer,
        o_new_image.m_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        i_image_layout,
        subresource_range);

    endOneTimeCommandBuffer(device, command_buffer);

    vkFreeMemory(device.getLogicalDevice(), staging_memory, nullptr);
    vkDestroyBuffer(device.getLogicalDevice(), staging_buffer, nullptr);

    VkSamplerCreateInfo sampler_create_info = {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter = i_filter;
    sampler_create_info.minFilter = i_filter;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.compareOp = VK_COMPARE_OP_NEVER;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = 0.0f;
    sampler_create_info.maxAnisotropy = 1.0f;

    if (VK_SUCCESS != vkCreateSampler(device.getLogicalDevice(), &sampler_create_info, nullptr, &o_new_image.m_sampler))
    {
        throw MiniEngineException("Error Creating Buffer");
    }

    VkImageViewCreateInfo view_create_info{};
    view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.pNext = NULL;
    view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_create_info.format = i_format;
    view_create_info.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    view_create_info.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    view_create_info.subresourceRange.levelCount = 1;
    view_create_info.image = o_new_image.m_image;

    if (VK_SUCCESS != vkCreateImageView(device.getLogicalDevice(), &view_create_info, nullptr, &o_new_image.m_image_view))
    {
        throw MiniEngineException("Error Creating Buffer");
    }
}


void UtilsVK::transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = 0;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
        newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else
    {
        throw std::runtime_error("Unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        cmd,
        srcStage, dstStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}


VkCommandBuffer UtilsVK::initOneTimeCommandBuffer(const DeviceVK& device)
{
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = device.getCommandPool();
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device.getLogicalDevice(), &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);
    UtilsVK::beginRegion(command_buffer, "One time Pass", Vector4f(0.0f, 0.0f, 0.5f, 1.0f));

    return command_buffer;
}


void UtilsVK::endOneTimeCommandBuffer(const DeviceVK& device, VkCommandBuffer io_command_buffer)
{
    UtilsVK::endRegion(io_command_buffer);
    vkEndCommandBuffer(io_command_buffer);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &io_command_buffer;

    vkQueueSubmit(device.getGraphicsQueue(), 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(device.getGraphicsQueue());
    vkFreeCommandBuffers(device.getLogicalDevice(), device.getCommandPool(), 1, &io_command_buffer);
}


int UtilsVK::getMultisampleCount_AA()
{
    int sampleCount = 1;
    if (runtimeVariables.antiAliasing == AntiAliasingType::MSAA2x) sampleCount = 2;
    else if (runtimeVariables.antiAliasing == AntiAliasingType::MSAA4x) sampleCount = 4;
    else if (runtimeVariables.antiAliasing == AntiAliasingType::MSAA8x) sampleCount = 8;
    return sampleCount;
}

VkSampleCountFlagBits UtilsVK::getSampleCountFlag_AA()
{
    switch (runtimeVariables.antiAliasing)
    {
    case AntiAliasingType::MSAA2x: return VK_SAMPLE_COUNT_2_BIT;
    case AntiAliasingType::MSAA4x: return VK_SAMPLE_COUNT_4_BIT;
    case AntiAliasingType::MSAA8x: return VK_SAMPLE_COUNT_8_BIT;
    default:                       return VK_SAMPLE_COUNT_1_BIT;
    }
}

VkSampleCountFlagBits UtilsVK::getCountFlag(int samples)
{
    switch (samples)
    {
    case 2:  return VK_SAMPLE_COUNT_2_BIT;
    case 4:  return VK_SAMPLE_COUNT_4_BIT;
    case 8:  return VK_SAMPLE_COUNT_8_BIT;
    default: return VK_SAMPLE_COUNT_1_BIT;
    }
}


void MiniEngine::UtilsVK::createBLAS(const DeviceVK& i_device,
    VkBuffer                     i_vertex_buffer,
    VkBuffer                     i_index_buffer,
    const std::vector<Vertex>& m_vertices,
    const std::vector<uint32_t>& i_indices,
    VkAccelerationStructureKHR& o_blas,
    VkBuffer& o_buffer,
    VkDeviceMemory& o_memory)
{
    // GEOMETRY INFO -----------------------------------------------------------
    VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress = {};
    VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress = {};
    VkDeviceOrHostAddressConstKHR voxelBufferDeviceAddress = {};

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;

    vertexBufferDeviceAddress.deviceAddress = get_device_address(i_device.getLogicalDevice(), i_vertex_buffer);
    if (!i_indices.empty())
        indexBufferDeviceAddress.deviceAddress = get_device_address(i_device.getLogicalDevice(), i_index_buffer);

    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    accelerationStructureGeometry.geometry.triangles.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
    accelerationStructureGeometry.geometry.triangles.maxVertex = (uint32_t)m_vertices.size() - 1;
    accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(Vertex);

    if (!i_indices.empty())
    {
        accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
    }

    accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
    accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;

    // ACCELERATION STRUCTURE INFO -----------------------------------------------------------
    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationStructureBuildGeometryInfo.geometryCount = 1;
    accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    const uint32_t numPrimitives = (uint32_t)i_indices.size() / 3;
    vkGetAccelerationStructureBuildSizes(i_device.getLogicalDevice(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &accelerationStructureBuildGeometryInfo,
        &numPrimitives,
        &accelerationStructureBuildSizesInfo);

    // CREATE ACCELERATION BUFFER -----------------------------------------------------------
    UtilsVK::createBuffer(
        i_device,
        accelerationStructureBuildSizesInfo.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        o_buffer,
        o_memory);

    // CREATE ACCELERATION STRUCTURE -----------------------------------------------------------
    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = o_buffer;
    accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    if (vkCreateAccelerationStructure(i_device.getLogicalDevice(), &accelerationStructureCreateInfo, nullptr, &o_blas) != VK_SUCCESS)
    {
        throw MiniEngineException("Error Creating Bottom-Level AS");
    }

    // Scratch buffer -----------------------------------------------------------
    VkBuffer       staging_buffer;
    VkDeviceMemory staging_memory;
    UtilsVK::createBuffer(i_device,
        accelerationStructureBuildSizesInfo.accelerationStructureSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        staging_buffer,
        staging_memory);

    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
    accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationBuildGeometryInfo.dstAccelerationStructure = o_blas;
    accelerationBuildGeometryInfo.geometryCount = 1;
    accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
    accelerationBuildGeometryInfo.scratchData.deviceAddress = get_device_address(i_device.getLogicalDevice(), staging_buffer);

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
    accelerationStructureBuildRangeInfo.primitiveCount = numPrimitives;
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

    VkCommandBuffer command_buffer = initOneTimeCommandBuffer(i_device);
    vkCmdBuildAccelerationStructures(command_buffer, 1, &accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos.data());
    endOneTimeCommandBuffer(i_device, command_buffer);

    vkDestroyBuffer(i_device.getLogicalDevice(), staging_buffer, nullptr);
    vkFreeMemory(i_device.getLogicalDevice(), staging_memory, nullptr);
}


void MiniEngine::UtilsVK::createTLAS(const DeviceVK& i_device,
    std::vector<Matrix4f>& i_transforms,
    std::vector<VkAccelerationStructureKHR>& i_blas_instances,
    VkAccelerationStructureKHR& o_tlas,
    VkBuffer& o_buffer,
    VkDeviceMemory& o_memory,
    VkBuffer& o_instances_buffer,
    VkDeviceMemory& o_instances_memory)
{
    // SUBSCRIBING BLAS INSTANCES -----------------------------------------------------------
    std::vector<VkAccelerationStructureInstanceKHR> instances;
    instances.resize(i_blas_instances.size(), {});

    for (size_t i = 0; i < i_blas_instances.size(); ++i)
    {
        VkTransformMatrixKHR transformMatrix = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 4; ++col)
                transformMatrix.matrix[row][col] = i_transforms[i][col][row]; // Column-major to Row-major

        instances[i].transform = transformMatrix;
        instances[i].instanceCustomIndex = i;
        instances[i].mask = 0xFF;
        instances[i].instanceShaderBindingTableRecordOffset = 0;
        instances[i].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;

        VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
        accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        accelerationDeviceAddressInfo.accelerationStructure = i_blas_instances[i];
        instances[i].accelerationStructureReference =
            vkGetAccelerationStructureDeviceAddress(i_device.getLogicalDevice(), &accelerationDeviceAddressInfo);
    }

    // Create a buffer for the instances -----------------------------------------------------------
    VkDeviceSize   instancesBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();
    UtilsVK::createBuffer(i_device,
        instancesBufferSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        o_instances_buffer,
        o_instances_memory);

    void* data;
    vkMapMemory(i_device.getLogicalDevice(), o_instances_memory, 0, instancesBufferSize, 0, &data);
    memcpy(data, instances.data(), instancesBufferSize);
    vkUnmapMemory(i_device.getLogicalDevice(), o_instances_memory);

    // GEOMETRY INFO -----------------------------------------------------------
    VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
    instanceDataDeviceAddress.deviceAddress = get_device_address(i_device.getLogicalDevice(), o_instances_buffer);

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

    // ACCELERATION STRUCTURE INFO -----------------------------------------------------------
    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationStructureBuildGeometryInfo.geometryCount = 1;
    accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

    uint32_t primitiveCount = static_cast<uint32_t>(instances.size());

    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizes(i_device.getLogicalDevice(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &accelerationStructureBuildGeometryInfo,
        &primitiveCount,
        &accelerationStructureBuildSizesInfo);

    // CREATE ACCELERATION BUFFER -----------------------------------------------------------
    UtilsVK::createBuffer(
        i_device,
        accelerationStructureBuildSizesInfo.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        o_buffer,
        o_memory);

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = o_buffer;
    accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

    if (vkCreateAccelerationStructure(i_device.getLogicalDevice(), &accelerationStructureCreateInfo, nullptr, &o_tlas) != VK_SUCCESS)
    {
        throw MiniEngineException("Error Creating Top-Level AS");
    }

    // Scratch buffer -----------------------------------------------------------
    VkBuffer       staging_buffer;
    VkDeviceMemory staging_memory;
    UtilsVK::createBuffer(i_device,
        accelerationStructureBuildSizesInfo.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        staging_buffer,
        staging_memory);

    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
    accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                                            VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationBuildGeometryInfo.dstAccelerationStructure = o_tlas;
    accelerationBuildGeometryInfo.geometryCount = 1;
    accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
    accelerationBuildGeometryInfo.scratchData.deviceAddress = get_device_address(i_device.getLogicalDevice(), staging_buffer);

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
    accelerationStructureBuildRangeInfo.primitiveCount = primitiveCount;
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

    VkCommandBuffer command_buffer = initOneTimeCommandBuffer(i_device);
    vkCmdBuildAccelerationStructures(command_buffer, 1, &accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos.data());
    endOneTimeCommandBuffer(i_device, command_buffer);

    vkDestroyBuffer(i_device.getLogicalDevice(), staging_buffer, nullptr);
    vkFreeMemory(i_device.getLogicalDevice(), staging_memory, nullptr);
}


uint64_t MiniEngine::UtilsVK::get_device_address(VkDevice i_device, VkBuffer i_buffer)
{
    VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
    bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAI.buffer = i_buffer;
    return vkGetBufferDeviceAddress(i_device, &bufferDeviceAI);
}

void MiniEngine::UtilsVK::updateTLAS(const DeviceVK& i_device,
    std::vector<Matrix4f>& i_transforms,
    std::vector<VkAccelerationStructureKHR>& i_blas_instances,
    VkBuffer& instances_buffer,
    VkDeviceMemory& instances_memory,
    VkAccelerationStructureKHR& o_tlas)
{
    //Instance update:
    std::vector<VkAccelerationStructureInstanceKHR> instances;
    instances.resize(i_blas_instances.size(), {});
    for (size_t i = 0; i < i_blas_instances.size(); ++i)
    {
        VkTransformMatrixKHR transformMatrix = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 4; ++col)
                transformMatrix.matrix[row][col] = i_transforms[i][col][row];
        instances[i].transform = transformMatrix;
        instances[i].instanceCustomIndex = i;
        instances[i].mask = 0xFF;
        instances[i].instanceShaderBindingTableRecordOffset = 0;
        instances[i].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
        accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        accelerationDeviceAddressInfo.accelerationStructure = i_blas_instances[i];
        instances[i].accelerationStructureReference =
            vkGetAccelerationStructureDeviceAddress(i_device.getLogicalDevice(), &accelerationDeviceAddressInfo);
    }
    //Buffer update:
    VkDeviceSize instancesBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();
    void* data;
    vkMapMemory(i_device.getLogicalDevice(), instances_memory, 0, instancesBufferSize, 0, &data);
    memcpy(data, instances.data(), instancesBufferSize);
    vkUnmapMemory(i_device.getLogicalDevice(), instances_memory);
    //Geometry info
    VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
    instanceDataDeviceAddress.deviceAddress = get_device_address(i_device.getLogicalDevice(), instances_buffer);
    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;
    //Scratch buffer
    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;
    uint32_t primitiveCount = static_cast<uint32_t>(instances.size());
    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
    accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
        VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
    accelerationBuildGeometryInfo.srcAccelerationStructure = o_tlas;
    accelerationBuildGeometryInfo.dstAccelerationStructure = o_tlas;
    accelerationBuildGeometryInfo.geometryCount = 1;
    accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
    VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
    buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizes(i_device.getLogicalDevice(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &accelerationBuildGeometryInfo,
        &primitiveCount,
        &buildSizesInfo);
    UtilsVK::createBuffer(i_device,
        buildSizesInfo.updateScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        staging_buffer,
        staging_memory);
    accelerationBuildGeometryInfo.scratchData.deviceAddress = get_device_address(i_device.getLogicalDevice(), staging_buffer);
    VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
    buildRangeInfo.primitiveCount = primitiveCount;
    buildRangeInfo.primitiveOffset = 0;
    buildRangeInfo.firstVertex = 0;
    buildRangeInfo.transformOffset = 0;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> buildRangeInfos = { &buildRangeInfo };
    VkCommandBuffer command_buffer = initOneTimeCommandBuffer(i_device);
    vkCmdBuildAccelerationStructures(command_buffer, 1, &accelerationBuildGeometryInfo, buildRangeInfos.data());
    endOneTimeCommandBuffer(i_device, command_buffer);

    vkDestroyBuffer(i_device.getLogicalDevice(), staging_buffer, nullptr);
    vkFreeMemory(i_device.getLogicalDevice(), staging_memory, nullptr);
}

void UtilsVK::copyAttachment(const DeviceVK& i_device, const VkCommandBuffer& current_cmd, ImageBlock& origin, ImageBlock& dest,
    unsigned int width, unsigned int height)
{
    UtilsVK::transitionImage(current_cmd,
        origin.m_image,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    UtilsVK::transitionImage(current_cmd,
        dest.m_image,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkImageCopy copy_region{};
    copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.srcSubresource.mipLevel = 0;
    copy_region.srcSubresource.baseArrayLayer = 0;
    copy_region.srcSubresource.layerCount = 1;
    copy_region.srcOffset = { 0, 0, 0 };
    copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.dstSubresource.mipLevel = 0;
    copy_region.dstSubresource.baseArrayLayer = 0;
    copy_region.dstSubresource.layerCount = 1;
    copy_region.dstOffset = { 0, 0, 0 };
    copy_region.extent = { width, height, 1 };
    vkCmdCopyImage(current_cmd,
        origin.m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dest.m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &copy_region);
    UtilsVK::transitionImage(current_cmd,
        origin.m_image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    UtilsVK::transitionImage(current_cmd,
        dest.m_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}