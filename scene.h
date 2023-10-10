#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <array>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "buffer.h"

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
	bool operator==(const Vertex& other) const;
};

namespace std {
	template <>
	struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >>
				1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}  // namespace std

struct Mesh {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

class Scene {
public:
	void loadModel(Context* context);
	void cleanup();

	Buffer vertexBuffer;
	Buffer indexBuffer;

	std::vector<Mesh> meshes;
};
