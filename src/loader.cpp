#include "data.hpp"
#include "files.hpp"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STB_IMAGE_IMPLEMENTATION
#include <tiny_gltf.h>
#include <stdexcept>

#include "global.hpp"
extern Application* app;

using namespace DATA;

// helper functions
VkFormat findTinyGLTFImageFormat(tinygltf::Image& image);
void unfoldTinyGLTFnodes(std::vector<tinygltf::Node>& output, tinygltf::Model& model, tinygltf::Node& parentNode);

Graph::Graph(const std::string modelPath, VkDevice backendDevice)
{
    d_device = backendDevice;
    std::string ext = FILES::get_file_extension(modelPath);
    if(ext == "gltf")
        loadModelGLTF(modelPath, false);
    else if(ext == "glb")
        loadModelGLTF(modelPath, true);
    else
        throw std::runtime_error("ERROR: unsupported model type for " + modelPath);
    createIndiceBuffers();
    createVertexBuffers();
    createUniformBuffers();
    createDescriptorSets();
}

// reference: https://github.com/syoyo/tinygltf/blob/master/examples/basic/main.cpp
// reference: https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/base/VulkanglTFModel.hpp
void Graph::loadModelGLTF(const std::string modelPath, bool binary)
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_GRAPH;

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    std::string path = std::string(GLOB_FILE_FOLDER) + "/" + modelPath;

    if(binary)
    {
        if(!loader.LoadBinaryFromFile(&model, &err, &warn, path))
            throw std::runtime_error("ERROR: failed to load gltf model " + path);
    }
    else
    {
        if(!loader.LoadASCIIFromFile(&model, &err, &warn, path))
            throw std::runtime_error("ERROR: failed to load gltf model " + path);
    }

    if(!warn.empty())
        if(myLogger){myLogger->AddMessage(myLoggerOwner, "gltf warn " + warn);}
    if(!err.empty())
        if(myLogger){myLogger->AddMessage(myLoggerOwner, "gltf error " + err);}

    // first load all textures (pre-load)
    for(size_t i = 0; i < model.textures.size(); i++)
    {
        tinygltf::Texture& tex = model.textures[i];
        // TODO: Add support for sampler as well
        if(tex.source < 0) continue;
        tinygltf::Image& image = model.images[tex.source];
        std::vector<unsigned char>& pixels = image.image;
        if(pixels.empty()) continue;

        Texture* newTexture = new Texture;

        VkDeviceSize imageSize = (uint64_t)image.height * (uint64_t)image.width * image.component;

        Buffer stagingBuffer = createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    	void* data;
    	vkMapMemory(d_device, stagingBuffer.mem, 0, imageSize, 0, &data);
    	memcpy(data, pixels.data(), static_cast<size_t>(imageSize));
    	vkUnmapMemory(d_device, stagingBuffer.mem);

        VkFormat imageFormat = findTinyGLTFImageFormat(image);

        uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(image.width, image.height)))) + 1;

    	newTexture->image = createTextureImage(static_cast<uint32_t>(image.width), static_cast<uint32_t>(image.height), mipLevels, imageFormat,
    	    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, stagingBuffer.buf);

    	VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = static_cast<float>(mipLevels);

		if (vkCreateSampler(d_device, &samplerInfo, nullptr, &newTexture->sampler) != VK_SUCCESS)
			throw std::runtime_error("ERROR: failed to create Vulkan texture sampler!");

		newTexture->allset = true;
    	stagingBuffer.destroy(d_device);
        d_unique_textures_int_map[static_cast<int>(i)] = newTexture;
    }
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "gltf model textures successfully loaded");}

    d_meshes.resize(0);
    d_ubo_per_mesh.resize(0);
    const tinygltf::Scene& scene = model.scenes[model.defaultScene];
    for(int nodeID : scene.nodes)
    {
        if(nodeID < 0 || nodeID >= (int)model.nodes.size()) throw std::runtime_error("ERROR: failed to load gltf model " + path);
        tinygltf::Node& node = model.nodes[nodeID];
        std::vector<tinygltf::Node> targetNodes;
        unfoldTinyGLTFnodes(targetNodes, model, node); // unfold all nodes
        for(auto& nn : targetNodes)
        {
            if(nn.mesh < 0 || nn.mesh >= (int)model.meshes.size()) continue;
            tinygltf::Mesh& mesh = model.meshes[nn.mesh];
            for(size_t i = 0; i < mesh.primitives.size(); i++)
            {
                Mesh newMesh{};
                std::vector<Vertex> vertices;
                vertices.resize(0);
                std::vector<uint32_t> indices;
                indices.resize(0);

                tinygltf::Primitive& primitive = mesh.primitives[i];

                // first find vertices
                if(primitive.attributes.find("POSITION") == primitive.attributes.end())
                    throw std::runtime_error("ERROR: no position info failed to load gltf model " + path);
                tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
                tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
                const float* bufferPos = reinterpret_cast<const float*>(&(model.buffers[posBufferView.buffer].data[posAccessor.byteOffset + posBufferView.byteOffset]));
                int posByteStride = posAccessor.ByteStride(posBufferView) ? posAccessor.ByteStride(posBufferView) / sizeof(float) : 3;

                // next try to find normal
                const float* bufferNormal = nullptr;
                int normByteStride;
                if(primitive.attributes.find("NORMAL") != primitive.attributes.end())
                {
                    tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
                    tinygltf::BufferView& normBufferView = model.bufferViews[normAccessor.bufferView];
                    bufferNormal = reinterpret_cast<const float*>(&(model.buffers[normBufferView.buffer].data[normAccessor.byteOffset + normBufferView.byteOffset]));
                    normByteStride = normAccessor.ByteStride(normBufferView) ? normAccessor.ByteStride(normBufferView) / sizeof(float) : 3;
                }

                // next try to find tangent
                const float* bufferTangent = nullptr;
                int tangByteStride;
                if(primitive.attributes.find("TANGENT") != primitive.attributes.end())
                {
                    tinygltf::Accessor& tangAccessor = model.accessors[primitive.attributes.find("TANGENT")->second];
                    tinygltf::BufferView& tangBufferView = model.bufferViews[tangAccessor.bufferView];
                    bufferTangent = reinterpret_cast<const float*>(&(model.buffers[tangBufferView.buffer].data[tangAccessor.byteOffset + tangBufferView.byteOffset]));
                    tangByteStride = tangAccessor.ByteStride(tangBufferView) ? tangAccessor.ByteStride(tangBufferView) / sizeof(float) : 4;
                }

                // next try to find first texture coordinate
                const float* bufferCoord0 = nullptr;
                int coord0ByteStride;
                if(primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
                {
                    tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                    tinygltf::BufferView& uvBufferView = model.bufferViews[uvAccessor.bufferView];
                    bufferCoord0 = reinterpret_cast<const float*>(&(model.buffers[uvBufferView.buffer].data[uvAccessor.byteOffset + uvBufferView.byteOffset]));
                    coord0ByteStride = uvAccessor.ByteStride(uvBufferView) ? uvAccessor.ByteStride(uvBufferView) / sizeof(float) : 2;
                }

                // finally try to find color
                const float* bufferColor = nullptr;
                int colorByteStride;
                if(primitive.attributes.find("COLOR_0") != primitive.attributes.end())
                {
                    tinygltf::Accessor& colorAccessor = model.accessors[primitive.attributes.find("COLOR_0")->second];
                    tinygltf::BufferView& colorBufferView = model.bufferViews[colorAccessor.bufferView];
                    bufferColor = reinterpret_cast<const float*>(&(model.buffers[colorBufferView.buffer].data[colorAccessor.byteOffset + colorBufferView.byteOffset]));
                    int defaultStride = colorAccessor.type == TINYGLTF_TYPE_VEC3 ? 3 : 4;
                    colorByteStride = colorAccessor.ByteStride(colorBufferView) ? colorAccessor.ByteStride(colorBufferView) / sizeof(float) : defaultStride;
                }

                // TODO: add joints and weight for skeleton in the future
                for (size_t v = 0; v < posAccessor.count; v++) {
					Vertex vert{};
					vert.pos = glm::make_vec3(&bufferPos[v * posByteStride]);
					vert.normal = glm::normalize(glm::vec3(bufferNormal ? glm::make_vec3(&bufferNormal[v * normByteStride]) : glm::vec3(0.0f)));
                    vert.tangent = bufferTangent ? glm::vec4(glm::make_vec4(&bufferTangent[v * tangByteStride])) : glm::vec4(0.0f);
					vert.coord = bufferCoord0 ? glm::make_vec2(&bufferCoord0[v * coord0ByteStride]) : glm::vec2(0.0f);
                    if(bufferColor && colorByteStride == 4)
                        vert.color = glm::make_vec4(&bufferColor[v * colorByteStride]);
                    else if(bufferColor && colorByteStride == 3)
                        vert.color = glm::vec4(glm::make_vec3(&bufferColor[v * colorByteStride]), 1.0f); // auto set alpha to 1
                    else
                        vert.color = glm::vec4(0.0f);
					
					vertices.push_back(vert);
				}
                newMesh.vertices = vertices;

                // next try to find indices
                if(primitive.indices > -1)
                {
                    tinygltf::Accessor& accessor = model.accessors[primitive.indices];
                    tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                    tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

                    indices.resize(accessor.count);
                    const void *dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

                    switch(accessor.componentType)
                    {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                        {
                            const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
                            for(size_t i = 0; i < accessor.count; i++)
                                indices[i] = buf[i];
                            break;
                        }
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        {
                            const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
                            for(size_t i = 0; i < accessor.count; i++)
                                indices[i] = static_cast<uint32_t>(buf[i]);
                            break;
                        }
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                        {
                            const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
                            for(size_t i = 0; i < accessor.count; i++)
                                indices[i] = static_cast<uint32_t>(buf[i]);
                            break;
                        }
                        default:
                            throw std::runtime_error("ERROR: unsupported indice type failed to load gltf model " + path);
                    }
                }
                newMesh.indices = indices;

                // next try to get the corresponding texture
                // TODO: add support for emissive factor
                // TODO: add support for alpha mode
                tinygltf::Material& material = model.materials[primitive.material];
                tinygltf::TextureInfo& info_base = material.pbrMetallicRoughness.baseColorTexture;
                tinygltf::TextureInfo& info_rough = material.pbrMetallicRoughness.metallicRoughnessTexture;
                tinygltf::NormalTextureInfo& info_normal = material.normalTexture;
                tinygltf::OcclusionTextureInfo& info_occlusion = material.occlusionTexture;
                tinygltf::TextureInfo& info_emissive = material.emissiveTexture;

                if(info_base.index >= 0 && info_base.index < (int)model.textures.size())
                    newMesh.texture_base = d_unique_textures_int_map[info_base.index];

                if(info_rough.index >= 0 && info_rough.index < (int)model.textures.size())
                    newMesh.texture_rough = d_unique_textures_int_map[info_rough.index];

                if(info_normal.index >= 0 && info_normal.index < (int)model.textures.size())
                    newMesh.texture_normal = d_unique_textures_int_map[info_normal.index];

                if(info_occlusion.index >= 0 && info_occlusion.index < (int)model.textures.size())
                    newMesh.texture_occlusion = d_unique_textures_int_map[info_occlusion.index];

                if(info_emissive.index >= 0 && info_emissive.index < (int)model.textures.size())
                    newMesh.texture_emissive = d_unique_textures_int_map[info_emissive.index];

                d_meshes.push_back(newMesh);

                CameraUniform ubo{};
                glm::mat4 model(1.0f);
                if(nn.translation.size() == 3)
                {
                    glm::vec3 translation = glm::make_vec3(nn.translation.data());
                    model = glm::translate(model, translation);
                }
                if(nn.rotation.size() == 4)
                {
                    glm::quat rotation = glm::make_quat(nn.rotation.data());
                    model = glm::mat4(rotation) * model;
                }
                if(nn.scale.size() == 3)
                {
                    glm::vec3 scale = glm::make_vec3(nn.scale.data());
                    model = glm::scale(model, scale);
                }
                ubo.model = model;
                d_ubo_per_mesh.push_back(ubo);
            }
        }
    }

    if(myLogger){myLogger->AddMessage(myLoggerOwner, "gltf model successfully loaded");}
}



