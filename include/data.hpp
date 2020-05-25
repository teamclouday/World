// File Description
// data structures
// used for renderer

#pragma once

#include <Vulkan/Vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>
#include <array>
#include <map>
#include <set>

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

    // reference possible types
    // https://github.com/KhronosGroup/glTF/tree/master/specification/2.0/#meshes
    struct Vertex
    {
        glm::vec3 pos; // POSITION
        glm::vec3 normal; // NORMAL
        glm::vec4 tangent; // TANGENT
        glm::vec2 coord; // TEXCOORD_0
        glm::vec4 color; // COLOR_0

        static VkVertexInputBindingDescription getBindingDescription()
	    {
	    	VkVertexInputBindingDescription bindingDescription{};
	    	bindingDescription.binding = 0;
	    	bindingDescription.stride = sizeof(Vertex);
	    	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	    	return bindingDescription;
	    }

	    static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions()
	    {
	    	std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};
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
	    	attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	    	attributeDescriptions[2].offset = offsetof(Vertex, tangent);

            attributeDescriptions[3].binding = 0;
	    	attributeDescriptions[3].location = 3;
	    	attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
	    	attributeDescriptions[3].offset = offsetof(Vertex, coord);

            attributeDescriptions[4].binding = 0;
	    	attributeDescriptions[4].location = 4;
	    	attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	    	attributeDescriptions[4].offset = offsetof(Vertex, color);

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
        VkDescriptorPool pool;
        VkDescriptorSetLayout layout;
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
        Texture* texture_base       = nullptr; // Binding = 1 // 0 is taken by uniform buffer
        Texture* texture_rough      = nullptr; // Binding = 2
        Texture* texture_normal     = nullptr; // Binding = 3
        Texture* texture_occlusion  = nullptr; // Binding = 4
        Texture* texture_emissive   = nullptr; // Binding = 5
        bool allset = false;
        void destroy(VkDevice device)
        {
            if(!allset) return;
            vertices.clear();
            vertices.resize(0);
            indices.clear();
            indices.resize(0);
            // texture.destroy(device); // texture resources should not be managed by mesh
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

        Graph(const std::string modelPath, VkDevice backendDevice);

        static Graph* newGraph(const std::string modelPath, VkDevice backendDevice)
        {
            Graph* newGraph = new Graph(modelPath, backendDevice);
            return newGraph;
        }

        // create render command buffers
        void createRenderCommandBuffers();
        // frame size change callback
        void onFrameSizeChangeStart();
        void onFrameSizeChangeEnd();

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
        // create textures from image paths
        void createTexturesFromPaths(const std::set<std::string> paths);
        // create buffer helper function
        Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
        // create texture image helper function
        Image createTextureImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat imageFormat,
            VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer stagingBuffer);
        // create mipmaps for texture image
        void createTextureImageMipmaps(VkImage& image, VkFormat imageFormat, int32_t width, int32_t height, uint32_t mipLevels);
        // transition texture image layout
        void transitionTextureImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
        // copy buffer to image helper function
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        // copy buffer to buffer helper function
        void copyBufferToBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        // model loading related functions
        // load model type gltf
        void loadModelGLTF(const std::string modelPath, bool binary);

    public:
        std::vector<Mesh> d_meshes;
    
        std::map<std::string, Texture*> d_unique_textures_string_map;
        std::map<int, Texture*> d_unique_textures_int_map;
    
        DescriptorSet d_desctiptor_sets;

        std::vector<CameraUniform> d_ubo_per_mesh;
        std::vector<Buffer> d_ubo_buffers;
        
        std::vector<Buffer> d_vertex_buffers;
        std::vector<Buffer> d_indice_buffers;

        std::vector<VkCommandBuffer> d_commands;

    private:
        VkDevice d_device;
    };
}