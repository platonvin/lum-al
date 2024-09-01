#pragma once

#include <iostream>
#include <optional>
#include <vector>

// #undef __STRICT_ANSI__
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <volk.h>
// #include <vulkan/vulkan.h>
#include <vk_enum_string_helper.h> //idk why but it is neither shipped with Linux Vulkan SDK nor bundled in vulkan-sdk-components
#include <vk_mem_alloc.h>
#include <GLFW/glfw3.h>

using std::vector;
using std::cout;
using std::tuple;
using std::tie;
// using std::set;
using glm::u8, glm::u16, glm::u16, glm::u32;
using glm::i8, glm::i16, glm::i32;
using glm::f32, glm::f64;
using glm::defaultp;
using glm::quat;
using glm::ivec2,glm::ivec3,glm::ivec4;
using glm::i8vec2,glm::i8vec3,glm::i8vec4;
using glm::i16vec2,glm::i16vec3,glm::i16vec4;
using glm::uvec2,glm::uvec3,glm::uvec4;
using glm::u8vec2,glm::u8vec3,glm::u8vec4;
using glm::u16vec2,glm::u16vec3,glm::u16vec4;
using glm::vec,glm::vec2,glm::vec3,glm::vec4;
using glm::dvec2,glm::dvec3,glm::dvec4;
using glm::mat, glm::mat2, glm::mat3, glm::mat4;
using glm::dmat2, glm::dmat3, glm::dmat4;

const int MAX_FRAMES_IN_FLIGHT = 2;

typedef struct Buffer {
    VkBuffer buffer;
    VmaAllocation alloc;
    bool is_mapped = false;
} Buffer;

typedef struct Image {
    VkImage image;
    VkImageView view;
    VmaAllocation alloc;
    VkFormat format;
    // VkImageLayout layout; everything is in GENERAL.
    VkImageAspectFlags aspect;
    VkExtent3D extent;
} Image;

typedef struct IndexedVertices {
    vector<Buffer> indexes;
    u32 icount;
} IndexedVertices;

typedef struct NonIndexedVertices {
    vector<Buffer> vertices;
    u32 vcount;
} NonIndexedVertices;

typedef struct FaceBuffers {
    IndexedVertices Pzz, Nzz, zPz, zNz, zzP, zzN;
    vector<Buffer> vertexes;
} FaceBuffers;

typedef struct ImageDeletion {
    Image image;
    int life_counter;
} ImageDeletion;

typedef struct BufferDeletion {
    Buffer buffer;
    int life_counter;
} BufferDeletion;

//used for configuring raster pipeline
typedef struct AttrFormOffs {
    VkFormat format;
    uint32_t offset;
} AttrFormOffs;

enum BlendAttachment {
    NO_BLEND,
    BLEND_MIX,
    BLEND_SUB,
    BLEND_REPLACE_IF_GREATER, //basically max
    BLEND_REPLACE_IF_LESS, //basically max
};
enum DepthTesting {
    NO_DEPTH_TEST,
    FULL_DEPTH_TEST,
    READ_DEPTH_TEST,
    WRITE_DEPTH_TEST,
};
//it was not discard in fragment. FML
enum Discard {
    NO_DISCARD,
    DO_DISCARD,
};
enum LoadStoreOp {
    DontCare,
    Clear,
    Store,
    Load,
};
const VkStencilOpState NO_STENCIL = {};

typedef struct ShaderStage {
    const char* src;
    VkShaderStageFlagBits stage;
} ShaderStage;
typedef struct RasterPipe {
    VkPipeline line;
    VkPipelineLayout lineLayout;

    vector<VkDescriptorSet> sets;
    VkDescriptorSetLayout setLayout;

    VkRenderPass renderPass; //we dont really need to store it in here but why not
    i32 subpassId;
} RasterPipe;

typedef struct ComputePipe {
    VkPipeline line;
    VkPipelineLayout lineLayout;

    vector<VkDescriptorSet> sets;
    VkDescriptorSetLayout setLayout;
} ComputePipe;

typedef struct AttachmentDescription {
    Image* image;
    LoadStoreOp load,store, sload, sstore;
    VkImageLayout finalLayout = VK_IMAGE_LAYOUT_GENERAL;
}AttachmentDescription;
typedef struct SubpassAttachments {
    vector<RasterPipe*> pipes;
    vector<Image*> a_input;
    vector<Image*> a_color;
    Image* a_depth;
}SubpassAttachments;
typedef struct SubpassAttachmentRefs {
    vector<VkAttachmentReference> a_input;
    vector<VkAttachmentReference> a_color;
    VkAttachmentReference a_depth;
}SubpassAttachmentRefs;

