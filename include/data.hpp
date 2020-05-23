// File Description
// data structures
// used for renderer

#pragma once

#include <Vulkan/Vulkan.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include <vector>
#include <string>
#include <array>

namespace DATA
{
    enum ShaderTypes
    {
        SHADER_VERTEX        = 0,
        SHADER_TESS_CONTROL  = 1,
        SHADER_TESS_EVALUATE = 2,
        SHADER_GEOMETRY      = 3,
        SHADER_FRAGMENT      = 4,
        SHADER_COMPUTE       = 5,
    };

    struct ShaderSourceDetails
    {
        std::vector<ShaderTypes> types;
        std::vector<std::string> names;
        std::string path = ".";

        bool validate()
        {
            if (!types.size()) return false;
            if (!names.size()) return false;
            return types.size() == names.size();
        }
    };

    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 coord;

        static VkVertexInputBindingDescription getBindingDescription()
	    {
	    	VkVertexInputBindingDescription bindingDescription{};
	    	bindingDescription.binding = 0;
	    	bindingDescription.stride = sizeof(Vertex);
	    	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	    	return bindingDescription;
	    }

	    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	    {
	    	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
	    	attributeDescriptions[0].binding = 0;
	    	attributeDescriptions[0].location = 0;
	    	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	    	attributeDescriptions[0].offset = offsetof(Vertex, pos);
    
	    	attributeDescriptions[1].binding = 0;
	    	attributeDescriptions[1].location = 1;
	    	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	    	attributeDescriptions[1].offset = offsetof(Vertex, color);

	    	attributeDescriptions[2].binding = 0;
	    	attributeDescriptions[2].location = 2;
	    	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	    	attributeDescriptions[2].offset = offsetof(Vertex, coord);

	    	return attributeDescriptions;
	    }
    };

    struct CameraUniform
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct Image
    {
        VkImage image;
        VkImageView view;
        VkDeviceMemory mem;
        bool allset = false;

        void destroy(VkDevice device)
        {
            if(!allset) return;
            vkDestroyImageView(device, view, nullptr);
            vkDestroyImage(device, image, nullptr);
            vkFreeMemory(device, mem, nullptr);
            allset = false;
        }
    };

    struct Buffer
    {
        VkBuffer buf;
        VkDeviceMemory mem;
        bool allset = false;
        void destroy(VkDevice device)
        {
            if(!allset) return;
            vkDestroyBuffer(device, buf, nullptr);
            vkFreeMemory(device, mem, nullptr);
            allset = false;
        }
    };

    struct DescriptorSet
    {
        VkDescriptorSetLayout layout;
        VkDescriptorPool pool;
        std::vector<VkDescriptorSet> sets;
        bool allset = false;
        void destroy(VkDevice device)
        {
            if(!allset) return;
            vkDestroyDescriptorSetLayout(device, layout, nullptr);
            vkDestroyDescriptorPool(device, pool, nullptr);
            sets.clear();
            sets.resize(0);
            allset = false;
        }
    };

    struct Texture
    {
        Image image;
        VkSampler sampler;
        bool allset = false;
        void destroy(VkDevice device)
        {
            if(!allset) return;
            image.destroy(device);
            vkDestroySampler(device, sampler, nullptr);
            allset = false;
        }
    };

    struct Mesh
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        Texture texture;
        bool allset = false;
        void destroy(VkDevice device)
        {
            if(!allset) return;
            vertices.clear();
            vertices.resize(0);
            indices.clear();
            indices.resize(0);
            texture.destroy(device);
            allset = false;
        }
    };

    struct MeshInput
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::string textureImagePath;
    };

    class Graph
    {
    public:
        Graph(std::vector<MeshInput>& meshes, VkDevice backendDevice);
        ~Graph();

        static Graph* newGraph(std::vector<MeshInput>& meshes, VkDevice backendDevice)
        {
            Graph* newGraph = new Graph(meshes, backendDevice);
            return newGraph;
        }

        // create render command buffers
        void createRenderCommandBuffers();

    private:
        // process input meshes
        void convertInputMeshes(std::vector<MeshInput>& meshes);
        // create uniform buffers for each mesh
        void createUniformBuffers();
        // create descriptor set related variables
        void createDescriptorSets();
        // vertex buffers
        void createVertexBuffers();
        // indice buffers
        void createIndiceBuffers();
        // create texture from image
        Texture createTexture(const std::string path);
        // create buffer helper function
        Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
        // create texture image helper function
        Image createTextureImage(uint32_t width, uint32_t height, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer stagingBuffer);
        // transition texture image layout
        void transitionTextureImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        // copy buffer to image helper function
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        // copy buffer to buffer helper function
        void copyBufferToBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        // frame size change callback
        void onFrameSizeChange();

    public:
        VkDevice d_device;

        std::vector<Mesh> d_meshes;
        DescriptorSet d_desctiptor_sets;

        std::vector<CameraUniform> d_ubo_per_mesh;
        std::vector<Buffer> d_ubo_buffers;
        
        std::vector<Buffer> d_vertex_buffers;
        std::vector<Buffer> d_indice_buffers;

        std::vector<VkCommandBuffer> d_commands;
    };
}