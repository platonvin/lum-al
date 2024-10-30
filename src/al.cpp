#include <numeric>
#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#include "al.hpp"
#include "defines/macros.hpp" //after al.hpp
#include "ringbuffer.hpp"
#include "defines/cache.hpp"

using std::vector;

using glm::u8, glm::u16, glm::u16, glm::u32;
using glm::i8, glm::i16, glm::i32;
using glm::f32, glm::f64;
using glm::quat;
using glm::ivec2,glm::ivec3,glm::ivec4;
using glm::i8vec2,glm::i8vec3,glm::i8vec4;
using glm::i16vec2,glm::i16vec3,glm::i16vec4;
using glm::uvec2,glm::uvec3,glm::uvec4;
using glm::u8vec2,glm::u8vec3,glm::u8vec4;
using glm::u16vec2,glm::u16vec3,glm::u16vec4;
using glm::vec,glm::vec2,glm::vec3,glm::vec4;
using glm::dvec2,glm::dvec3,glm::dvec4;
using glm::mat2, glm::mat3, glm::mat4;
using glm::dmat2, glm::dmat3, glm::dmat4;

tuple<int, int> get_block_xy (int N);

void Renderer::init (Settings settings) {
    this->settings = settings;
    timestampCount = settings.timestampCount;

    createWindow();
    VK_CHECK (volkInitialize());
    getInstanceLayers();
    getInstanceExtensions();
    createInstance();
    volkLoadInstance (instance);
    if(settings.debug) setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createAllocator();
    DEBUG_LOG(VMAllocator);
    createSwapchain(); //should be before anything related to its size and format
    createSwapchainImageViews();
    createCommandPool();
    createSyncObjects();

    if(settings.profile){
        assert(timestampCount > 0);
        timestampCount = settings.timestampCount; //TODO dynamic
        timestampNames.resize (timestampCount);
        timestamps.resize (timestampCount);
        ftimestamps.resize (timestampCount);
        average_ftimestamps.resize (timestampCount);
        VkQueryPoolCreateInfo query_pool_info{};
            query_pool_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            query_pool_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
            query_pool_info.queryCount = timestampCount;
            queryPoolTimestamps.allocate (settings.fif);
        for (auto &q : queryPoolTimestamps) {
            VK_CHECK (vkCreateQueryPool (device, &query_pool_info, NULL, &q));
            assert(q != VK_NULL_HANDLE);
        }
    }
}

void Renderer::destroyImages (ring<Image>* images) {
    for (auto& img : (*images)) {
        destroyImages(&img);
    }
    *images = {};
}
void Renderer::destroyImages (Image* image) {
    // "just" .view and .mip_views[0] are not the same
    // from 0 to .size() is important
    for(int i=0; i < (*image).mip_views.size(); i++){
        vkDestroyImageView (device, (*image).mip_views[i], NULL);
    }
    vkDestroyImageView (device, (*image).view, NULL);
    vmaDestroyImage (VMAllocator, (*image).image, (*image).alloc);
    *image = {};
}

void Renderer::destroyBuffers (ring<Buffer>* buffers) {
    for (auto& buf : (*buffers)) {
        destroyBuffers(&buf);
    }
    *buffers = {};
}
void Renderer::destroyBuffers (Buffer* buffer) {
    if((*buffer).mapped != NULL){
        vmaUnmapMemory (VMAllocator, (*buffer).alloc);
    }
    vmaDestroyBuffer (VMAllocator, (*buffer).buffer, (*buffer).alloc);
    *buffer = {};
}

