#include "texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <string>
#include <array>

#include "buffer.h"
#include "command_buffer.h"
#include "common.h"

void Texture::generateMipmaps() {
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(context->gpu, image.format,
                                      &formatProperties);
  if (!(formatProperties.optimalTilingFeatures &
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    throw std::runtime_error(
        "texture image format does not support linear blitting!");
  }

  CommandBuffer cb = CommandBuffer::beginSingleTimeCommands(context);
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image = image.handle;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = this->image.aspectFlags;
  barrier.subresourceRange.baseArrayLayer = 0;
  // barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.layerCount = image.layers;
  barrier.subresourceRange.levelCount = 1;

  int32_t mipWidth = image.width;
  int32_t mipHeight = image.height;
  for (uint32_t i = 1; i < image.mipLevels; ++i) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(cb.handle, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);

    // start edit
    std::vector<VkImageBlit> imageBlits;
    for (unsigned j = 0; j < image.layers; ++j) {
      VkImageBlit blit{};
      blit.srcOffsets[0] = {0, 0, 0};
      blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
      blit.srcSubresource.aspectMask = this->image.aspectFlags;
      blit.srcSubresource.mipLevel = i - 1;
      blit.srcSubresource.baseArrayLayer = j;
      blit.srcSubresource.layerCount = 1;
      blit.dstOffsets[0] = {0, 0, 0};
      blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1,
                            mipHeight > 1 ? mipHeight / 2 : 1, 1};
      blit.dstSubresource.aspectMask = this->image.aspectFlags;
      blit.dstSubresource.mipLevel = i;
      blit.dstSubresource.baseArrayLayer = j;
      blit.dstSubresource.layerCount = 1;
      imageBlits.push_back(blit);
    }

    // VkImageBlit blit{};
    // blit.srcOffsets[0] = { 0, 0, 0 };
    // blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
    // blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // blit.srcSubresource.mipLevel = i - 1;
    // blit.srcSubresource.baseArrayLayer = 0;
    // blit.srcSubresource.layerCount = 1;
    // blit.dstOffsets[0] = { 0, 0, 0 };
    // blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1,
    //					  mipHeight > 1 ? mipHeight / 2 : 1, 1
    //}; blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // blit.dstSubresource.mipLevel = i;
    // blit.dstSubresource.baseArrayLayer = 0;
    // blit.dstSubresource.layerCount = 1;
    vkCmdBlitImage(cb.handle, image.handle,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image.handle,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   static_cast<uint32_t>(imageBlits.size()), imageBlits.data(),
                   VK_FILTER_LINEAR);
    // end edit

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cb.handle, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &barrier);
    if (mipWidth > 1) mipWidth /= 2;
    if (mipHeight > 1) mipHeight /= 2;
  }

  barrier.subresourceRange.baseMipLevel = image.mipLevels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  vkCmdPipelineBarrier(cb.handle, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);

  CommandBuffer::endSingleTimeCommands(cb);
}

void Texture::create(Context* context_, uint32_t texWidth, uint32_t texHeight,
                     VkFormat format, VkImageUsageFlags usage,
                     VkImageAspectFlagBits aspectFlags, bool depthMipmap) {
  context = context_;

  uint32_t mipLevels =
      depthMipmap ? static_cast<uint32_t>(
                        std::floor(std::log2(std::max(texWidth, texHeight)))) +
                        1
                  : 1;

  image.create(context, texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT,
               format, usage, aspectFlags);

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  samplerInfo.anisotropyEnable = VK_TRUE;

  VkPhysicalDeviceProperties gpuProperties = context->gpuProperties;

  samplerInfo.maxAnisotropy = gpuProperties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 1.0f;
  if (vkCreateSampler(context->device, &samplerInfo, nullptr, &sampler) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }
}

