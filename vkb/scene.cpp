#include "scene.h"

#include "global.h"
#include "vkb/command_buffer.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure
#include <tiny_obj_loader.h>

#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>  // C++ importer interface

VkVertexInputBindingDescription Vertex::getBindingDescription() {
  VkVertexInputBindingDescription bindingDescription{};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription>
Vertex::getAttributeDescriptions() {
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(Vertex, pos);

  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(Vertex, normal);

  attributeDescriptions[2].binding = 0;
  attributeDescriptions[2].location = 2;
  attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

  return attributeDescriptions;
}

bool Vertex::operator==(const Vertex& other) const {
  return pos == other.pos && normal == other.normal &&
         texCoord == other.texCoord;
}

VkVertexInputBindingDescription VertexPrt::getBindingDescription() {
  VkVertexInputBindingDescription bindingDescription{};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(VertexPrt);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 5>
VertexPrt::getAttributeDescriptions() {
  std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};

  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(VertexPrt, pos);

  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(VertexPrt, texCoord);

  attributeDescriptions[2].binding = 0;
  attributeDescriptions[2].location = 2;
  attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[2].offset = offsetof(VertexPrt, transport0);

  attributeDescriptions[3].binding = 0;
  attributeDescriptions[3].location = 3;
  attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[3].offset = offsetof(VertexPrt, transport1);

  attributeDescriptions[4].binding = 0;
  attributeDescriptions[4].location = 4;
  attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[4].offset = offsetof(VertexPrt, transport2);

  return attributeDescriptions;
}

bool VertexPrt::operator==(const VertexPrt& other) const {
  return pos == other.pos && texCoord == other.texCoord &&
         transport0 == other.transport0 && transport1 == other.transport1 &&
         transport2 == other.transport2;
}

void Scene::loadObj(std::string path) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;
  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                        path.c_str())) {
    throw std::runtime_error(warn + err);
  }

  float ymin = FLT_MAX;

  Mesh mesh;
  std::unordered_map<Vertex, uint32_t> uniqueVertices{};
  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
      Vertex vertex{};
      vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]};
      vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                         1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
      vertex.normal = {attrib.normals[3 * index.normal_index + 0],
                       attrib.normals[3 * index.normal_index + 1],
                       attrib.normals[3 * index.normal_index + 2]};

      if (vertex.pos.y < ymin) ymin = vertex.pos.y;

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back(vertex);
      }
      mesh.indices.push_back(uniqueVertices[vertex]);
    }
  }

  mesh.type = COMMON;
  this->meshes.push_back(mesh);
}

void Scene::loadGLFT(std::string path) {
  // Create an instance of the Importer class
  Assimp::Importer importer;

  // And have it read the given file with some example postprocessing
  // Usually - if speed is not the most important aspect for you - you'll
  // probably to request more postprocessing than we do in this example.
  const aiScene* scene = importer.ReadFile(
      path, aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices);

  for (int i = scene->mNumMeshes - 1; i >= 0; --i) {
    Mesh m;

    m.type = COMMON;
    unsigned numVertices = scene->mMeshes[i]->mNumVertices;

    for (unsigned j = 0; j < numVertices; ++j) {
      Vertex v;
      v.pos.x = scene->mMeshes[i]->mVertices[j].x;
      v.pos.y = scene->mMeshes[i]->mVertices[j].y;
      v.pos.z = scene->mMeshes[i]->mVertices[j].z;
      v.normal.x = scene->mMeshes[i]->mNormals[j].x;
      v.normal.y = scene->mMeshes[i]->mNormals[j].y;
      v.normal.z = scene->mMeshes[i]->mNormals[j].z;
      v.texCoord.x = scene->mMeshes[i]->mTextureCoords[0][j].x;
      v.texCoord.y = scene->mMeshes[i]->mTextureCoords[0][j].y;

      m.vertices.push_back(v);
    }

    unsigned numFaces = scene->mMeshes[i]->mNumFaces;
    for (unsigned j = 0; j < numFaces; ++j) {
      m.indices.push_back(scene->mMeshes[i]->mFaces[j].mIndices[0]);
      m.indices.push_back(scene->mMeshes[i]->mFaces[j].mIndices[1]);
      m.indices.push_back(scene->mMeshes[i]->mFaces[j].mIndices[2]);
    }
    meshes.push_back(m);
  }
}

void Scene::cleanup() {
  meshes.resize(0);
  if (vertexBuffer.handle != VK_NULL_HANDLE) vertexBuffer.destroy();
  if (indexBuffer.handle != VK_NULL_HANDLE) indexBuffer.destroy();
}

