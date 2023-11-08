#pragma once
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <assimp/Importer.hpp>
#include <glm/gtx/hash.hpp>

#include "buffer.h"
#include "global.h"

enum VertexType { COMMON, PRT };

struct Vertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 texCoord;

  static VkVertexInputBindingDescription getBindingDescription();
  static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptions();
  bool operator==(const Vertex& other) const;
};

struct VertexPrt {
  glm::vec3 pos;
  glm::vec2 texCoord;
  glm::vec3 transport0;
  glm::vec3 transport1;
  glm::vec3 transport2;

  static VkVertexInputBindingDescription getBindingDescription();

  static std::array<VkVertexInputAttributeDescription, 5>
  getAttributeDescriptions();

  bool operator==(const VertexPrt& other) const;
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
  enum VertexType type;
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  std::vector<VertexPrt> prtVertices;

  VkDeviceSize vertexOffset;
  VkDeviceSize indexOffset;
};

struct Material {
  glm::vec3 albedo;
  glm::vec3 ambient;
  std::string textureName;
};

class Scene {
 public:
  void loadObj(std::string path);
  void loadModelPrt(std::string path);
  void loadGLFT(std::string path);
  void cleanup();
  void createGeometryBuffer(Context* context);

  Buffer vertexBuffer{VK_NULL_HANDLE};
  Buffer indexBuffer{VK_NULL_HANDLE};

  std::vector<Mesh> meshes;
};