void Texture::create(Context* context_, std::string path) {
  context = context_;

  int texWidth, texHeight, texChannels;
  stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels,
                              STBI_rgb_alpha);
  if (!pixels) {
    throw std::runtime_error("failed to load texture image");
  }
  VkDeviceSize imageSize = texWidth * texHeight * 4;
  uint32_t mipLevels = static_cast<uint32_t>(std::floor(
                           std::log2(std::max(texWidth, texHeight)))) +
                       1;

  Buffer stagingBuffer;
  stagingBuffer.create(context, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, imageSize,
                       true);

  stagingBuffer.upload(pixels, false);
  stbi_image_free(pixels);

  image.create(context, texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT,
               VK_FORMAT_R8G8B8A8_SRGB,
               VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                   VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
               VK_IMAGE_ASPECT_COLOR_BIT);
  image.transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  copy(context, image, stagingBuffer, texWidth, texHeight);
  stagingBuffer.destroy();

  generateMipmaps();

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_TRUE;

  VkPhysicalDeviceProperties properties = context->gpuProperties;

  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = static_cast<float>(mipLevels);
  if (vkCreateSampler(context->device, &samplerInfo, nullptr, &sampler) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }
}

void Texture::create(Context* cnt, std::array<std::string, 6> path) {
  context = cnt;
  std::array<stbi_uc*, 6> pixels;
  int texWidth, texHeight, texChannels;
  for (int i = 0; i < 6; ++i) {
    int width, height, channels;
    pixels[i] = stbi_load(path[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels[i]) throw std::runtime_error("failed to load texture image");
    if (i == 0) {
      texWidth = width;
      texHeight = height;
      texChannels = channels;
    } else if (width != texWidth || height != texHeight ||
               channels != texChannels) {
      throw std::runtime_error("images of cube map have different size");
    }
  }
  VkDeviceSize imageSize = texWidth * texHeight * 4;
  uint32_t mipLevels = static_cast<uint32_t>(std::floor(
                           std::log2(std::max(texWidth, texHeight)))) +
                       1;

  std::vector<stbi_uc> allPixels(imageSize * 6);
  for (int i = 0; i < 6; ++i) {
    memcpy(allPixels.data() + i * imageSize, pixels[i], imageSize);
  }

  Buffer stagingBuffer;
  stagingBuffer.create(context, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, imageSize * 6,
                       true);
  void* pData;
  vmaMapMemory(context->allocator, stagingBuffer.allocation, &pData);
  memcpy(pData, allPixels.data(), imageSize * 6);
  vmaUnmapMemory(context->allocator, stagingBuffer.allocation);

  image.create(context, texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT,
               VK_FORMAT_R8G8B8A8_SRGB,
               VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                   VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
               VK_IMAGE_ASPECT_COLOR_BIT, true);
  image.transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  CommandBuffer cb = CommandBuffer::beginSingleTimeCommands(context);
  std::vector<VkBufferImageCopy> bufferCopyRegions;
  for (uint32_t face = 0; face < 6; face++) {
    // Calculate offset into staging buffer for the current face
    VkBufferImageCopy bufferCopyRegion = {};
    bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferCopyRegion.imageSubresource.mipLevel = 0;
    bufferCopyRegion.imageSubresource.baseArrayLayer = face;
    bufferCopyRegion.imageSubresource.layerCount = 1;
    bufferCopyRegion.imageExtent = {(uint32_t)texWidth, (uint32_t)texHeight, 1};
    bufferCopyRegion.bufferOffset = imageSize * face;
    bufferCopyRegions.push_back(bufferCopyRegion);
  }
  vkCmdCopyBufferToImage(cb.handle, stagingBuffer.handle, image.handle,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         static_cast<uint32_t>(bufferCopyRegions.size()),
                         bufferCopyRegions.data());
  CommandBuffer::endSingleTimeCommands(cb);
  stagingBuffer.destroy();

  generateMipmaps();
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_TRUE;

  VkPhysicalDeviceProperties properties = context->gpuProperties;

  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = static_cast<float>(mipLevels);
  if (vkCreateSampler(context->device, &samplerInfo, nullptr, &sampler) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }
}

void Texture::destroy() {
  vkDestroySampler(context->device, sampler, nullptr);
  image.destroy();
}