void Renderer::cleanup() {
    vkDeviceWaitIdle(device);
TRACE();
    
    for (auto [info, sampler]: samplerMap) {
        vkDestroySampler(device, sampler, NULL);
    }
TRACE();

    vkDestroyDescriptorPool (device, descriptorPool, NULL);
    for (int i = 0; i < settings.fif; i++) {
        vkDestroySemaphore (device, imageAvailableSemaphores[i], NULL);
        vkDestroySemaphore (device, renderFinishedSemaphores[i], NULL);
        vkDestroyFence (device, frameInFlightFences[i], NULL);
        if(settings.profile){
            vkDestroyQueryPool (device, queryPoolTimestamps[i], NULL);
        }
    }
    vkDestroyCommandPool (device, commandPool, NULL);
TRACE();

    for (auto img : swapchainImages) {
        vkDestroyImageView (device, img.view, NULL);
    }
    vkDestroySwapchainKHR (device, swapchain, NULL);
TRACE();

    for (int i = 0; i < settings.fif + 1; i++) {
        processDeletionQueues();
    } 
    assert((bufferDeletionQueue.size() == 0) && "buffer deletion lifetime was way to long. There is no reason to keep it higher than FIF"); 
    assert(( imageDeletionQueue.size() == 0) &&  "image deletion lifetime was way to long. There is no reason to keep it higher than FIF"); 
    vmaDestroyAllocator (VMAllocator); //do before destroyDevice
    vkDestroyDevice (device, NULL);
    vkDestroySurfaceKHR (instance, surface, NULL);
    if(settings.debug) vkDestroyDebugUtilsMessengerEXT (instance, debugMessenger, NULL);
    vkDestroyInstance (instance, NULL);
    glfwDestroyWindow (window.pointer);
}

