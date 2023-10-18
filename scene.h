#pragma once
#include <array>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include "buffer.h"
#include "global.h"

#include <assimp/Importer.hpp>

using vec3 = glm::vec3;
using vec2 = glm::vec2;
using mat3 = glm::mat3;
using mat4 = glm::mat4;

struct Vertex {
	vec3 pos;
	vec3 normal;
	vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
	bool operator==(const Vertex& other) const;
};

struct VertexPRT {
	vec3 pos;
	vec2 texCoord;
	vec3 transport0;
	vec3 transport1;
	vec3 transport2;
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(VertexPRT);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}
	static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(VertexPRT, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(VertexPRT, texCoord);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(VertexPRT, transport0);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(VertexPRT, transport1);

		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[4].offset = offsetof(VertexPRT, transport2);

		return attributeDescriptions;
	}

	bool operator==(const VertexPRT& other) const {
		return pos == other.pos && texCoord == other.texCoord && transport0 == other.transport0 && transport1 == other.transport1 && transport2 == other.transport2;
	}
};

namespace std {
	template <>
	struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}  // namespace std

struct Mesh {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<VertexPRT> verticesPRT;

	VkDeviceSize vertexOffset;
	VkDeviceSize indexOffset;
};

struct Material {
	vec3 albedo;
	vec3 ambient;
};

class Scene {
public:
	void loadModel(Context* context);
	void loadGLFT(Context* context);
	void cleanup();

	Buffer vertexBuffer{ VK_NULL_HANDLE };
	Buffer indexBuffer{ VK_NULL_HANDLE };

	std::vector<Mesh> meshes;

	/***************************************************************************/
	std::vector<mat3> lightCoef;
	std::vector<mat3> transportCoef;
	void loadModelPRT(Context* context);
	void loadPrecomputedData(std::string lightPath, std::string transportPath);
	/***************************************************************************/
};
