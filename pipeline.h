#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include "global.h"
#include "context.h"
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

	void createPipelinePRT(Context* context_, std::vector<VkDescriptorSetLayout>& setLayouts, RenderPass& pass);

	void createPipelineSkyBox(Context* context_, std::vector<VkDescriptorSetLayout>& setLayouts, RenderPass& pass);

	void createPipelineGBuffer(Context* context_, std::vector<VkDescriptorSetLayout>& setLayouts, RenderPass& pass);

	void createPipelineSSR(Context* context_, std::vector<VkDescriptorSetLayout>& setLayouts, RenderPass& pass);

	void destroy();
private:
	VkShaderModule createShaderModule(const std::vector<char>& code);

	//std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	//VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	//VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	//VkPipelineViewportStateCreateInfo viewportState{};
	//VkPipelineRasterizationStateCreateInfo rasterizer{};
	//VkPipelineMultisampleStateCreateInfo multisampling{};
	//VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	//VkPipelineColorBlendStateCreateInfo colorBlending{};
	//VkPipelineDynamicStateCreateInfo dynamicState{};
	//VkPipelineDepthStencilStateCreateInfo depthStencil{};
};