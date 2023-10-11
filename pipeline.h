#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include"context.h"
#include "renderpass.h"

class Pipeline {
public:
	VkPipeline handle;
	VkPipelineLayout layout;
	std::vector<std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT>> descriptorSets;

	Context* context{ nullptr };

	void createShadowPipeline(Context* context_, std::vector<VkDescriptorSetLayout>& setLayouts, RenderPass& pass);

	void createPipeline(Context* context_, std::vector<VkDescriptorSetLayout>& setLayouts, RenderPass& pass);

	void createPipelineNoTexture(Context* context_, std::vector<VkDescriptorSetLayout>& setLayouts, RenderPass& pass);

	void destroy();
private:
	VkShaderModule createShaderModule(const std::vector<char>& code);
};