//problem with such abstractions in vulkan is that they almost always have to be extended to a point where they make no sense
//pipeline with extra info for subpass creation
typedef struct subPass {
    VkPipeline pipe;
    VkPipelineLayout pipeLayout;

    VkDescriptorSetLayout dsetLayout;
    vector<VkDescriptorSet> descriptors;

    VkRenderPass rpass; //we dont really need to store it in here but why not
} subPass;

typedef struct Window {
    GLFWwindow* pointer;
    int width;
    int height;
} Window;

typedef struct QueueFamilyIndices {
    std::optional<u32> graphicalAndCompute;
    std::optional<u32> present;

  public:
    bool isComplete() {
        return graphicalAndCompute.has_value() && present.has_value();
    }
} QueueFamilyIndices;

typedef struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    vector<VkSurfaceFormatKHR> formats;
    vector<VkPresentModeKHR> presentModes;

  public:
    bool is_Suitable() {
        return (not formats.empty()) and (not presentModes.empty());
    }
} SwapChainSupportDetails;

enum RelativeDescriptorPos {
    RD_NONE, //what?
    RD_PREVIOUS, //Relatove Descriptor position previous - for accumulators
    RD_CURRENT, //Relatove Descriptor position matching - common cpu-paired
    RD_FIRST, //Relatove Descriptor position first    - for gpu-only
};
#define NO_SAMPLER ((VkSampler)(0))
#define NO_LAYOUT ((VkImageLayout)(0))

typedef struct DescriptorInfo {
    VkDescriptorType type;
    RelativeDescriptorPos relativePos;
    vector<Buffer> buffers;
    vector<Image> images;
    VkSampler imageSampler;
    VkImageLayout imageLayout; //ones that will be in use, not current
    VkShaderStageFlags stages;
} DescriptorInfo;
typedef struct DelayedDescriptorSetup {
    VkDescriptorSetLayout* setLayout;
    vector<VkDescriptorSet>* sets;
    vector<DescriptorInfo> descriptions;
    VkShaderStageFlags stages;
    VkDescriptorSetLayoutCreateFlags createFlags;
} DelayedDescriptorSetup;

typedef struct Settings {
    int timestampCount = 1024;
    bool vsync = false;
    bool fullscreen = false;
    bool debug = false;
    bool profile = false;
} Settings;

#include <unordered_map>
#include <functional>

//hash for VkSamplerCreateInfo
struct VkSamplerCreateInfoHash {
    size_t operator()(const VkSamplerCreateInfo& k) const {
        size_t res = 17;
        res = res * 31 + std::hash<VkStructureType>()(k.sType);
        res = res * 31 + std::hash<VkFilter>()(k.magFilter);
        res = res * 31 + std::hash<VkFilter>()(k.minFilter);
        res = res * 31 + std::hash<VkSamplerAddressMode>()(k.addressModeU);
        res = res * 31 + std::hash<VkSamplerAddressMode>()(k.addressModeV);
        res = res * 31 + std::hash<VkSamplerAddressMode>()(k.addressModeW);
        res = res * 31 + std::hash<VkBool32>()(k.anisotropyEnable);
        res = res * 31 + std::hash<VkBorderColor>()(k.borderColor);
        res = res * 31 + std::hash<VkBool32>()(k.unnormalizedCoordinates);
        res = res * 31 + std::hash<VkBool32>()(k.compareEnable);
        res = res * 31 + std::hash<VkCompareOp>()(k.compareOp);
        res = res * 31 + std::hash<VkSamplerMipmapMode>()(k.mipmapMode);
        res = res * 31 + std::hash<float>()(k.mipLodBias);
        res = res * 31 + std::hash<float>()(k.minLod);
        res = res * 31 + std::hash<float>()(k.maxLod);
        return res;
    }
};

struct VkSamplerCreateInfoEqual {
    bool operator()(const VkSamplerCreateInfo& lhs, const VkSamplerCreateInfo& rhs) const {
        return lhs.sType == rhs.sType &&
               lhs.magFilter == rhs.magFilter &&
               lhs.minFilter == rhs.minFilter &&
               lhs.addressModeU == rhs.addressModeU &&
               lhs.addressModeV == rhs.addressModeV &&
               lhs.addressModeW == rhs.addressModeW &&
               lhs.anisotropyEnable == rhs.anisotropyEnable &&
               lhs.borderColor == rhs.borderColor &&
               lhs.unnormalizedCoordinates == rhs.unnormalizedCoordinates &&
               lhs.compareEnable == rhs.compareEnable &&
               lhs.compareOp == rhs.compareOp &&
               lhs.mipmapMode == rhs.mipmapMode &&
               lhs.mipLodBias == rhs.mipLodBias &&
               lhs.minLod == rhs.minLod &&
               lhs.maxLod == rhs.maxLod;
    }
};

// Define the unordered_map

struct Renderer {
public:
    void init (Settings settings);
    void cleanup();

