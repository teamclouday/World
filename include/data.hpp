// File Description
// data structures
// used for renderer

#pragma once

#include <vulkan/vulkan.h>

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
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct NodeUniformData
    {
        glm::mat4 localTransformation = glm::mat4(1.0f);
    };

    struct MeshConstantData
    {
        float hasBase       = 0.0f; // 0.0 means false
        float hasRough      = 0.0f;
        float hasNormal     = 0.0f;
        float hasOcclusion  = 0.0f;
        float hasEmissive   = 0.0f;
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

    struct GraphUserInput
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::string textureImagePath;
    };

    struct Mesh
    {
        // for rendering
        uint32_t indiceStart;
        uint32_t indiceCount = 0;
        uint32_t vertexStart;
        uint32_t vertexCount = 0;
        // texture bindings
        uint32_t texBase      = 0; // binding = 2
        uint32_t texRough     = 0; // binding = 3
        uint32_t texNormal    = 0; // binding = 4
        uint32_t texOcclusion = 0; // binding = 5
        uint32_t texEmissive  = 0; // binding = 6
        // descriptor set reference
        uint32_t meshID = 0; // for referencing descriptor set
        uint32_t nodeID = 0; // for referencing the node
    };

    struct Node
    {
        uint32_t nodeID;
        std::vector<uint32_t> meshIDs;
        Node* parentNode = nullptr;
        std::vector<Node*> childrenNodes;
        glm::mat4 transformMat = glm::mat4(1.0f);
        void destroy()
        {
            for(auto& ptr : childrenNodes)
            {
                ptr->destroy();
                delete ptr;
            }
        }
    };

    class Graph
    {
    public:
        Graph(std::vector<GraphUserInput>& meshes, VkDevice backendDevice);
        ~Graph();

        static Graph* newGraph(std::vector<GraphUserInput>& meshes, VkDevice backendDevice)
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
        // update render command buffer by id
        void updateRenderCommandBuffer(uint32_t imageID);
        // frame size change callback
        void onFrameSizeChangeStart();
        void onFrameSizeChangeEnd();

    private:
        // process input meshes
        void convertInputMeshes(std::vector<GraphUserInput>& meshes);
        // create uniform buffers for each mesh
        void createUniformBuffers();
        // create descriptor set related variables
        void createDescriptorSets();
        // vertex buffers
        void createVertexBuffers(std::vector<GraphUserInput>& meshes);
        // indice buffers
        void createIndiceBuffers(std::vector<GraphUserInput>& meshes);
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
        // initialize an empty texture
        void initTextures();

        // model loading related functions
        // load model type gltf
        std::vector<GraphUserInput> loadModelGLTF(const std::string modelPath, bool binary);

    public:
        std::vector<Node*> d_nodes;
        std::vector<Mesh*> d_meshes;
        std::vector<MeshConstantData> d_mesh_constants; // size of d_meshes
        std::vector<std::vector<Buffer>> d_node_uniform_buffers; // size of d_nodes * swap chain images
        bool d_node_uniform_buffers_need_update = true;

        std::vector<Texture> d_unique_textures;

        std::vector<Buffer> d_ubo_buffers; // size of swap chain images
        CameraUniform d_ubo_data;
        
        VkDescriptorSetLayout d_descriptor_layout;
        VkDescriptorPool d_descriptor_pool;
        std::vector<std::vector<VkDescriptorSet>> d_descriptor_per_mesh;
        std::vector<VkDescriptorSet> d_descriptor_ubo; // size of swap chain images

        Buffer d_vertex_buffer; // all vertex data
        Buffer d_indice_buffer; // all indice data
        uint32_t d_indice_count = 0;

        std::vector<VkCommandBuffer> d_commands;

    private:
        VkDevice d_device;
    };
}