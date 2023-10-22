#include "common.h"
#include "command_buffer.h"

std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

void copy(Context* context, Image& dst, Buffer& src, uint32_t width, uint32_t height) {
	CommandBuffer cb = CommandBuffer::beginSingleTimeCommands(context);
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };
	vkCmdCopyBufferToImage(cb.handles[0], src.handle, dst.handle,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	CommandBuffer::endSingleTimeCommands(cb);
}

void copy(Context* context, Buffer& dst, Image& src, uint32_t mipLevel, uint32_t width, uint32_t height) {
	CommandBuffer cb = CommandBuffer::beginSingleTimeCommands(context);
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = src.aspectFlags;
	region.imageSubresource.mipLevel = mipLevel;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };
	vkCmdCopyImageToBuffer(cb.handles[0], src.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst.handle, 1, &region);
	CommandBuffer::endSingleTimeCommands(cb);
}