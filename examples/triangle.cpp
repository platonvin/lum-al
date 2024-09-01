#include "../src/al.hpp"
#include "defines.hpp"

Renderer render = {};

int main(){
    Settings settings = {};
        settings.vsync = false;
        settings.fullscreen = false;
        settings.debug = true;
        settings.timestampCount = 128;
        settings.profile = false;
    render.init(settings);

    RasterPipe simple_raster_pipe = {};
    render.deferDescriptorSetup (&simple_raster_pipe.setLayout, &simple_raster_pipe.sets, {
        //no descriptors bound, not needed for such simple case
    }, VK_SHADER_STAGE_VERTEX_BIT);
    render.flushDescriptorSetup();

    VkRenderPass simple_rpass = {};
    render.createRenderPass({
            {&render.swapchainImages[0], DontCare, Store, DontCare, DontCare, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR}
        }, {
            {{&simple_raster_pipe}, {}, {&render.swapchainImages[0]}, {}}
        }, &simple_rpass);
        
    render.createRasterPipeline (&simple_raster_pipe, 0, {
        {"examples/vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
        {"examples/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT},
    }, {/*no input geometry, it is generated in shader from vertex id*/},
    0, VK_VERTEX_INPUT_RATE_VERTEX, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    render.swapChainExtent, {NO_BLEND}, (0), NO_DEPTH_TEST, VK_CULL_MODE_NONE, NO_DISCARD, NO_STENCIL);


    vector<VkFramebuffer> simple_framebuffers = {};
    render.createFramebuffers(&simple_framebuffers, {render.swapchainImages}, simple_rpass, render.swapChainExtent);


    while(!glfwWindowShouldClose(render.window.pointer) && (glfwGetKey(render.window.pointer, GLFW_KEY_ESCAPE) != GLFW_PRESS)){
        glfwPollEvents();
        render.start_frame();                
                vector<VkClearValue> clearColors = {
                    {},
                };
                VkRenderPassBeginInfo 
                    renderPassInfo = {};
                    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    renderPassInfo.renderPass = simple_rpass;
                    renderPassInfo.framebuffer = simple_framebuffers[render.imageIndex];
                    renderPassInfo.renderArea.offset = {0, 0};
                    renderPassInfo.renderArea.extent = render.swapChainExtent;
                    renderPassInfo.clearValueCount = clearColors.size();
                    renderPassInfo.pClearValues = clearColors.data();
                vkCmdBeginRenderPass (render.graphicsCommandBuffers[render.currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                render.cmdBindPipe(render.graphicsCommandBuffers[render.currentFrame], simple_raster_pipe);
                render.cmdSetViewport(render.graphicsCommandBuffers[render.currentFrame], render.swapChainExtent);
//
                // VkDeviceSize offsets[] = {0};
                // VkBuffer n = VK_NULL_HANDLE;
                // vkCmdBindVertexBuffers(render.graphicsCommandBuffers[render.currentFrame], 0, 1, &n, offsets);
                vkCmdDraw(render.graphicsCommandBuffers[render.currentFrame], 3, 1, 0, 0);
                vkCmdEndRenderPass (render.graphicsCommandBuffers[render.currentFrame]);
            render.present();
        render.end_frame();
    }
}