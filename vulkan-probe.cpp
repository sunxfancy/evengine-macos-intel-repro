// Standalone diagnostic: no EVEngine or VKBuilder code, no window or swapchain.
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "color_vert_spv.inc"
#include "color_frag_spv.inc"

#define REQUIRE(call) do { VkResult r=(call); if(r!=VK_SUCCESS) { std::fprintf(stderr,"%s = %d at %d\n",#call,int(r),__LINE__); std::exit(2); } } while(0)
#define LOAD(name) auto name=reinterpret_cast<PFN_##name>(get(instance,#name)); if(!name){std::fprintf(stderr,"missing %s\n",#name);return 2;}
int main(int argc,char**argv) {
    if(argc!=2)return 2;
    void* library=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    if(!library){std::fprintf(stderr,"%s\n",dlerror());return 2;}
    auto get=reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(library,"vkGetInstanceProcAddr"));
    if(!get)return 2;
    VkInstance instance=VK_NULL_HANDLE;
    LOAD(vkCreateInstance)
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.pApplicationName="Standalone raster probe";app.apiVersion=VK_API_VERSION_1_1;
    VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ici.pApplicationInfo=&app;
    REQUIRE(vkCreateInstance(&ici,nullptr,&instance));
    LOAD(vkEnumeratePhysicalDevices) LOAD(vkGetPhysicalDeviceProperties) LOAD(vkGetPhysicalDeviceQueueFamilyProperties)
    LOAD(vkGetPhysicalDeviceMemoryProperties) LOAD(vkEnumerateDeviceExtensionProperties) LOAD(vkCreateDevice)
    LOAD(vkCreateImage) LOAD(vkGetImageMemoryRequirements) LOAD(vkAllocateMemory) LOAD(vkBindImageMemory)
    LOAD(vkCreateBuffer) LOAD(vkGetBufferMemoryRequirements) LOAD(vkBindBufferMemory) LOAD(vkMapMemory) LOAD(vkUnmapMemory)
    LOAD(vkCreateImageView) LOAD(vkCreateRenderPass) LOAD(vkCreateFramebuffer) LOAD(vkCreateShaderModule)
    LOAD(vkCreatePipelineLayout) LOAD(vkCreateGraphicsPipelines) LOAD(vkCreateCommandPool) LOAD(vkAllocateCommandBuffers)
    LOAD(vkBeginCommandBuffer) LOAD(vkCmdBeginRenderPass) LOAD(vkCmdBindPipeline) LOAD(vkCmdBindVertexBuffers)
    LOAD(vkCmdDraw) LOAD(vkCmdEndRenderPass) LOAD(vkCmdCopyImageToBuffer) LOAD(vkEndCommandBuffer)
    LOAD(vkGetDeviceQueue) LOAD(vkQueueSubmit) LOAD(vkQueueWaitIdle)
    uint32_t count=0;REQUIRE(vkEnumeratePhysicalDevices(instance,&count,nullptr));if(!count)return 2;
    std::vector<VkPhysicalDevice> devices(count);REQUIRE(vkEnumeratePhysicalDevices(instance,&count,devices.data()));
    auto physical=devices[0];VkPhysicalDeviceProperties props{};vkGetPhysicalDeviceProperties(physical,&props);
    std::printf("device=%s api=%u\n",props.deviceName,props.apiVersion);
    uint32_t nq=0;vkGetPhysicalDeviceQueueFamilyProperties(physical,&nq,nullptr);std::vector<VkQueueFamilyProperties> qs(nq);
    vkGetPhysicalDeviceQueueFamilyProperties(physical,&nq,qs.data());uint32_t family=0;
    while(family<nq && !(qs[family].queueFlags&VK_QUEUE_GRAPHICS_BIT))++family;if(family==nq)return 2;
    float priority=1;VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};qi.queueFamilyIndex=family;qi.queueCount=1;qi.pQueuePriorities=&priority;
    uint32_t ne=0;REQUIRE(vkEnumerateDeviceExtensionProperties(physical,nullptr,&ne,nullptr));std::vector<VkExtensionProperties> es(ne);
    REQUIRE(vkEnumerateDeviceExtensionProperties(physical,nullptr,&ne,es.data()));std::vector<const char*> extensions;
    for(auto&e:es)if(!std::strcmp(e.extensionName,"VK_KHR_portability_subset"))extensions.push_back("VK_KHR_portability_subset");
    VkDeviceCreateInfo di{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};di.queueCreateInfoCount=1;di.pQueueCreateInfos=&qi;di.enabledExtensionCount=uint32_t(extensions.size());di.ppEnabledExtensionNames=extensions.data();
    VkDevice device;REQUIRE(vkCreateDevice(physical,&di,nullptr,&device));
    VkPhysicalDeviceMemoryProperties memory{};vkGetPhysicalDeviceMemoryProperties(physical,&memory);
    auto allocate=[&](VkMemoryRequirements req,VkMemoryPropertyFlags flags){
        uint32_t type=0;while(type<memory.memoryTypeCount && (!(req.memoryTypeBits&(1u<<type)) || (memory.memoryTypes[type].propertyFlags&flags)!=flags))++type;
        if(type==memory.memoryTypeCount)std::exit(3);
        VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=req.size;a.memoryTypeIndex=type;
        VkDeviceMemory out;REQUIRE(vkAllocateMemory(device,&a,nullptr,&out));return out;
    };
    constexpr uint32_t size=64;
    VkImageCreateInfo im{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};im.imageType=VK_IMAGE_TYPE_2D;im.format=VK_FORMAT_R8G8B8A8_UNORM;im.extent={size,size,1};im.mipLevels=1;im.arrayLayers=1;im.samples=VK_SAMPLE_COUNT_1_BIT;im.tiling=VK_IMAGE_TILING_OPTIMAL;im.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    VkImage image;REQUIRE(vkCreateImage(device,&im,nullptr,&image));VkMemoryRequirements req{};vkGetImageMemoryRequirements(device,image,&req);auto imageMemory=allocate(req,0);REQUIRE(vkBindImageMemory(device,image,imageMemory,0));
    VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};bi.size=size*size*4;bi.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VkBuffer readback;REQUIRE(vkCreateBuffer(device,&bi,nullptr,&readback));vkGetBufferMemoryRequirements(device,readback,&req);auto readMemory=allocate(req,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);REQUIRE(vkBindBufferMemory(device,readback,readMemory,0));
    float vertices[]={-1,-1,1,0,0,1, 1,-1,1,0,0,1, 0,1,1,0,0,1};
    bi.size=sizeof(vertices);bi.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VkBuffer vertex;REQUIRE(vkCreateBuffer(device,&bi,nullptr,&vertex));vkGetBufferMemoryRequirements(device,vertex,&req);auto vertexMemory=allocate(req,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);REQUIRE(vkBindBufferMemory(device,vertex,vertexMemory,0));
    void* mapped;REQUIRE(vkMapMemory(device,vertexMemory,0,VK_WHOLE_SIZE,0,&mapped));std::memcpy(mapped,vertices,sizeof(vertices));vkUnmapMemory(device,vertexMemory);
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vi.image=image;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=im.format;vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};VkImageView view;REQUIRE(vkCreateImageView(device,&vi,nullptr,&view));
    VkAttachmentDescription attachment{};attachment.format=im.format;attachment.samples=im.samples;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachment.finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    VkAttachmentReference ref{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&ref;
    VkSubpassDependency dep{};dep.srcSubpass=0;dep.dstSubpass=VK_SUBPASS_EXTERNAL;dep.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;dep.dstStageMask=VK_PIPELINE_STAGE_TRANSFER_BIT;dep.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;dep.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
    VkRenderPassCreateInfo ri{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};ri.attachmentCount=1;ri.pAttachments=&attachment;ri.subpassCount=1;ri.pSubpasses=&sub;ri.dependencyCount=1;ri.pDependencies=&dep;VkRenderPass pass;REQUIRE(vkCreateRenderPass(device,&ri,nullptr,&pass));
    VkFramebufferCreateInfo fi{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fi.renderPass=pass;fi.attachmentCount=1;fi.pAttachments=&view;fi.width=size;fi.height=size;fi.layers=1;VkFramebuffer fb;REQUIRE(vkCreateFramebuffer(device,&fi,nullptr,&fb));
    auto shader=[&](const uint32_t* words,size_t bytes){VkShaderModuleCreateInfo s{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};s.codeSize=bytes;s.pCode=words;VkShaderModule m;REQUIRE(vkCreateShaderModule(device,&s,nullptr,&m));return m;};
    VkPipelineShaderStageCreateInfo stages[2]{};stages[0].sType=stages[1].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[0].module=shader(color_vert_spv,sizeof(color_vert_spv));stages[1].module=shader(color_frag_spv,sizeof(color_frag_spv));stages[0].pName=stages[1].pName="main";
    VkVertexInputBindingDescription binding{0,6*sizeof(float),VK_VERTEX_INPUT_RATE_VERTEX};VkVertexInputAttributeDescription attrs[]={{0,0,VK_FORMAT_R32G32_SFLOAT,0},{1,0,VK_FORMAT_R32G32B32A32_SFLOAT,2*sizeof(float)}};
    VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};input.vertexBindingDescriptionCount=1;input.pVertexBindingDescriptions=&binding;input.vertexAttributeDescriptionCount=2;input.pVertexAttributeDescriptions=attrs;
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkViewport viewport{0,0,float(size),float(size),0,1};VkRect2D scissor{{0,0},{size,size}};VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vp.viewportCount=1;vp.pViewports=&viewport;vp.scissorCount=1;vp.pScissors=&scissor;
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=VK_CULL_MODE_NONE;raster.lineWidth=1;
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState blend{};blend.colorWriteMask=15;VkPipelineColorBlendStateCreateInfo bs{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};bs.attachmentCount=1;bs.pAttachments=&blend;
    VkPipelineLayoutCreateInfo li{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};VkPipelineLayout layout;REQUIRE(vkCreatePipelineLayout(device,&li,nullptr,&layout));
    VkGraphicsPipelineCreateInfo pi{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};pi.stageCount=2;pi.pStages=stages;pi.pVertexInputState=&input;pi.pInputAssemblyState=&assembly;pi.pViewportState=&vp;pi.pRasterizationState=&raster;pi.pMultisampleState=&ms;pi.pColorBlendState=&bs;pi.layout=layout;pi.renderPass=pass;
    VkPipeline pipeline;REQUIRE(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&pi,nullptr,&pipeline));
    VkCommandPoolCreateInfo ci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};ci.queueFamilyIndex=family;VkCommandPool pool;REQUIRE(vkCreateCommandPool(device,&ci,nullptr,&pool));VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ca.commandPool=pool;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;VkCommandBuffer cb;REQUIRE(vkAllocateCommandBuffers(device,&ca,&cb));
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};REQUIRE(vkBeginCommandBuffer(cb,&begin));VkClearValue clear{};clear.color.float32[2]=1;clear.color.float32[3]=1;VkRenderPassBeginInfo rb{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};rb.renderPass=pass;rb.framebuffer=fb;rb.renderArea=scissor;rb.clearValueCount=1;rb.pClearValues=&clear;
    vkCmdBeginRenderPass(cb,&rb,VK_SUBPASS_CONTENTS_INLINE);vkCmdBindPipeline(cb,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);VkDeviceSize offset=0;vkCmdBindVertexBuffers(cb,0,1,&vertex,&offset);vkCmdDraw(cb,3,1,0,0);vkCmdEndRenderPass(cb);
    VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={size,size,1};vkCmdCopyImageToBuffer(cb,image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,readback,1,&copy);REQUIRE(vkEndCommandBuffer(cb));
    VkQueue queue;vkGetDeviceQueue(device,family,0,&queue);VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&cb;REQUIRE(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE));REQUIRE(vkQueueWaitIdle(queue));
    REQUIRE(vkMapMemory(device,readMemory,0,VK_WHOLE_SIZE,0,&mapped));auto p=static_cast<unsigned char*>(mapped)+(size/2*size+size/2)*4;
    bool ok=p[0]>240&&p[1]<10&&p[2]<10;std::printf("VULKAN center=%u,%u,%u,%u result=%s\n",p[0],p[1],p[2],p[3],ok?"PASS":"FAIL");std::fflush(stdout);
    // A process-owned diagnostic; the OS reclaims resources after the queue is idle.
    return ok?0:1;
}