    double deltaTime = 0;
    bool resized = false;
    Settings settings = {};

    void start_frame();
        void present();
    void end_frame();


    void cmdBindPipe (VkCommandBuffer commandBuffer, RasterPipe pipe);
    void cmdBindPipe (VkCommandBuffer commandBuffer, ComputePipe pipe);

    void cmdPipelineBarrier (VkCommandBuffer commandBuffer,
                             VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                             VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                             Buffer buffer);
    void cmdPipelineBarrier (VkCommandBuffer commandBuffer,
                             VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                             VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                             Image image);
    void cmdPipelineBarrier (VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
    void cmdExplicitTransLayoutBarrier (VkCommandBuffer commandBuffer, VkImageLayout srcLayout, VkImageLayout targetLayout,
                                VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                                Image* image);
    void cmdSetViewport(VkCommandBuffer commandBuffer, int width, int height);
    void cmdSetViewport(VkCommandBuffer commandBuffer, VkExtent2D extent);

    template<class Vertex_T> vector<Buffer> createElemBuffers (Vertex_T* vertices, u32 count, u32 buffer_usage = 0);
    template<class Vertex_T> vector<Buffer> createElemBuffers (vector<Vertex_T> vertices, u32 buffer_usage = 0);

    void createAllocator();
    void createWindow();
    void createInstance();
    void setupDebugMessenger();
    void debugSetName();
    void createSurface();
    SwapChainSupportDetails querySwapchainSupport (VkPhysicalDevice);
    QueueFamilyIndices findQueueFamilies (VkPhysicalDevice);
    bool checkFormatSupport (VkPhysicalDevice device, VkFormat format, VkFormatFeatureFlags features);
    bool isPhysicalDeviceSuitable (VkPhysicalDevice);
    bool checkPhysicalDeviceExtensionSupport (VkPhysicalDevice);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat (vector<VkSurfaceFormatKHR> availableFormats);
    VkPresentModeKHR chooseSwapPresentMode (vector<VkPresentModeKHR> availablePresentModes);
    VkExtent2D chooseSwapExtent (VkSurfaceCapabilitiesKHR capabilities);
    void createSwapchainImageViews();
    VkAttachmentLoadOp  getOpLoad (LoadStoreOp op);
    VkAttachmentStoreOp getOpStore(LoadStoreOp op);
    void createRenderPass(vector<AttachmentDescription> attachments, vector<SubpassAttachments> subpasses, VkRenderPass* rpass);

    VkFormat findSupportedFormat (vector<VkFormat> candidates, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage);

    void createRasterPipeline (RasterPipe* pipe, VkDescriptorSetLayout extra_dynamic_layout, vector<ShaderStage> shader_stages, vector<AttrFormOffs> attr_desc,
                                 u32 stride, VkVertexInputRate input_rate, VkPrimitiveTopology topology,
                                 VkExtent2D extent, vector<BlendAttachment> blends, u32 push_size, DepthTesting depthTest, VkCullModeFlags culling, Discard discard, VkStencilOpState stencil);
    void destroyRasterPipeline (RasterPipe* pipe);

    void createComputePipeline (ComputePipe* pipe, VkDescriptorSetLayout extra_dynamic_layout, const char* src, u32 push_size, VkPipelineCreateFlags create_flags);
    void destroyComputePipeline (ComputePipe* pipe);

    void createDescriptorPool();
    void deferDescriptorSetup (VkDescriptorSetLayout* dsetLayout, vector<VkDescriptorSet>* descriptors, vector<DescriptorInfo> description, VkShaderStageFlags stages, VkDescriptorSetLayoutCreateFlags createFlags = 0);
    void setupDescriptor (VkDescriptorSetLayout* dsetLayout, vector<VkDescriptorSet>* descriptors, vector<DescriptorInfo> description, VkShaderStageFlags stages);
    vector<DelayedDescriptorSetup> delayed_descriptor_setups;
    void flushDescriptorSetup();

    void createSamplers();

    void deleteImages (vector<Image>* images);
    void deleteImages (Image* images);
    void deleteBuffers (vector<Buffer>* buffers);
    void deleteBuffers (Buffer* buffers);
    void createImageStorages (vector<Image>* images,
                                VkImageType type, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage vma_usage, VmaAllocationCreateFlags vma_flags, VkImageAspectFlags aspect,
                                uvec3 size, int mipmaps = 1, VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT);
    void createImageStorages (Image* image,
                                VkImageType type, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage vma_usage, VmaAllocationCreateFlags vma_flags, VkImageAspectFlags aspect,
                                uvec3 size, int mipmaps = 1, VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT);
    void createImageStoragesDepthStencil (vector<VkImage>* images, vector<VmaAllocation>* allocs, vector<VkImageView>* depthViews, vector<VkImageView>* stencilViews,
            VkImageType type, VkFormat format, VkImageUsageFlags usage, VmaMemoryUsage vma_usage, VmaAllocationCreateFlags vma_flags, VkImageAspectFlags aspect, VkImageLayout layout, VkPipelineStageFlags pipeStage, VkAccessFlags access,
            uvec3 size);
    void generateMipmaps (VkCommandBuffer commandBuffer, VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, VkImageAspectFlags aspect);
    void createBufferStorages (vector<Buffer>* buffers, VkBufferUsageFlags usage, u32 size, bool host = false);
    void createBufferStorages (Buffer* buffer, VkBufferUsageFlags usage, u32 size, bool host = false);
    VkShaderModule createShaderModule (vector<char>* code);
    void createFramebuffers (vector<VkFramebuffer>* framebuffers, vector<vector<Image>> imgs4views, VkRenderPass renderPass, u32 Width, u32 Height);
    void createFramebuffers (vector<VkFramebuffer>* framebuffers, vector<vector<Image>> imgs4views, VkRenderPass renderPass, VkExtent2D extent);
    void createCommandPool();
    void createCommandBuffers (vector<VkCommandBuffer>* commandBuffers, u32 size);
    void createSyncObjects();

    void createBuffer (VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* bufferMemory);
    void copyBufferSingleTime (VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferSingleTime (VkBuffer srcBuffer, Image* image, uvec3 size);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands (VkCommandBuffer commandBuffer);
    void transitionImageLayoutSingletime (Image* image, VkImageLayout newLayout, int mipmaps = 1);
    void copyImage (VkExtent3D extent, VkCommandBuffer cmdbuf, Image src, Image dst);
    void copyWholeImage (VkCommandBuffer cmdbuf, Image src, Image dst);
    void blitWholeImage (VkCommandBuffer cmdbuf, Image src, Image dst, VkFilter filter);
    void processDeletionQueues();
    void getInstanceExtensions();

    u32 STORAGE_BUFFER_DESCRIPTOR_COUNT = 0;
    u32 STORAGE_IMAGE_DESCRIPTOR_COUNT = 0;
    u32 COMBINED_IMAGE_SAMPLER_DESCRIPTOR_COUNT = 0;
    u32 UNIFORM_BUFFER_DESCRIPTOR_COUNT = 0;
    u32 INPUT_ATTACHMENT_DESCRIPTOR_COUNT = 0;
    u32 descriptor_sets_count = 0;
    void countDescriptor (const VkDescriptorType type);
    void createDescriptorSetLayout (vector<VkDescriptorType> descriptorTypes, VkShaderStageFlags baseStages, VkDescriptorSetLayout* layout, VkDescriptorSetLayoutCreateFlags flags = 0);
    void createDescriptorSetLayout (vector<VkDescriptorType> descriptorTypes, vector<VkShaderStageFlags> stages, VkDescriptorSetLayout* layout, VkDescriptorSetLayoutCreateFlags flags = 0);

    Window window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkSurfaceKHR surface;
    QueueFamilyIndices familyIndices;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkCommandPool commandPool;
    VkSwapchainKHR swapchain;

    VkPhysicalDeviceProperties physicalDeviceProperties;

    VkDescriptorPool descriptorPool;
    VmaAllocator VMAllocator;

    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    vector<Image> swapchainImages;
    u32 imageIndex = 0;

    vector<VkCommandBuffer> graphicsCommandBuffers;
    vector<VkCommandBuffer> copyCommandBuffers; //runtime copies. Also does first frame resources

    vector<VkSemaphore> imageAvailableSemaphores;
    vector<VkSemaphore> renderFinishedSemaphores; //to sync renering with presenting
    vector<VkFence> frameInFlightFences;

    //holds all the cpu-side VkSamplers 
    std::unordered_map<VkSamplerCreateInfo, VkSampler, VkSamplerCreateInfoHash, VkSamplerCreateInfoEqual> samplerMap;

    vector<ImageDeletion> imageDeletionQueue; //cpu side  image abstractions deletion queue. Exists for delayed copies
    vector<BufferDeletion> bufferDeletionQueue; //cpu side buffer abstractions deletion queue. Exists for delayed copies

    u32 currentFrame = 0;
    u32 previousFrame = MAX_FRAMES_IN_FLIGHT - 1;
    u32 nextFrame = 1;

    int iFrame = 0;
    VkDebugUtilsMessengerEXT debugMessenger; //just keep it there, who cares about 
    vector<VkQueryPool> queryPoolTimestamps;
    int currentTimestamp = 0;
    int timestampCount = 0;
    vector<uint64_t>        timestamps = {};
    vector<double>         ftimestamps = {};
    vector<double> average_ftimestamps = {};
    vector<const char*> timestampNames = {};
};