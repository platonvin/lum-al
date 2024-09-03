#include "../src/al.hpp"

Renderer render = {};

int main(){
    Settings settings = {};
        settings.vsync = false; //every time deciding to which image to render, wait until monitor draws current. Icreases perfomance, but limits fps
        settings.fullscreen = false;
        settings.debug = false; //Validation Layers
        settings.timestampCount = 128;
        settings.profile = false; //monitors perfomance via timestamps. You can place one with PLACE_TIMESTAMP() macro
        settings.fif = 2; // Frames In Flight. If 1, then record cmdbuff and submit it. If multiple, cpu will (might) be ahead of gpu by FIF-1, which makes GPU wait less
    render.init(settings);

    RasterPipe simple_raster_pipe = {};

    render.descriptorBuilder
        .setLayout(&simple_raster_pipe.setLayout)
        .setDescriptorSets(&simple_raster_pipe.sets)
        .defer();
    render.flushDescriptorSetup();

    RenderPass simple_rpass = {};
    render.renderPassBuilder.setSubpasses({
            {{&simple_raster_pipe}, {}, {render.swapchainImages}, {}}
        }).setAttachments({
            {render.swapchainImages, DontCare, Store, DontCare, DontCare, {}, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR}
        }).build(&simple_rpass);

    render.pipeBuilder.setStages({
            {"examples/vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
            {"examples/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
        }).setExtent(render.swapChainExtent).setBlends({NO_BLEND})
        .buildRaster(&simple_raster_pipe);

    ring<VkCommandBuffer> mainCommandBuffers;
    ring<VkCommandBuffer> extraCommandBuffers; //runtime copies. Also does first frame resources
    //you typically want to have FIF'count command buffers in their ring
    //but if you only need like 1 "baked" command buffer, just use 1
    render.createCommandBuffers ( &mainCommandBuffers, render.settings.fif);
    render.createCommandBuffers (&extraCommandBuffers, render.settings.fif);

    //you have to set this if you want to use builtin profiler
    render.mainCommandBuffers = &mainCommandBuffers;

    while(!glfwWindowShouldClose(render.window.pointer) && (glfwGetKey(render.window.pointer, GLFW_KEY_ESCAPE) != GLFW_PRESS)){
        glfwPollEvents();
        render.start_frame({mainCommandBuffers.current()});                
            render.cmdBeginRenderPass(mainCommandBuffers.current(), &simple_rpass);
                render.cmdBindPipe(mainCommandBuffers.current(), simple_raster_pipe);
                   render.cmdDraw(mainCommandBuffers.current(), 3, 1, 0, 0);
            render.cmdEndRenderPass(mainCommandBuffers.current(), &simple_rpass);
        render.end_frame({mainCommandBuffers.current()});
        //you are the one responsible for this, because using "previous" command buffer is quite common
        mainCommandBuffers.move();
    }
}