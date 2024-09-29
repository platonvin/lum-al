#include "../src/al.hpp"
#include "defines/macros.hpp"

//example fo simple triangle rendering with 2 subpasses - main shading and posteffect
//println is literally "printf __LINE__"

Renderer render = {};
RasterPipe simple_raster_pipe = {};
RasterPipe simple_posteffect_pipe = {};
RenderPass simple_rpass = {};
ring<Image> simple_inter_image; 
ring<VkCommandBuffer> mainCommandBuffers = {};
ring<VkCommandBuffer> extraCommandBuffers = {}; //runtime copies. Also does first frame resources

//this function will (also) be used after swapchain recreation to properly resize window
std::function<VkResult(void)> createSwapchainDependent = [](){
    simple_raster_pipe.sets = {};
    simple_raster_pipe.setLayout = 0;
    simple_posteffect_pipe.sets = {};
    simple_posteffect_pipe.setLayout = 0;
    simple_rpass = {};
    simple_inter_image = {}; 
    mainCommandBuffers = {};
    extraCommandBuffers = {};
    
    render.createImageStorages(&simple_inter_image,
        VK_IMAGE_TYPE_2D,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
    {render.swapChainExtent.width, render.swapChainExtent.height, 1});
    for(auto img : simple_inter_image){
        render.transitionImageLayoutSingletime(&img, VK_IMAGE_LAYOUT_GENERAL);
    }
    // printl(simple_inter_image[0].view)
    // printl(simple_inter_image[1].view)

    render.descriptorBuilder
        .setLayout(&simple_raster_pipe.setLayout)
        .setDescriptorSets(&simple_raster_pipe.sets)
        .defer();
println
    render.descriptorBuilder
        .setLayout(&simple_posteffect_pipe.setLayout)
        .setDescriptorSets(&simple_posteffect_pipe.sets)
        .setDescriptions({
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, RD_FIRST, {/*empty*/}, &simple_inter_image, NO_SAMPLER, VK_IMAGE_LAYOUT_GENERAL, VK_SHADER_STAGE_FRAGMENT_BIT}
        })
        .defer();
println
    render.flushDescriptorSetup();
println

    render.renderPassBuilder.setAttachments({
            {&simple_inter_image,   DontCare, DontCare, DontCare, DontCare, {}, VK_IMAGE_LAYOUT_GENERAL},
            {&render.swapchainImages, DontCare, Store,    DontCare, DontCare, {}, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR},
        }).setSubpasses({
            {{&simple_raster_pipe},     {},                     {&simple_inter_image},   {}},
            {{&simple_posteffect_pipe}, {&simple_inter_image}, {&render.swapchainImages}, {}}
        }).build(&simple_rpass);

println
    render.pipeBuilder.setStages({
            {"examples/vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
            {"examples/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
        }).setExtent(render.swapChainExtent).setBlends({NO_BLEND})
        .buildRaster(&simple_raster_pipe);
println
    render.pipeBuilder.setStages({
            {"examples/vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
            {"examples/posteffect.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
        }).setExtent(render.swapChainExtent).setBlends({NO_BLEND})
        .buildRaster(&simple_posteffect_pipe);
println

    //you typically want to have FIF'count command buffers in their ring
    //but if you only need like 1 "baked" command buffer, just use 1
    render.createCommandBuffers ( &mainCommandBuffers, render.settings.fif);
    render.createCommandBuffers (&extraCommandBuffers, render.settings.fif);

    //you have to set this if you want to use builtin profiler
    render.mainCommandBuffers = &mainCommandBuffers;

    return VK_SUCCESS;
};

std::function<VkResult(void)> cleanupSwapchainDependent = [](){
    render.deviceWaitIdle();
    render.destroyRenderPass(&simple_rpass);
    render.destroyRasterPipeline(&simple_raster_pipe);
    render.destroyRasterPipeline(&simple_posteffect_pipe);
    render.deleteImages(&simple_inter_image);
    return VK_SUCCESS;
};

int main(){
    Settings settings = {};
        settings.vsync = false; //every time deciding to which image to render, wait until monitor draws current. Icreases perfomance, but limits fps
        settings.fullscreen = false;
        settings.debug = false; //Validation Layers. Use them while developing or be tricked into thinking that your code is correct
        settings.timestampCount = 128;
        settings.profile = false; //monitors perfomance via timestamps. You can place one with PLACE_TIMESTAMP() macro
        settings.fif = 2; // Frames In Flight. If 1, then record cmdbuff and submit it. If multiple, cpu will (might) be ahead of gpu by FIF-1, which makes GPU wait less
    render.init(settings);

    render.createSwapchainDependent = createSwapchainDependent;
    render.cleanupSwapchainDependent = cleanupSwapchainDependent;

    createSwapchainDependent();
println

    while(!glfwWindowShouldClose(render.window.pointer) && (glfwGetKey(render.window.pointer, GLFW_KEY_ESCAPE) != GLFW_PRESS)){
        glfwPollEvents();
println
        render.start_frame({mainCommandBuffers.current()});                
println
            render.cmdBeginRenderPass(mainCommandBuffers.current(), &simple_rpass);
println
                render.cmdBindPipe(mainCommandBuffers.current(), simple_raster_pipe);
println
                   render.cmdDraw(mainCommandBuffers.current(), 3, 1, 0, 0);
println
            render.cmdNextSubpass(mainCommandBuffers.current(), &simple_rpass);
println
                render.cmdBindPipe(mainCommandBuffers.current(), simple_posteffect_pipe);
println
                   render.cmdDraw(mainCommandBuffers.current(), 3, 1, 0, 0);
println
            render.cmdEndRenderPass(mainCommandBuffers.current(), &simple_rpass);
println
        render.end_frame({mainCommandBuffers.current()});
        //you are the one responsible for this, because using "previous" command buffer is quite common
        mainCommandBuffers.move();
        // static int ctr=0; ctr++; if(ctr==2) abort();
    }
println
    cleanupSwapchainDependent();
    render.cleanup();
}