// helper functions

VkFormat findTinyGLTFImageFormat(tinygltf::Image& image)
{
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    if(image.component == 1)
    {
        if(image.bits == 8) format = VK_FORMAT_R8_SRGB;
        else if(image.bits == 16) format = VK_FORMAT_R16_UNORM;
        else throw std::runtime_error("ERROR: unsupported tinygltf image bits");
    }
    else if(image.component == 2)
    {
        if(image.bits == 8) format = VK_FORMAT_R8G8_SRGB;
        else if(image.bits == 16) format = VK_FORMAT_R16G16_UNORM;
        else throw std::runtime_error("ERROR: unsupported tinygltf image bits");
    }
    else if(image.component == 3)
    {
        if(image.bits == 8) format = VK_FORMAT_R8G8B8_SRGB;
        else if(image.bits == 16) format = VK_FORMAT_R16G16B16_UNORM;
        else throw std::runtime_error("ERROR: unsupported tinygltf image bits");
    }
    else if(image.component == 4)
    {
        if(image.bits == 8) format = VK_FORMAT_R8G8B8A8_SRGB;
        else if(image.bits == 16) format = VK_FORMAT_R16G16B16A16_UNORM;
        else throw std::runtime_error("ERROR: unsupported tinygltf image bits");
    }
    else throw std::runtime_error("ERROR: unsupported tinygltf image component");
    return format;
}

void unfoldTinyGLTFnodes(std::vector<tinygltf::Node>& output, tinygltf::Model& model, tinygltf::Node& parentNode)
{
    output.push_back(parentNode);
    for(int nodeID : parentNode.children)
    {
        if(nodeID < 0 || nodeID >= (int)model.nodes.size()) throw std::runtime_error("ERROR: failed to unfold tinygltf nodes!");
        unfoldTinyGLTFnodes(output, model, model.nodes[nodeID]);
    }
}