void Renderer::createSwapchain() {
    SwapChainSupportDetails swapChainSupport = querySwapchainSupport (physicalDevice);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat (swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode (swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent (swapChainSupport.capabilities);
    u32 imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    QueueFamilyIndices indices = findQueueFamilies (physicalDevice);
    u32 queueFamilyIndices[] = {indices.graphicalAndCompute.value(), indices.present.value()};
    if (indices.graphicalAndCompute != indices.present) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;
    VK_CHECK (vkCreateSwapchainKHR (device, &createInfo, NULL, &swapchain));
    vkGetSwapchainImagesKHR (device, swapchain, &imageCount, NULL);
    swapchainImages.allocate (imageCount);
    ring<VkImage> imgs (imageCount);
    vkGetSwapchainImagesKHR (device, swapchain, &imageCount, imgs.data());
    for (int i = 0; i < imageCount; i++) {
        assert(imgs[i] != VK_NULL_HANDLE);
        swapchainImages[i].image = imgs[i];
    }
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void Renderer::recreateSwapchain(){
    int width = 0, height = 0;
TRACE();
    glfwGetFramebufferSize(window.pointer, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window.pointer, &width, &height);
        glfwWaitEvents();
    }

TRACE();
    deviceWaitIdle();
    
    if(cleanupSwapchainDependent) cleanupSwapchainDependent();
    vkDestroyCommandPool(device, commandPool, NULL);
    vkDestroyDescriptorPool(device, descriptorPool, NULL);
    resetDescriptorSetup();
    deviceWaitIdle();

TRACE();
    for (auto img : swapchainImages) {
        vkDestroyImageView (device, img.view, NULL);
    }
    vkDestroySwapchainKHR (device, swapchain, NULL);
    // vkDestroyCommandPool(device, commandPool, NULL);    
TRACE();
    createSwapchain();
    createSwapchainImageViews();
    createCommandPool();
    // called in flushDescriptors
    // createDescriptorPool();
TRACE();

    if(createSwapchainDependent) createSwapchainDependent();
}

void Renderer::createSwapchainImageViews() {
    for (i32 i = 0; i < swapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            createInfo.image = swapchainImages[i].image;
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
        swapchainImages[i].aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        swapchainImages[i].format = swapChainImageFormat;
        swapchainImages[i].extent.width = swapChainExtent.width;
        swapchainImages[i].extent.height = swapChainExtent.height;
        swapchainImages[i].extent.depth = 1;
        VK_CHECK (vkCreateImageView (device, &createInfo, NULL, &swapchainImages[i].view));
    }
}

void Renderer::createFramebuffers (ring<VkFramebuffer>* framebuffers, vector<ring<Image>*> imgs4views, VkRenderPass renderPass, u32 Width, u32 Height) {
    //we want to iterate over ring, so we need Least Common Multiple of ring's sizes 
    //for example: 1 depth buffer, 3 swapchain image, 2 taa images result in 6 framebuffers
    int lcm = 1;
    for (auto imgs : imgs4views){
        lcm = std::lcm((*imgs).size(), lcm);
    }
    DEBUG_LOG(lcm)
TRACE();
    
    (*framebuffers).allocate (lcm);
    for (u32 i = 0; i < lcm; i++) {
        vector<VkImageView> attachments = {};
        for (auto imgs : imgs4views) {
            DEBUG_LOG((*imgs).size())
            int internal_iter = i % (*imgs).size();
            attachments.push_back ((*imgs)[internal_iter].view);
            DEBUG_LOG((*imgs)[internal_iter].view)
        }
        VkFramebufferCreateInfo 
            framebufferInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = attachments.size();
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = Width;
            framebufferInfo.height = Height;
            framebufferInfo.layers = 1;
        VK_CHECK (vkCreateFramebuffer (device, &framebufferInfo, NULL, & (*framebuffers)[i]));
    }
}
void Renderer::createFramebuffers (ring<VkFramebuffer>* framebuffers, vector<ring<Image>*> imgs4views, VkRenderPass renderPass, VkExtent2D extent) {
    createFramebuffers(framebuffers, imgs4views, renderPass, extent.width, extent.height);
}
void Renderer::processDeletionQueues() {
    int write_index = 0;
    write_index = 0;
    for (int i = 0; i < imageDeletionQueue.size(); i++) {
        bool should_keep = imageDeletionQueue[i].life_counter > 0;
        if (should_keep) {
            imageDeletionQueue[write_index] = imageDeletionQueue[i];
            imageDeletionQueue[write_index].life_counter -= 1;
            write_index++;
        } else {
            vmaDestroyImage (VMAllocator, imageDeletionQueue[i].image.image, imageDeletionQueue[i].image.alloc);
            vkDestroyImageView (device, imageDeletionQueue[i].image.view, NULL);
        }
    }
    imageDeletionQueue.resize (write_index);
    write_index = 0;
    for (int i = 0; i < bufferDeletionQueue.size(); i++) {
        bool should_keep = bufferDeletionQueue[i].life_counter > 0;
        if (should_keep) {
            bufferDeletionQueue[write_index] = bufferDeletionQueue[i];
            bufferDeletionQueue[write_index].life_counter -= 1;
            write_index++;
        } else {
            if (bufferDeletionQueue[i].buffer.mapped != NULL) {
                vmaUnmapMemory (VMAllocator, bufferDeletionQueue[i].buffer.alloc);
            }
            vmaDestroyBuffer (VMAllocator, bufferDeletionQueue[i].buffer.buffer, bufferDeletionQueue[i].buffer.alloc);
        }
    }
    bufferDeletionQueue.resize (write_index);
}


// #include <glm/gtx/string_cast.hpp>
void Renderer::start_frame(vector<VkCommandBuffer> commandBuffers) {
    TRACE();
    vkWaitForFences (device, 1, &frameInFlightFences.current(), VK_TRUE, UINT32_MAX);
    TRACE();
    vkResetFences (device, 1, &frameInFlightFences.current());

    VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = NULL;
    TRACE();
    for (auto cb : commandBuffers){
        assert(cb != VK_NULL_HANDLE);
        vkResetCommandBuffer(cb, 0);
    TRACE();
        vkBeginCommandBuffer(cb, &beginInfo);
    }
    TRACE();

    VkResult result = vkAcquireNextImageKHR (device, swapchain, UINT64_MAX, imageAvailableSemaphores.current(), VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // recreateSwapchain();
        resized = true;
        // return; // can be avoided, but it is just 1 frame
    } else if ((result != VK_SUCCESS)) {
        printf (KRED "failed to acquire swap chain image!\n" KEND);
        exit (result);
    }
    TRACE();

    if(settings.profile){
        assert(mainCommandBuffers->current() != VK_NULL_HANDLE);
        assert(queryPoolTimestamps.current() != VK_NULL_HANDLE);
        // if(iFrame<10){
        //     for(auto& q: queryPoolTimestamps)
        //         vkCmdResetQueryPool ((*mainCommandBuffers).current(), q, 0, timestampCount);
        // } else 
        vkCmdResetQueryPool ((*mainCommandBuffers).current(), queryPoolTimestamps.current(), 0, timestampCount);

        currentTimestamp = 0;
    }
}

void Renderer::present() {
    vector<VkSemaphore> waitSemaphores = {renderFinishedSemaphores.current()};
    
    VkSwapchainKHR swapchains[] = {swapchain};
    VkPresentInfoKHR 
        presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = waitSemaphores.size();
        presentInfo.pWaitSemaphores = waitSemaphores.data();
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = NULL;
    VkResult result = vkQueuePresentKHR (presentQueue, &presentInfo);

    //why here? To simplify things
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || resized) {
        resized = false;
        recreateSwapchain();
        // cout << KRED"failed to present swap chain image!\n" KEND;
    } else if (result != VK_SUCCESS) {
        cout << KRED"failed to present swap chain image!\n" KEND;
        exit (result);
    }
}

void Renderer::end_frame(vector<VkCommandBuffer> commandBuffers) {
    for(auto cb : commandBuffers){
        vkEndCommandBuffer(cb);
    }
    
    vector<VkSemaphore> signalSemaphores = {renderFinishedSemaphores.current()};
    vector<VkSemaphore> waitSemaphores = {imageAvailableSemaphores.current()};

    vector<VkPipelineStageFlags> waitStages (commandBuffers.size(), VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    VkSubmitInfo 
        submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = waitSemaphores.size();
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.data();
        submitInfo.commandBufferCount = commandBuffers.size();
        submitInfo.pCommandBuffers = commandBuffers.data();
        submitInfo.signalSemaphoreCount = signalSemaphores.size();
        submitInfo.pSignalSemaphores = signalSemaphores.data();
    VK_CHECK (vkQueueSubmit (graphicsQueue, 1, &submitInfo, frameInFlightFences.current()));

    present();

    if(settings.profile){
        if (iFrame > 0) {
                vkGetQueryPoolResults (
                    device,
                    //this has to wait for cmdbuff to finish executing on gpu, so to keep concurrency we use previous frame measurement. For 1 FIF its still current one
                    queryPoolTimestamps.previous(), 
                    0,
                    currentTimestamp,
                    currentTimestamp* sizeof (uint64_t),
                    timestamps.data(),
                    sizeof (uint64_t),
                    VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
            }
        double timestampPeriod = physicalDeviceProperties.limits.timestampPeriod;
        for (int i = 0; i < timestampCount; i++) {
            ftimestamps[i] = double (timestamps[i]) * timestampPeriod / 1000000.0;
        }
        //for less fluctuation
        if (iFrame > 5) {
            for (int i = 0; i < timestampCount; i++) {
                average_ftimestamps[i] = glm::mix (average_ftimestamps[i], ftimestamps[i], 1.0);
            }
        } else {
            for (int i = 0; i < timestampCount; i++) {
                average_ftimestamps[i] = ftimestamps[i];
            }
        }
    }
    currentFrame = (currentFrame + 1) % settings.fif;
    iFrame++;
    
    imageAvailableSemaphores.move(); //to sync presenting with renering  
    renderFinishedSemaphores.move(); //to sync renering with presenting
    frameInFlightFences.move();
    if(settings.profile){
        queryPoolTimestamps.move();
    }

    processDeletionQueues();
}

void Renderer::cmdPipelineBarrier (VkCommandBuffer commandBuffer,
                VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                Buffer buffer) {
    VkBufferMemoryBarrier 
        barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.srcAccessMask = srcAccessMask;
        barrier.dstAccessMask = dstAccessMask;
        barrier.size = VK_WHOLE_SIZE;
        barrier.buffer = buffer.buffer; // buffer buffer buffer
    vkCmdPipelineBarrier (
        commandBuffer,
        srcStageMask, dstStageMask,
        0,
        0, NULL,
        1, &barrier,
        0, NULL
    );
}

void Renderer::cmdPipelineBarrier (VkCommandBuffer commandBuffer,
                VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                Image image) {
    VkImageMemoryBarrier 
        barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = (image).image;
        barrier.subresourceRange.aspectMask = (image).aspect;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = srcAccessMask;
        barrier.dstAccessMask = dstAccessMask;
    vkCmdPipelineBarrier (
        commandBuffer,
        srcStageMask, dstStageMask,
        0,
        0, NULL,
        0, NULL,
        1, &barrier
    );
}

void Renderer::cmdPipelineBarrier (VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask) {
    vkCmdPipelineBarrier (
        commandBuffer,
        srcStageMask, dstStageMask,
        0,
        0, NULL,
        0, NULL,
        0, NULL
    );
}

//used for image creation
void Renderer::cmdExplicitTransLayoutBarrier (VkCommandBuffer commandBuffer, VkImageLayout srcLayout, VkImageLayout targetLayout,
                    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                    Image* image) {
    VkImageMemoryBarrier 
        barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = srcLayout;
        barrier.newLayout = targetLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = (*image).image;
        barrier.subresourceRange.aspectMask = (*image).aspect;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = srcAccessMask;
        barrier.dstAccessMask = dstAccessMask;
    vkCmdPipelineBarrier (
        commandBuffer,
        srcStageMask, dstStageMask,
        0,
        0, NULL,
        0, NULL,
        1, &barrier
    );
}

void Renderer::createSampler(VkSampler* sampler, VkFilter mag, VkFilter min, 
    VkSamplerAddressMode U, VkSamplerAddressMode V, VkSamplerAddressMode W) {
    VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = mag;
		samplerInfo.minFilter = min;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = U;
		samplerInfo.addressModeV = V;
		samplerInfo.addressModeW = W;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		samplerInfo.maxAnisotropy = 1.0;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
		samplerInfo.maxAnisotropy = 8.0f; //safe i guess
		samplerInfo.anisotropyEnable = VK_TRUE;
		VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, sampler));
}

void Renderer::destroySampler(VkSampler sampler){
    vkDestroySampler(device, sampler, NULL);
    sampler = VK_NULL_HANDLE;
}
void Renderer::createImageFromMemorySingleTime(Image* image, const void* source, u32 bufferSize){
    assert (bufferSize != 0);
    VkBufferCreateInfo 
        stagingBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        stagingBufferInfo.size = bufferSize;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VmaAllocationCreateInfo 
        stagingAllocInfo = {};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        stagingAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VkBuffer stagingBuffer = {};
    VmaAllocation stagingAllocation = {};

    VK_CHECK (vmaCreateBuffer (VMAllocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, NULL));
    void* data = NULL;
    VK_CHECK (vmaMapMemory (VMAllocator, stagingAllocation, &data));
    memcpy (data, source, bufferSize);
    vmaUnmapMemory (VMAllocator, stagingAllocation);

    VkCommandBuffer copyCommandBuffer = beginSingleTimeCommands();
    cmdExplicitTransLayoutBarrier (copyCommandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   VK_ACCESS_NONE, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
                                   image);
    VkBufferImageCopy staging_copy = {};
        staging_copy.imageExtent = image->extent;
        staging_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        staging_copy.imageSubresource.baseArrayLayer = 0;
        staging_copy.imageSubresource.mipLevel = 0;
        staging_copy.imageSubresource.layerCount = 1;
    vkCmdCopyBufferToImage (copyCommandBuffer, stagingBuffer, image->image, VK_IMAGE_LAYOUT_GENERAL, 1, &staging_copy);
    cmdExplicitTransLayoutBarrier (copyCommandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                   VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
                                   image);
    vmaDestroyBuffer(VMAllocator, stagingBuffer, stagingAllocation);
    endSingleTimeCommands(copyCommandBuffer);
}
//slow.
void Renderer::copyBufferSingleTime (VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer (commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    endSingleTimeCommands (commandBuffer);
}
void Renderer::copyBufferSingleTime (VkBuffer srcBuffer, Image* image, uvec3 size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    VkBufferImageCopy 
        copyRegion = {};
        copyRegion.imageExtent.width = size.x;
        copyRegion.imageExtent.height = size.y;
        copyRegion.imageExtent.depth = size.z;
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.layerCount = 1;
    vkCmdCopyBufferToImage (commandBuffer, srcBuffer, (*image).image, VK_IMAGE_LAYOUT_GENERAL, 1, &copyRegion);
    endSingleTimeCommands (commandBuffer);
}

VkCommandBuffer Renderer::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo 
        allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers (device, &allocInfo, &commandBuffer);
    VkCommandBufferBeginInfo 
        beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer (commandBuffer, &beginInfo);
    return commandBuffer;
}

void Renderer::endSingleTimeCommands (VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer (commandBuffer);
    VkSubmitInfo 
        submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
    //TODO: change to barrier
    vkQueueSubmit (graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle (graphicsQueue);
    vkFreeCommandBuffers (device, commandPool, 1, &commandBuffer);
}

void Renderer::generateMipmaps (VkCommandBuffer commandBuffer, VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, VkImageAspectFlags aspect) {
    // VkCommandBuffer commandBuffer = begin_Single_Time_Commands();
    VkImageMemoryBarrier 
        barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = aspect;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;
    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;
    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier (commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);
        VkImageBlit 
            blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = aspect;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = aspect;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;
        vkCmdBlitImage (commandBuffer,
            image, VK_IMAGE_LAYOUT_GENERAL,
            image, VK_IMAGE_LAYOUT_GENERAL,
            1, &blit,
            (aspect == VK_IMAGE_ASPECT_COLOR_BIT) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST);
                barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier (commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);
        if (mipWidth > 1) { mipWidth /= 2; }
        if (mipHeight > 1) { mipHeight /= 2; }
    }
        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier (commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
    // end_Single_Time_Commands(commandBuffer);
}

void Renderer::copyWholeImage (VkCommandBuffer cmdbuf, Image src, Image dst) {
    VkImageCopy 
        copy_op = {};
        copy_op.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_op.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_op.srcSubresource.layerCount = 1;
        copy_op.dstSubresource.layerCount = 1;
        copy_op.srcSubresource.baseArrayLayer = 0;
        copy_op.dstSubresource.baseArrayLayer = 0;
        copy_op.srcSubresource.mipLevel = 0;
        copy_op.dstSubresource.mipLevel = 0;
        copy_op.srcOffset = {0, 0, 0};
        copy_op.dstOffset = {0, 0, 0};
        copy_op.extent = src.extent;
    vkCmdCopyImage (cmdbuf,
                    src.image,
                    VK_IMAGE_LAYOUT_GENERAL, //TODO:
                    dst.image,
                    VK_IMAGE_LAYOUT_GENERAL, //TODO:
                    1,
                    &copy_op
                   );
    VkImageMemoryBarrier 
        barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = dst.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT ;
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    vkCmdPipelineBarrier (
        cmdbuf,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0, NULL,
        0, NULL,
        1, &barrier
    );
}

void Renderer::blitWholeImage (VkCommandBuffer cmdbuf, Image src, Image dst, VkFilter filter) {
    VkImageBlit 
        blit_op = {};
        blit_op.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit_op.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit_op.srcSubresource.layerCount = 1;
        blit_op.dstSubresource.layerCount = 1;
        blit_op.srcSubresource.baseArrayLayer = 0;
        blit_op.dstSubresource.baseArrayLayer = 0;
        blit_op.srcSubresource.mipLevel = 0;
        blit_op.dstSubresource.mipLevel = 0;
            blit_op.srcOffsets[0] = {0,0,0};
            blit_op.srcOffsets[1] = {int(src.extent.width), int(src.extent.height), int(src.extent.depth)};
            blit_op.dstOffsets[0] = {0,0,0};
            blit_op.dstOffsets[1] = {int(dst.extent.width), int(dst.extent.height), int(dst.extent.depth)};
    vkCmdBlitImage (cmdbuf,
                    src.image,
                    VK_IMAGE_LAYOUT_GENERAL, //TODO:
                    dst.image,
                    VK_IMAGE_LAYOUT_GENERAL, //TODO:
                    1,
                    &blit_op,
                    filter
                   );
    VkImageMemoryBarrier 
        barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = dst.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT ;
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    vkCmdPipelineBarrier (
        cmdbuf,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0, NULL,
        0, NULL,
        1, &barrier
    );
}

VkFormat Renderer::findSupportedFormat (vector<VkFormat> candidates,
                        VkImageType type,
                        VkImageTiling tiling,
                        VkImageUsageFlags usage) {
    for (VkFormat format : candidates) {
        VkImageFormatProperties imageFormatProperties;
        VkResult result = vkGetPhysicalDeviceImageFormatProperties (
            physicalDevice,
            format,
            type,
            tiling,
            usage,
            0, // No flags
      &     imageFormatProperties);
        if (result == VK_SUCCESS) {
            return format;
        }
    }
    crash (Failed to find supported format!);
}

void Renderer::cmdSetViewport(VkCommandBuffer commandBuffer, int width, int height){
    VkViewport 
        viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) (width );
        viewport.height = (float) (height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
    vkCmdSetViewport (commandBuffer, 0, 1, &viewport);
    VkRect2D 
        scissor = {};
        scissor.offset = {0, 0};
        scissor.extent.width = width;
        scissor.extent.height = height;
    vkCmdSetScissor (commandBuffer, 0, 1, &scissor);
}
void Renderer::cmdSetViewport(VkCommandBuffer commandBuffer, VkExtent2D extent){
    cmdSetViewport(commandBuffer, extent.width, extent.height);
}

void Renderer::cmdDraw(VkCommandBuffer commandBuffer, u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance){
    vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}
void Renderer::cmdDispatch(VkCommandBuffer commandBuffer, u32 groupCountX, u32 groupCountY, u32 groupCountZ){
    vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
}
void Renderer::cmdBeginRenderPass(VkCommandBuffer commandBuffer, RenderPass* rpass){
    VkRenderPassBeginInfo 
        renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = rpass->rpass;
        renderPassInfo.framebuffer = rpass->framebuffers.current();
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = rpass->extent;
        renderPassInfo.clearValueCount = rpass->clear_colors.size();
        renderPassInfo.pClearValues = rpass->clear_colors.data();
    vkCmdBeginRenderPass (commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    cmdSetViewport(commandBuffer, rpass->extent);
}
void Renderer::cmdNextSubpass(VkCommandBuffer commandBuffer, RenderPass* rpass){
    vkCmdNextSubpass (commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
}
void Renderer::cmdEndRenderPass(VkCommandBuffer commandBuffer, RenderPass* rpass){
    vkCmdEndRenderPass (commandBuffer);
    rpass->framebuffers.move();
}

void Renderer::cmdBindPipe(VkCommandBuffer commandBuffer, RasterPipe pipe){
    // PLACE_TIMESTAMP(commandBuffer);
    BIND_CACHED(pipe.line, vkCmdBindPipeline (commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.line));
    BIND_CACHED(pipe.lineLayout, vkCmdBindDescriptorSets (commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.lineLayout, 0, 1, &pipe.sets.current(), 0, 0));
}
void Renderer::cmdBindPipe(VkCommandBuffer commandBuffer, ComputePipe pipe){
    // PLACE_TIMESTAMP(commandBuffer);
    BIND_CACHED(pipe.line, vkCmdBindPipeline (commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipe.line));
    BIND_CACHED(pipe.lineLayout, vkCmdBindDescriptorSets (commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipe.lineLayout, 0, 1, &pipe.sets.current(), 0, 0));
}

void Renderer::transitionImageLayoutSingletime (Image* image, VkImageLayout newLayout, int mipmaps) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    VkImageMemoryBarrier 
        barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = (*image).image;
        barrier.subresourceRange.aspectMask = (*image).aspect;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipmaps;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    vkCmdPipelineBarrier (
        commandBuffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0, NULL,
        0, NULL,
        1, &barrier
    );
    endSingleTimeCommands (commandBuffer);
}