void Scene::loadModelPrt(std::string path) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;
  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                        path.c_str())) {
    throw std::runtime_error(warn + err);
  }

  Mesh mesh;
  std::unordered_map<Vertex, uint32_t> uniqueVertices{};
  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
      Vertex vertex{};
      vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]};
      vertex.texCoord = {0.0f, 0.0f};
      vertex.normal = {0.0f, 0.0f, 0.0f};

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(mesh.vertices.size());
        VertexPrt vertexPRT;
        vertexPRT.pos = vertex.pos;
        vertexPRT.texCoord = vertex.texCoord;
        mesh.prtVertices.push_back(vertexPRT);
        mesh.vertices.push_back(vertex);
      }
      mesh.indices.push_back(uniqueVertices[vertex]);
    }
  }
  std::vector<Vertex>().swap(mesh.vertices);

  size_t pos = path.find_last_of('.');
  std::string tranportPath = path.substr(0, pos) + ".transport";

  std::fstream input(tranportPath, std::ios::in);
  for (int i = 0; i < mesh.indices.size(); ++i) {
    uint32_t index = mesh.indices[i];
    // input >> transportCoef[index][0][0] >> transportCoef[index][0][1] >>
    // transportCoef[index][0][2]
    //	>> transportCoef[index][1][0] >> transportCoef[index][1][1] >>
    // transportCoef[index][1][2]
    //	>> transportCoef[index][2][0] >> transportCoef[index][2][1] >>
    // transportCoef[index][2][2];
    input >> mesh.prtVertices[index].transport0[0] >>
        mesh.prtVertices[index].transport0[1] >>
        mesh.prtVertices[index].transport0[2] >>
        mesh.prtVertices[index].transport1[0] >>
        mesh.prtVertices[index].transport1[1] >>
        mesh.prtVertices[index].transport1[2] >>
        mesh.prtVertices[index].transport2[0] >>
        mesh.prtVertices[index].transport2[1] >>
        mesh.prtVertices[index].transport2[2];
  }
  input.close();

  // loadPrecomputedData(PRECOMPUTED_TRANSPORT_PATH, mesh);
  // for (int i = 0; i < mesh.prtVertices.size(); ++i) {
  //   mesh.prtVertices[i].transport0 = glm::vec3(
  //       transportCoef[i][0][0], transportCoef[i][0][1],
  //       transportCoef[i][0][2]);
  //   mesh.prtVertices[i].transport1 = glm::vec3(
  //       transportCoef[i][1][0], transportCoef[i][1][1],
  //       transportCoef[i][1][2]);
  //   mesh.prtVertices[i].transport2 = glm::vec3(
  //       transportCoef[i][2][0], transportCoef[i][2][1],
  //       transportCoef[i][2][2]);
  // }
  mesh.type = PRT;
  this->meshes.push_back(mesh);
}

/// @brief upload vertex and index to GPU. Note that all vertices in the vertex
/// buffer must be the same type.
/// @param context
void Scene::createGeometryBuffer(Context* context) {
  VkDeviceSize bufferSize = 0;
  VkDeviceSize maxSize = 0;
  uint32_t numIndicesAll = 0;
  uint32_t numVerticesAll = 0;
  uint32_t maxNumIndices = 0;

  for (Mesh& m : meshes) {
    m.vertexOffset = numVerticesAll;
    m.indexOffset = numIndicesAll;
    VkDeviceSize meshSize = m.type == COMMON
                                ? m.vertices.size() * sizeof(Vertex)
                                : m.prtVertices.size() * sizeof(VertexPrt);
    bufferSize += meshSize;
    maxSize = meshSize > maxSize ? meshSize : maxSize;
    numIndicesAll += m.indices.size();
    numVerticesAll +=
        m.type == COMMON ? m.vertices.size() : m.prtVertices.size();
    maxNumIndices =
        m.indices.size() > maxNumIndices ? m.indices.size() : maxNumIndices;
  }

  vertexBuffer.create(
      context,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      bufferSize, false);

  Buffer stagingBuffer;
  stagingBuffer.create(context, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, maxSize,
                       true);
  void* pData;
  VkDeviceSize offset = 0;
  for (int i = 0; i < meshes.size(); ++i) {
    VkDeviceSize size = meshes[i].type == COMMON
                            ? meshes[i].vertices.size() * sizeof(Vertex)
                            : meshes[i].prtVertices.size() * sizeof(VertexPrt);
    vmaMapMemory(context->allocator, stagingBuffer.allocation, &pData);
    void* pMesh = meshes[i].type == COMMON
                      ? static_cast<void*>(meshes[i].vertices.data())
                      : static_cast<void*>(meshes[i].prtVertices.data());
    memcpy(pData, pMesh, size);
    vmaUnmapMemory(context->allocator, stagingBuffer.allocation);
    CommandBuffer commandBuffer =
        CommandBuffer::beginSingleTimeCommands(context);
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = offset;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer.handle, stagingBuffer.handle,
                    vertexBuffer.handle, 1, &copyRegion);
    CommandBuffer::endSingleTimeCommands(commandBuffer);
    offset += size;
  }
  stagingBuffer.destroy();

  bufferSize = numIndicesAll * sizeof(uint32_t);
  indexBuffer.create(
      context,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      bufferSize, false);
  maxSize = maxNumIndices * sizeof(uint32_t);
  stagingBuffer.create(context, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, maxSize,
                       true);
  offset = 0;
  for (int i = 0; i < meshes.size(); ++i) {
    VkDeviceSize size = meshes[i].indices.size() * sizeof(uint32_t);

    vmaMapMemory(context->allocator, stagingBuffer.allocation, &pData);
    memcpy(pData, meshes[i].indices.data(), size);
    vmaUnmapMemory(context->allocator, stagingBuffer.allocation);
    CommandBuffer commandBuffer =
        CommandBuffer::beginSingleTimeCommands(context);
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = offset;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer.handle, stagingBuffer.handle,
                    indexBuffer.handle, 1, &copyRegion);
    CommandBuffer::endSingleTimeCommands(commandBuffer);
    offset += size;
  }
  stagingBuffer.destroy();
}