#define GAME_MATH_IMPLEMENTATION
#include <vulkan/vulkan.h>
#include <cstdio>
#include "glass.h"
#include "render.h"
#include <cstring>
#include "assert.h"
#include "transform.h"
#include "Matrix4.h"
#include "Vector2.h"
#include "hash_table.h"
#include "text.h"
#include "list.h"
#include "queue.h"
#include "array.h"

#define MAX_FRAMES 2
#define POOL_SIZE  128

struct RenderPipeline;
struct GpuBuffer;

// @Temp ?
typedef struct PerFrameData {
    Matrix4 view;
    Matrix4 proj;
    float   time;
    float   dt;
} PerFrameData;

typedef struct Matrices {
    Matrix4 model;
    Matrix4 mvp;
} Matrices;

typedef struct GpuBuffer {
    VkBuffer       buffer;
    VkDeviceMemory memory;
} GpuBuffer;

// typedef struct GpuMemory {
//     GpuBuffer* buffer;
//     u64        offset;
//     u64        size;
// } GpuMemory;

typedef struct BufferManager {
    Array<GpuBuffer> buffers;
    u32              count;
    Queue<u32>       free;
} BufferManager;

typedef struct RenderPass {
    VkRenderPass             pass;
    VkAttachmentDescription* attachments;
    u32                      attachment_count;
    VkSubpassDescription*    subpasses;
    u32                      subpass_count;
} RenderPass;

typedef struct ShaderDataSet {
    VkDescriptorType      type;
    VkDescriptorSetLayout layout;
    VkBufferUsageFlags    usage;
    VkMemoryPropertyFlags mem_flags;
    u64                   buffer_size;
} ShaderDataSet;

typedef struct Shader {
    VkShaderModule  vert;
    VkShaderModule  frag;
    RenderPipeline* pipeline;
    ShaderDataSet*  data_sets;
    u32             set_count;
} Shader;

typedef struct Material {
    Shader*           shader;
    u32*              uniform_buffers[MAX_FRAMES];
    VkDescriptorPool* pools[MAX_FRAMES];
    VkDescriptorSet*  sets[MAX_FRAMES];
} Material;

typedef struct QueueFamilies {
    bool graphics_supported;
    bool present_supported;
    u32  graphics;
    u32  present;
} QueueFamilies;

typedef struct Application {
    Window*              window;
    VkInstance           instance;
    VkSurfaceKHR         surface;
    QueueFamilies        queues;
    VkPhysicalDevice     phys_device;
    VkDevice             device;
    VkSwapchainKHR       swapchain;
    VkExtent2D           extent;
    VkFramebuffer*       frame_buffers;
    VkImageView*         image_views;
    VkCommandPool        graphics_command_pool;
    VkCommandBuffer      graphics_cmd;
    VkQueue              graphics_queue;
    VkQueue              present_queue;
    VkFence              single_frame_fence;
    VkSemaphore          image_ready_semaphore;
    VkSemaphore          render_finished_semaphores[MAX_FRAMES];
    GpuBuffer*           vertex_buffer;
    GpuBuffer*           index_buffer;
    List<RenderPass>     render_passes;
    List<Shader>         shaders;
    List<Material>       materials;
    List<RenderPipeline> pipelines;
    BufferManager        buffers;

    HashTable<VkDescriptorType, VkDescriptorPool> pool_table[MAX_FRAMES];
} Application;

typedef struct RenderPipeline {
    VkPipeline       pipeline;
    VkPipelineLayout layout;
    VkViewport       viewport;
    VkRect2D         scissors;
    u32              render_pass;
    Shader*          shader;
} RenderPipeline;

typedef struct CommandBuffer {
    VkCommandPool   pool;
    VkCommandBuffer buffer;
    u32             image_index;
} CommandBuffer;

static Application  App;
static Shader*      Test_Shader   = NULL;
static Material*    Test_Material = NULL;

// internal
static inline RenderError     render_init_internal(Window* window, Application* app);
static inline void            render_destroy_internal(Application* app);
static inline RenderError     create_vk_instance(const char** extensions, u32 extensions_count, VkInstance* instance);
static inline RenderError     get_phys_device(VkInstance instance, const char** properties, u32 properties_count, VkPhysicalDevice* phys_device);
static inline RenderError     select_queues(VkPhysicalDevice phys_device, VkSurfaceKHR surface, QueueFamilies* queues);
static inline RenderError     create_logical_device(QueueFamilies queues, VkPhysicalDevice phys_device, const char** extensions, u32 extensions_count, VkDevice *device);
static inline RenderError     select_surface_format(VkPhysicalDevice phys_device, VkSurfaceKHR surface, VkFormat target_format, VkColorSpaceKHR target_color_space, VkSurfaceFormatKHR* surface_format);
static inline RenderError     select_present_mode(VkPhysicalDevice phys_device, VkSurfaceKHR surface, VkPresentModeKHR target, VkPresentModeKHR fallback, VkPresentModeKHR* present_mode);
static inline RenderError     create_swapchain(VkSurfaceCapabilitiesKHR surface_capabilities, VkSurfaceFormatKHR surface_format, VkPresentModeKHR present_mode);
static inline void            create_descriptor_set_layout(VkDescriptorSetLayoutBinding* bindings, u32 bindings_count, VkDescriptorSetLayout* dsl);
static inline RenderPipeline* render_pipeline_make(Shader* shader, RenderError* err);
static inline void            render_pipeline_destroy(RenderPipeline* pipeline);
static inline GpuBuffer*      buffer_allocate(u64 size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, RenderError* err, u32* index = NULL);
static inline void            buffer_free(GpuBuffer* buffer);
static inline void            buffer_copy(GpuBuffer* src, GpuBuffer* dst, u64 size);
static inline GpuBuffer*      buffer_get(u32 index);
static inline void            buffer_set(GpuBuffer* dst, void* data, u64 size);
static inline void            buffer_set(u32 dst, void* data, u64 size);

Material*       material_make(Shader* shader, RenderError* err);
void            material_destroy(Material* material);

RenderError render_pass_make(VkAttachmentDescription* attachments, u32 attachment_count, VkSubpassDescription* subpasses, u32 subpass_count, RenderPass* pass);
u32         render_pass_get_or_create(VkAttachmentDescription* attachments, u32 attachment_count, VkSubpassDescription* subpasses, u32 subpass_count, RenderPass* pass);
RenderPass* render_pass_get(u32 index);
void        render_pass_destroy(RenderPass* pass);

// glass.h
GlassErrorCode glass_create_vulkan_surface(Window* window, VkInstance vk_instance, VkSurfaceKHR* surface);
const char** glass_get_vulkan_instance_extensions(u32* count);

// render.h
RenderError render_init(Window* window) {
    RenderError err = render_init_internal(window, &App);

    if (err != RENDER_OK) {
        return err;
    }

    char* vert_text;
    char* frag_text;
    u32   vert_size;
    u32   frag_size;

    if (!read_entire_file_binary("vert.spv", &vert_text, &vert_size, &Allocator_Persistent)) {
        printf("Cannot read vertex shader file from disk\n");
        return RENDER_INTERNAL_ERROR;
    }

    if (!read_entire_file_binary("frag.spv", &frag_text, &frag_size, &Allocator_Persistent)) {
        printf("Cannot read vertex shader file from disk\n");
        return RENDER_INTERNAL_ERROR;
    }

    Test_Shader = shader_make(vert_text, frag_text, vert_size, frag_size, &err);

    if (err != RENDER_OK) {
        return err;
    }

    printf("Shader created.\n");

    Test_Material = material_make(Test_Shader, &err);

    if (err != RENDER_OK) {
        return err;
    }

    Assert(Test_Material, "Test_Material is NULL");

    Vector2 camera_pos = vector2_make(0, 0);
    float   camera_size = 5;
    u32     width;
    u32     height;
    glass_get_window_size(App.window, &width, &height);

    float   aspect = (float)width / (float)height;
    float   size_x = camera_size * aspect;
    float   size_y = camera_size;
    float   left   = camera_pos.x - size_x;
    float   right  = camera_pos.x + size_x;
    float   top    = camera_pos.y + size_y;
    float   bottom = camera_pos.y - size_y;

    Matrix4 view = matrix4_transform_2d(0, 0);
    Matrix4 proj = matrix4_ortho_2d(left, right, top, bottom);

    PerFrameData data = {
        .view = view,
        .proj = proj,
        .time = 2.0f,
        .dt   = 0.016f,
    };

    Matrix4 model = matrix4_trs_2d(0, 0, 0, 1, 1);
    Matrix4 mvp   = matrix4_mvp(camera_pos.x, camera_pos.y, left, right, top, bottom, 0, 0, 0, 1, 1);

    Matrices matrix = {
        .model = model,
        .mvp   = mvp,
    };

    buffer_set(Test_Material->uniform_buffers[0][0], &data, sizeof(PerFrameData));
    buffer_set(Test_Material->uniform_buffers[0][1], &matrix, sizeof(Matrices));
    buffer_set(Test_Material->uniform_buffers[1][0], &data, sizeof(PerFrameData));
    buffer_set(Test_Material->uniform_buffers[1][1], &matrix, sizeof(Matrices));

    if (err != RENDER_OK) {
        return err;
    }

    printf("Material created.\n");

    return err;
}

void render_destroy() {
    render_destroy_internal(&App);
}

Shader* shader_make(char* vert_text, char* frag_text, u32 vert_len, u32 frag_len, RenderError* err) {
    u32 shader_index = 0;
    Shader* shader = list_append_empty(&App.shaders, &shader_index);

    VkShaderModuleCreateInfo vert_info = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = NULL,
        .flags    = 0,
        .codeSize = vert_len,
        .pCode    = (u32*)vert_text
    };

    VkResult result = vkCreateShaderModule(App.device,
                                           &vert_info,
                                           NULL,
                                           &shader->vert);

    if (result != VK_SUCCESS) {
        printf("Cannot create vertex shader. Vulkan error: %d", result);
        *err = RENDER_INTERNAL_ERROR;
        list_remove_at(&App.shaders, shader_index);
        return NULL;
    }

    VkShaderModuleCreateInfo frag_info = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = NULL,
        .flags    = 0,
        .codeSize = frag_len,
        .pCode    = (u32*)frag_text
    };

    result = vkCreateShaderModule(App.device,
                                  &frag_info,
                                  NULL,
                                  &shader->frag);

    if (result != VK_SUCCESS) {
        printf("Cannot create fragment shader. Vulkan error: %d", result);
        *err = RENDER_INTERNAL_ERROR;
        list_remove_at(&App.shaders, shader_index);
        return NULL;
    }

    VkDescriptorSetLayoutBinding frame_bindings[] = {
    {
        .binding            = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount    = 1,
        .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = NULL,
    }};

    VkDescriptorSetLayoutBinding matrix_bindings[] = {
    {
        .binding            = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount    = 1,
        .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = NULL,
    }};


    VkDescriptorSetLayout* set_layouts = Calloc(VkDescriptorSetLayout, 2);

    create_descriptor_set_layout(frame_bindings, 1, &set_layouts[0]);
    create_descriptor_set_layout(matrix_bindings, 1, &set_layouts[1]);

    ShaderDataSet* descriptors = Calloc(ShaderDataSet, 2);

    descriptors[0] = {
        .type        = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .layout      = set_layouts[0],
        .usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .mem_flags   = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        .buffer_size = sizeof(PerFrameData),
    };

    descriptors[1] = {
        .type        = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .layout      = set_layouts[1],
        .usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .mem_flags   = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        .buffer_size = sizeof(Matrices),
    };

    shader->data_sets = descriptors;
    shader->set_count = 2;

    // RenderPipeline* pipeline = Make(RenderPipeline);

    RenderPipeline* pipeline = render_pipeline_make(shader, err);

    if ((*err) != RENDER_OK) {
        printf("Cannot create render pipeline for this shader.\n");
        return NULL;
    }

    Assert(pipeline, "pipeline is NULL");

    shader->pipeline = pipeline;

    *err = RENDER_OK;

    return shader;
}

void shader_destroy(Shader* shader) {
    vkDestroyShaderModule(App.device, shader->vert, NULL);
    vkDestroyShaderModule(App.device, shader->frag, NULL);

    for (u32 i = 0; i < shader->set_count; i++) {
        vkDestroyDescriptorSetLayout(App.device, shader->data_sets[i].layout, NULL);
    }
}

RenderError render_test() {
    Material*       mat      = Test_Material;
    Shader*         shader   = mat->shader;
    RenderPipeline* pipeline = shader->pipeline;

    vkWaitForFences(App.device, 1, &App.single_frame_fence, VK_TRUE, u64_max);
    vkResetFences(App.device, 1, &App.single_frame_fence);

    u32 image_index;
    VkResult result = vkAcquireNextImageKHR(App.device, 
                                            App.swapchain, 
                                            u64_max,
                                            App.image_ready_semaphore, 
                                            VK_NULL_HANDLE, 
                                            &image_index);

    if (result != VK_SUCCESS) {
        printf("Cannot acquire next image. Vulkan error: %d\n", result);
        return RENDER_INTERNAL_ERROR;
    }
    
    result = vkResetCommandBuffer(App.graphics_cmd, 0);

    if (result != VK_SUCCESS) {
        printf("Cannot reset command buffer. Vulkan error: %d\n", result);
        return RENDER_INTERNAL_ERROR;
    }
    
    VkCommandBufferBeginInfo begin_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = NULL,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL
    };

    result = vkBeginCommandBuffer(App.graphics_cmd, &begin_info);

    if (result != VK_SUCCESS) {
        printf("Cannot begin command buffer. Vulkan error: %d\n", result);
        return RENDER_INTERNAL_ERROR;
    }

    VkClearValue clear_color = {
        .color = {{0, 0, 0, 0}}
    };

    // printf("%f, %f\n", pipeline->viewport.width, pipeline->viewport.height);
    vkCmdSetViewport(App.graphics_cmd, 0, 1, &pipeline->viewport);
    vkCmdSetScissor(App.graphics_cmd, 0, 1, &pipeline->scissors);

    RenderPass* pass = render_pass_get(pipeline->render_pass);

    VkRenderPassBeginInfo render_pass_begin_info = {
        .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext       = NULL,
        .renderPass  = pass->pass,
        .framebuffer = App.frame_buffers[image_index],
        .renderArea  = {
            .offset = {0, 0},
            .extent = App.extent
        },
        .clearValueCount = 1,
        .pClearValues    = &clear_color
    };

    vkCmdBeginRenderPass(App.graphics_cmd, 
                         &render_pass_begin_info, 
                         VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(App.graphics_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

    vkCmdBindDescriptorSets(App.graphics_cmd, 
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline->layout,
                            0, shader->set_count,
                            mat->sets[image_index],
                            0, NULL);

    const VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(App.graphics_cmd, 0, 1, &App.vertex_buffer->buffer, offsets);
    vkCmdBindIndexBuffer(App.graphics_cmd, App.index_buffer->buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(App.graphics_cmd, 6, 1, 0, 0, 0);

    vkCmdEndRenderPass(App.graphics_cmd);
    result = vkEndCommandBuffer(App.graphics_cmd);

    if (result != VK_SUCCESS) {
        printf("Cannot end command buffer. Vulkan error: %d\n", result);
        return RENDER_INTERNAL_ERROR;
    }

    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit_info = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext                = NULL,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &App.image_ready_semaphore,
        .pWaitDstStageMask    = wait_stages,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &App.graphics_cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &App.render_finished_semaphores[image_index]
    };

    result = vkQueueSubmit(App.graphics_queue, 1, &submit_info, App.single_frame_fence);

    if (result != VK_SUCCESS) {
        printf("Cannot submit queue. Vulkan error: %d\n", result);
        return RENDER_INTERNAL_ERROR;
    }

    VkPresentInfoKHR present_info = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &App.render_finished_semaphores[image_index],
        .swapchainCount     = 1,
        .pSwapchains        = &App.swapchain,
        .pImageIndices      = &image_index
    };

    result = vkQueuePresentKHR(App.present_queue, &present_info);

    if (result != VK_SUCCESS) {
        printf("Cannot draw. Vulkan error: %d\n", result);
        return RENDER_INTERNAL_ERROR;
    }

    return RENDER_OK;
}

RenderError create_command_buffer(CommandBuffer* cmd) {
    VkCommandPoolCreateInfo command_pool_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = NULL,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = App.queues.graphics
    };

    VkResult result = vkCreateCommandPool(App.device,
                                          &command_pool_info,
                                          NULL,
                                          &cmd->pool);

    if (result != VK_SUCCESS) {
        printf("Cannot create command pool. Vulkan error: %d\n", result);
        return RENDER_INTERNAL_ERROR;
    }

    VkCommandBufferAllocateInfo command_buffer_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = NULL,
        .commandPool        = cmd->pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    result = vkAllocateCommandBuffers(App.device,
                                      &command_buffer_info,
                                      &cmd->buffer);

    if (result != VK_SUCCESS) {
        printf("Cannot allocate command buffer. Vulkan error: %d\n", result);
        return RENDER_INTERNAL_ERROR;
    }

    return RENDER_OK;
}

void free_command_buffer(CommandBuffer* cmd) {
    vkFreeCommandBuffers(App.device, cmd->pool, 1, &cmd->buffer);
    vkDestroyCommandPool(App.device, cmd->pool, NULL);
}

void cmd_begin_render(CommandBuffer* cmd) {
    vkWaitForFences(App.device, 1, &App.single_frame_fence, VK_TRUE, u64_max);
    vkResetFences(App.device, 1, &App.single_frame_fence);

    u32 image_index;
    vkAcquireNextImageKHR(App.device, 
                          App.swapchain, 
                          u64_max,
                          App.image_ready_semaphore, 
                          VK_NULL_HANDLE, 
                          &image_index);
    
    vkResetCommandBuffer(cmd->buffer, 0);
    cmd->image_index = image_index;
}

void cmd_begin_command_buffer(CommandBuffer* cmd) {
    vkResetCommandBuffer(cmd->buffer, 0);

    VkCommandBufferBeginInfo begin_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = NULL,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL
    };

    vkBeginCommandBuffer(cmd->buffer, &begin_info);
}

// internal
static inline RenderPipeline* render_pipeline_make(Shader* shader, RenderError* err) {
    u32 pipeline_index = 0;
    RenderPipeline* pipeline = list_append_empty(&App.pipelines, &pipeline_index);
    VkPipelineShaderStageCreateInfo vert_shader_info = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext  = NULL,
        .flags  = 0,
        .stage  = VK_SHADER_STAGE_VERTEX_BIT,
        .module = shader->vert,
        .pName  = "main",
        .pSpecializationInfo = NULL
    };

    VkPipelineShaderStageCreateInfo frag_shader_info = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext  = NULL,
        .flags  = 0,
        .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = shader->frag,
        .pName  = "main",
        .pSpecializationInfo = NULL
    };

    VkPipelineShaderStageCreateInfo shader_stages[2] = {vert_shader_info, frag_shader_info};

    VkVertexInputBindingDescription vertex_binding = {
        .binding   = 0,
        .stride    = sizeof(Vertex2D),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkVertexInputAttributeDescription vertex_attributes[2] = {
        {
            .location = 0,                          // inPosition
            .binding  = 0,
            .format   = VK_FORMAT_R32G32_SFLOAT,
            .offset   = offsetof(Vertex2D, position)
        },
        {
            .location = 1,                            // inColor  
            .binding  = 0,
            .format   = VK_FORMAT_R8G8B8A8_UNORM,
            .offset   = offsetof(Vertex2D, color)
        }
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = NULL,
        .vertexBindingDescriptionCount   = 1,
        .pVertexBindingDescriptions      = &vertex_binding,
        .vertexAttributeDescriptionCount = 2,
        .pVertexAttributeDescriptions    = vertex_attributes
    };

    VkPipelineInputAssemblyStateCreateInfo assembly_input_info = {
        .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext    = NULL,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    pipeline->viewport = {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = (float)App.extent.width,
        .height   = (float)App.extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    pipeline->scissors = {
        .offset = {0, 0},
        .extent = App.extent
    };

    VkPipelineViewportStateCreateInfo viewport_info = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext         = NULL,
        .flags         = 0,
        .viewportCount = 1,
        .pViewports    = NULL,
        .scissorCount  = 1,
        .pScissors     = NULL
    };

    VkPipelineRasterizationStateCreateInfo rasterization_info = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext                   = NULL,
        .flags                   = 0,
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .cullMode                = VK_CULL_MODE_BACK_BIT,
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .depthBiasConstantFactor = 0,
        .depthBiasClamp          = 0,
        .depthBiasSlopeFactor    = 0,
        .lineWidth               = 1.0f
    };

    VkPipelineMultisampleStateCreateInfo multisampling_info = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext                 = NULL,
        .flags                 = 0,
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable   = VK_FALSE,
        .minSampleShading      = 0,
        .pSampleMask           = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable      = VK_FALSE
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .blendEnable         = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
        .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo color_blend_info = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext           = NULL,
        .flags           = 0,
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments    = &color_blend_attachment,
        .blendConstants  = {0, 0, 0, 0}
    };

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_info = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext             = NULL,
        .flags             = 0,
        .dynamicStateCount = 2,
        .pDynamicStates    = dynamic_states
    };
    
    VkDescriptorSetLayout* set_layouts = Calloc(VkDescriptorSetLayout, shader->set_count);

    for (u32 i = 0; i < shader->set_count; i++) {
        set_layouts[i] = shader->data_sets[i].layout;
    }

    VkPipelineLayoutCreateInfo layout_info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = NULL,
        .flags                  = 0,
        .setLayoutCount         = shader->set_count,
        .pSetLayouts            = set_layouts,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = NULL
    };

    VkResult result = vkCreatePipelineLayout(App.device,
                                             &layout_info,
                                             NULL,
                                             &pipeline->layout);

    Free(set_layouts);

    if (result != VK_SUCCESS) {
        printf("Cannot create pipeline layout. %d", result);
        *err = RENDER_INTERNAL_ERROR;
        return NULL;
    }

    printf("Pipeline layout created.\n");

    VkAttachmentDescription color_attachment = {
        .flags          = 0,
        .format         = VK_FORMAT_R8G8B8A8_SRGB,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &color_attachment_ref
    };

    RenderPass pass;

    pipeline->render_pass = render_pass_get_or_create(&color_attachment, 1, &subpass, 1, &pass);

    printf("Render pass created.\n");

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = NULL,
        .flags               = 0,
        .stageCount          = 2,
        .pStages             = shader_stages,
        .pVertexInputState   = &vertex_input_info,
        .pInputAssemblyState = &assembly_input_info,
        .pTessellationState  = NULL,
        .pViewportState      = &viewport_info,
        .pRasterizationState = &rasterization_info,
        .pMultisampleState   = &multisampling_info,
        .pDepthStencilState  = NULL,
        .pColorBlendState    = &color_blend_info,
        .pDynamicState       = &dynamic_info,
        .layout              = pipeline->layout,
        .renderPass          = pass.pass,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1
    };

    result = vkCreateGraphicsPipelines(App.device,
                                       VK_NULL_HANDLE,
                                       1,
                                       &pipeline_info,
                                       NULL,
                                       &pipeline->pipeline);

    if (result != VK_SUCCESS) {
        printf("Cannot create render pipeline. Vulkan error %d.\n", result);
        *err = RENDER_INTERNAL_ERROR;
        return NULL;
    }

    pipeline->shader = shader;
    shader->pipeline = pipeline;

    *err = RENDER_OK;
    return pipeline;
}

static inline void render_pipeline_destroy(RenderPipeline* pipeline) {
    vkDestroyPipeline(App.device, pipeline->pipeline, NULL);
    vkDestroyPipelineLayout(App.device, pipeline->layout, NULL);
}

static inline GpuBuffer* buffer_allocate(u64 size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, RenderError* err, u32* index) {
    u32 buffer_index = 0;

    if (App.buffers.free.count > 0) {
        buffer_index = queue_dequeue(&App.buffers.free);
    } else {
        buffer_index = App.buffers.count++;

        if (App.buffers.count >= App.buffers.buffers.length) {
            array_realloc(&App.buffers.buffers, App.buffers.count * 2);
        }
    }

    GpuBuffer* buffer = array_get_ptr(&App.buffers.buffers, buffer_index);

    VkBufferCreateInfo    buffer_info = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = size,
        .usage       = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VkResult result = vkCreateBuffer(App.device, &buffer_info, NULL, &buffer->buffer);

    if (result != VK_SUCCESS) {
        printf("Cannot create buffer. Vulkan error: %d.\n", result);
        *err = RENDER_INTERNAL_ERROR;
        return NULL;
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(App.device, buffer->buffer, &mem_requirements);

    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(App.phys_device, &mem_properties);
    
    u32 mem_type_index = 0;

    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((mem_requirements.memoryTypeBits & (1 << i)) && 
            (mem_properties.memoryTypes[i].propertyFlags & props) == props) {
            mem_type_index = i;
            break;
        }
    }

    VkMemoryAllocateInfo alloc_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = mem_requirements.size,
        .memoryTypeIndex = mem_type_index
    };

    result = vkAllocateMemory(App.device, &alloc_info, NULL, &buffer->memory);

    if (result != VK_SUCCESS) {
        printf("Cannot allocate memory for buffer. Vulkan error: %d.\n", result);
        *err = RENDER_INTERNAL_ERROR;
        return NULL;
    }

    result = vkBindBufferMemory(App.device, buffer->buffer, buffer->memory, 0);

    if (result != VK_SUCCESS) {
        printf("Cannot bind buffer with memory. Vulkan error: %d.\n", result);
        *err = RENDER_INTERNAL_ERROR;
        return NULL;
    }

    if (index != NULL) {
        *index = buffer_index;
    }

    return buffer;
}

static inline void buffer_free(GpuBuffer* buffer) {
    u32 index = array_index_of_ptr(&App.buffers.buffers, buffer);
    printf("Buffer index: %d.\n", index);
    vkDestroyBuffer(App.device, buffer->buffer, NULL);
    vkFreeMemory(App.device, buffer->memory, NULL);
    queue_enqueue(&App.buffers.free, index);
}

static inline void buffer_free(u32 index) {
    GpuBuffer* buffer = buffer_get(index);
    buffer_free(buffer);
}

static inline void buffer_copy(GpuBuffer* src, GpuBuffer* dst, u64 size) {
    VkCommandBufferAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = App.graphics_command_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(App.device, &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(command_buffer, &begin_info);

    VkBufferCopy copy_region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size      = size
    };

    vkCmdCopyBuffer(command_buffer, src->buffer, dst->buffer, 1, &copy_region);
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &command_buffer
    };

    vkQueueSubmit(App.graphics_queue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(App.graphics_queue);

    vkFreeCommandBuffers(App.device, App.graphics_command_pool, 1, &command_buffer);
}

static inline GpuBuffer* buffer_get(u32 index) {
    return array_get_ptr(&App.buffers.buffers, index);
}

static inline void buffer_set(GpuBuffer* dst, void* data, u64 size) {
    void* buffer_data;

    vkMapMemory(App.device, dst->memory, 0, size, 0, &buffer_data);
    memcpy(buffer_data, data, size);
    vkUnmapMemory(App.device, dst->memory);
}

static inline void buffer_set(u32 dst_index, void* data, u64 size) {
    GpuBuffer* dst = buffer_get(dst_index);
    buffer_set(dst, data, size);
}

static inline RenderError render_init_internal(Window* window, Application* app) {
    app->window        = window;
    list_make<RenderPass>(&app->render_passes);
    list_make<Shader>(&app->shaders);
    list_make<Material>(&app->materials);
    list_make<RenderPipeline>(&app->pipelines);
    array_make<GpuBuffer>(&app->buffers.buffers, 128);
    queue_make<u32>(&app->buffers.free);

    u32 instance_extensions_count;
    const char** instance_extensions = glass_get_vulkan_instance_extensions(&instance_extensions_count);

    RenderError result = create_vk_instance(instance_extensions, instance_extensions_count, &app->instance);

    if (result != RENDER_OK) {
        return result;
    }

    const char* phys_device_properties[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    result = get_phys_device(app->instance, phys_device_properties, 1, &app->phys_device);

    if (result != RENDER_OK) {
        return result;
    }

    GlassErrorCode glass_error = glass_create_vulkan_surface(window, app->instance, &app->surface);

    if (glass_error != GLASS_OK) {
        return RENDER_INTERNAL_ERROR;
    }

    result = select_queues(app->phys_device, app->surface, &app->queues);

    if (result != RENDER_OK) {
        return result;
    }

    u32   enabled_extensions_count = 1;
    const char* extension_names[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    result = create_logical_device(app->queues, app->phys_device, extension_names, enabled_extensions_count, &app->device);

    if (result != RENDER_OK) {
        return result;
    }

    VkSurfaceCapabilitiesKHR surface_capabilities;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(app->phys_device,
                                              app->surface,
                                              &surface_capabilities);
    VkExtent2D extent = surface_capabilities.currentExtent;

    if (extent.width > surface_capabilities.maxImageExtent.width) {
        extent.width = surface_capabilities.maxImageExtent.width;
    }

    if (extent.height > surface_capabilities.maxImageExtent.height) {
        extent.height = surface_capabilities.maxImageExtent.height;
    }
    
    app->extent = extent;
    auto max_extent = surface_capabilities.maxImageExtent;
    auto min_extent = surface_capabilities.minImageExtent;

    printf("Current extent: x:%u, y:%u\n", app->extent.width, app->extent.height);
    printf("Max extent: x:%u, y:%u\n", max_extent.width, max_extent.height);
    printf("Min extent: x:%u, y:%u\n", min_extent.width, min_extent.height);
    printf("Min image count: %u\n", surface_capabilities.minImageCount);
    printf("Max image count: %u\n", surface_capabilities.maxImageCount);
    printf("Max image array layers: %u\n", surface_capabilities.maxImageArrayLayers);

    VkSurfaceFormatKHR surface_format{};

    result = select_surface_format(app->phys_device, app->surface, VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, &surface_format);

    if (result != RENDER_OK) {
        return result;
    }
    
    VkPresentModeKHR present_mode;

    result = select_present_mode(app->phys_device, app->surface, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR, &present_mode);

    if (result != RENDER_OK) {
        return result;
    }

    printf("Present mode: %i\n", present_mode);

    result = create_swapchain(surface_capabilities, surface_format, present_mode);

    if (result != RENDER_OK) {
        return result;
    }

    u32 image_count = 0;

    vkGetSwapchainImagesKHR(app->device, 
                            app->swapchain,
                            &image_count,
                            NULL);

    printf("Image count: %d\n", image_count);
    
    VkImage* images = Calloc(VkImage, image_count);

    VkResult res = vkGetSwapchainImagesKHR(app->device,
                                           app->swapchain,
                                           &image_count,
                                           images);

    if (res != VK_SUCCESS) {
        printf("Cannot get swapchain images. Vulkan error: %d\n", res);
        return RENDER_INTERNAL_ERROR;
    }

    VkImageView* image_views = Calloc(VkImageView, image_count);

    for(u32 i = 0; i < image_count; i++) {
        VkImageView image_view;

        VkImageViewCreateInfo image_view_info = {
            .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext      = NULL,
            .flags      = 0,
            .image      = images[i],
            .viewType   = VK_IMAGE_VIEW_TYPE_2D,
            .format     = surface_format.format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            }
        };

        res = vkCreateImageView(app->device, 
                                &image_view_info,
                                NULL,
                                &image_view);
        
        if (res != VK_SUCCESS) {
            printf("Cannot create image view for image %d. Vulkan error: %d\n", i, res);
            return RENDER_INTERNAL_ERROR;
        }
        
        image_views[i] = image_view;
    }

    App.image_views = image_views;

    printf("Image views created.\n");

    VkAttachmentDescription color_attachment = {
        .flags          = 0,
        .format         = VK_FORMAT_R8G8B8A8_SRGB,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &color_attachment_ref
    };

    RenderPass pass;

    render_pass_get_or_create(&color_attachment, 1, &subpass, 1, &pass);

    App.frame_buffers = Calloc(VkFramebuffer, image_count);

    // create frame buffers
    for (u32 i = 0; i < image_count; i++) {
        VkFramebuffer frame_buffer;

        VkFramebufferCreateInfo frame_buffer_info = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext           = NULL,
            .flags           = 0,
            .renderPass      = pass.pass,
            .attachmentCount = 1,
            .pAttachments    = &image_views[i],
            .width           = app->extent.width,
            .height          = app->extent.height,
            .layers          = 1
        };

        VkResult result = vkCreateFramebuffer(app->device,
                                              &frame_buffer_info,
                                              NULL,
                                              &frame_buffer);

        if (result != VK_SUCCESS) {
            printf("Cannot create frame buffer. Vulkan error: %d\n", result);
            return RENDER_INTERNAL_ERROR;
        }

        app->frame_buffers[i] = frame_buffer;
    }

    printf("Frame buffers created.\n");

    VkCommandPoolCreateInfo command_pool_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = NULL,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = app->queues.graphics
    };

    res = vkCreateCommandPool(app->device,
                              &command_pool_info,
                              NULL,
                              &app->graphics_command_pool);

    if (res != VK_SUCCESS) {
        printf("Cannot create command pool. Vulkan error: %d.\n", res);
        return RENDER_INTERNAL_ERROR;
    }

    printf("Command pool created.\n");

    VkCommandBufferAllocateInfo command_buffer_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = NULL,
        .commandPool        = app->graphics_command_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    res = vkAllocateCommandBuffers(app->device,
                                   &command_buffer_info,
                                   &app->graphics_cmd);

    if (res != VK_SUCCESS) {
        printf("Cannot create command buffer. Vulkan error: %d.\n", res);
        return RENDER_INTERNAL_ERROR;
    }

    printf("Command buffer created.\n");

    vkGetDeviceQueue(app->device,
                     app->queues.graphics,
                     0,
                     &app->graphics_queue);

    vkGetDeviceQueue(app->device,
                     app->queues.present,
                     0,
                     &app->present_queue);

    // sync
    VkSemaphoreCreateInfo image_ready_semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0
    };

    VkSemaphoreCreateInfo render_finished_semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0
    };

    VkFenceCreateInfo single_frame_fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    vkCreateFence(app->device, &single_frame_fence_info, NULL, &app->single_frame_fence);
    vkCreateSemaphore(app->device, &image_ready_semaphore_info, NULL, &app->image_ready_semaphore);
    
    for (u32 i = 0; i < MAX_FRAMES; i++) {
        vkCreateSemaphore(app->device, &render_finished_semaphore_info, NULL, &app->render_finished_semaphores[i]);
    }

    Vertex2D vertices[] = {
        {{{   0.5f,  -0.5f }}, { 255, 0,   0,   255 }},
        {{{   0.5f,   0.5f }}, { 0,   255, 0,   255 }},
        {{{  -0.5f,   0.5f }}, { 0,   0,   255, 255 }},
        {{{  -0.5f,  -0.5f }}, { 255, 255, 255, 255 }},
    };

    u16 indices[] = {
        0, 1, 2, 0, 2, 3
    };

    u32 vertex_count = 4;
    u32 index_count  = 6;

    u64 size = sizeof(vertices[0]) * vertex_count;

    RenderError err;

    GpuBuffer* staging_buffer = buffer_allocate(size, 
                                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                &err);

    if (err != RENDER_OK) {
        return err;
    }
    
    void* data;
    vkMapMemory(app->device, staging_buffer->memory, 0, size, 0, &data);
    memcpy(data, vertices, size);
    vkUnmapMemory(app->device, staging_buffer->memory);

    app->vertex_buffer = buffer_allocate(size, 
                                         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                         &err);

    if (err != RENDER_OK) {
        return err;
    }

    buffer_copy(staging_buffer, app->vertex_buffer, size);

    buffer_free(staging_buffer);

    u64 index_size = sizeof(indices[0]) * index_count;

    staging_buffer = buffer_allocate(index_size, 
                                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                     &err);

    if (err != RENDER_OK) {
        return err;
    }
    
    vkMapMemory(app->device, staging_buffer->memory, 0, index_size, 0, &data);
    memcpy(data, indices, index_size);
    vkUnmapMemory(app->device, staging_buffer->memory);

    app->index_buffer = buffer_allocate(index_size, 
                                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                        &err);

    if (err != RENDER_OK) {
        return err;
    }

    buffer_copy(staging_buffer, app->index_buffer, index_size);

    buffer_free(staging_buffer);

    for (u32 i = 0; i < MAX_FRAMES; i++) {
        hash_table_make<VkDescriptorType, VkDescriptorPool>(&app->pool_table[i]);
    }
    

    printf("Init over.\n");

    return RENDER_OK;
}

static void render_destroy_internal(Application* app) {
    vkDeviceWaitIdle(App.device);

    for(u32 i = 0; i < app->materials.count; i++) {
        material_destroy(list_get_ptr(&app->materials, i));
    }

    list_clear(&app->materials);

    for(u32 i = 0; i < app->render_passes.count; i++) {
        render_pass_destroy(list_get_ptr(&app->render_passes, i));
    }

    list_clear(&app->render_passes);

    for(u32 i = 0; i < app->pipelines.count; i++) {
        render_pipeline_destroy(list_get_ptr(&app->pipelines, i));
    }

    list_clear(&app->pipelines);

    for(u32 i = 0; i < app->shaders.count; i++) {
        shader_destroy(list_get_ptr(&app->shaders, i));
    }

    list_clear(&app->shaders);

    for(u32 i = 0; i < app->buffers.count; i++) {
        if (queue_contains(&app->buffers.free, i) == false) {
            buffer_free(i);
        }
    }

    array_clear(&app->buffers.buffers);

    vkDestroySemaphore(app->device, app->image_ready_semaphore, NULL);
    vkDestroyFence(app->device, app->single_frame_fence, NULL);
    for (u32 i = 0; i < MAX_FRAMES; i++) {
        vkDestroySemaphore(app->device, app->render_finished_semaphores[i], NULL);
    }

    vkDestroyCommandPool(App.device, App.graphics_command_pool, NULL);

    for (u32 i = 0; i < MAX_FRAMES; i++) {
        vkDestroyFramebuffer(App.device, App.frame_buffers[i], NULL);
        vkDestroyImageView(App.device, App.image_views[i], NULL);
    }

    vkDestroySwapchainKHR(app->device, app->swapchain, NULL);
    vkDestroyDevice(app->device, NULL);
    vkDestroySurfaceKHR(app->instance, app->surface, NULL);
    vkDestroyInstance(app->instance, NULL);
}

static RenderError create_vk_instance(const char** extensions, u32 extensions_count, VkInstance* instance) {
    const char* validation_layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    u32 layers_count = 1;

    VkApplicationInfo    vk_app_info {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "space_game",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName        = "Custom Engine",
        .engineVersion      = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion         = VK_API_VERSION_1_0,
    };

    VkInstanceCreateInfo vk_info {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &vk_app_info,
        .enabledLayerCount       = layers_count,
        .ppEnabledLayerNames     = validation_layers,
        .enabledExtensionCount   = extensions_count,
        .ppEnabledExtensionNames = extensions,
    };

    VkResult result = vkCreateInstance(&vk_info, null, instance);

    if (result != VK_SUCCESS) {
        printf("Cannot create VkInstance, %d\n", result);
        return RENDER_INTERNAL_ERROR;
    }

    return RENDER_OK;
}

static RenderError get_phys_device(VkInstance instance, const char** properties, u32 properties_count, VkPhysicalDevice* phys_device) {
    u32               phys_devices_count;
    VkPhysicalDevice* devices;
    vkEnumeratePhysicalDevices(instance, &phys_devices_count, null);
    devices = Calloc(VkPhysicalDevice, phys_devices_count);
    vkEnumeratePhysicalDevices(instance, &phys_devices_count, devices);

    // printf("GPU count: %i\n", phys_devices_count);

    if (phys_devices_count == 0) {
        return RENDER_NO_GPU_FOUND;
    }

    for (u32 i = 0; i < phys_devices_count; i++) {
        u32 extensions_count;
        u32 properties_found = 0;

        vkEnumerateDeviceExtensionProperties(devices[i],
                                             NULL,
                                             &extensions_count,
                                             NULL);
        
        VkExtensionProperties* extension_properties = Calloc(VkExtensionProperties, extensions_count);

        vkEnumerateDeviceExtensionProperties(devices[i],
                                             NULL,
                                             &extensions_count,
                                             extension_properties);


        for (u32 j = 0; j < extensions_count; j++) {
            for (u32 k = 0; k < properties_count; k++) {
                if (strcmp(extension_properties[j].extensionName, properties[k]) == 0) {
                    properties_found++;
                    break;
                }
            }
        }

        Free(extension_properties);

        if (properties_found == properties_count) {
            *phys_device = devices[i];
            return RENDER_OK;
        }
    }

    printf("Not all physical device properties have been found.\n");

    return RENDER_INTERNAL_ERROR;
}

static RenderError select_queues(VkPhysicalDevice phys_device, VkSurfaceKHR surface, QueueFamilies* queues) {
    u32 family_count;

    vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &family_count, NULL);

    VkQueueFamilyProperties* family_properties = Calloc(VkQueueFamilyProperties, family_count);

    vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &family_count, family_properties);

    bool graphics_queue_found = false;
    bool present_queue_found = false;
    u32  graphics_queue_index;
    u32  present_queue_index;

    for (u32 i = 0; i < family_count; i++) {
        if (family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && graphics_queue_found == false) {
            graphics_queue_index = i;
            graphics_queue_found = true;
        }
        
        VkBool32 present_supported = false;

        vkGetPhysicalDeviceSurfaceSupportKHR(phys_device,
                                             i,
                                             surface,
                                             &present_supported);

        if (present_supported && present_queue_found == false) {
            present_queue_index = i;
            present_queue_found = true;
        }

        if (graphics_queue_found && present_queue_found) break;
    }

    if (!graphics_queue_found) {
        printf("Graphics queue was not found\n");
        return RENDER_INTERNAL_ERROR;
    }

    if (!present_queue_found) {
        printf("Present queue was not found\n");
        return RENDER_INTERNAL_ERROR;
    }

    queues->graphics_supported = graphics_queue_found;
    queues->graphics           = graphics_queue_index;
    queues->present_supported  = present_queue_found;
    queues->present            = present_queue_index;

    return RENDER_OK;
}

static RenderError create_logical_device(QueueFamilies queues, VkPhysicalDevice phys_device, const char** extensions, u32 extensions_count, VkDevice *device) {
    VkPhysicalDeviceFeatures device_features = {
        .fillModeNonSolid  = VK_TRUE,
        .samplerAnisotropy = VK_TRUE
    };

    if (queues.present == queues.graphics) {
        float priority = 1.0f;
        VkDeviceQueueCreateInfo queue_create_info = {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queues.graphics,
            .queueCount       = 1,
            .pQueuePriorities = &priority
        };

        VkDeviceCreateInfo device_create_info = {
            .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext                   = NULL,
            .queueCreateInfoCount    = 1,
            .pQueueCreateInfos       = &queue_create_info,
            .enabledExtensionCount   = extensions_count,
            .ppEnabledExtensionNames = extensions,
            .pEnabledFeatures        = &device_features
        };

        VkResult result = vkCreateDevice(phys_device,
                                &device_create_info,
                                NULL,
                                device);

        if (result != VK_SUCCESS) {
            printf("Cannot create vkdevice. %d", result);
            return RENDER_INTERNAL_ERROR;
        }
    } else {
        VkDeviceQueueCreateInfo queue_create_info[2] = {};
        float priority = 1.0f;
        queue_create_info[0] = {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queues.graphics,
            .queueCount       = 1,
            .pQueuePriorities = &priority
        };

        queue_create_info[1] = {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queues.present,
            .queueCount       = 1,
            .pQueuePriorities = &priority
        };

        VkDeviceCreateInfo device_create_info = {
            .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext                   = NULL,
            .queueCreateInfoCount    = 2,
            .pQueueCreateInfos       = queue_create_info,
            .enabledExtensionCount   = extensions_count,
            .ppEnabledExtensionNames = extensions,
            .pEnabledFeatures        = &device_features
        };

        VkResult result = vkCreateDevice(phys_device,
                                &device_create_info,
                                NULL,
                                device);

        if (result != VK_SUCCESS) {
            printf("Cannot create vkdevice. %d", result);
            return RENDER_INTERNAL_ERROR;
        }
    }

    return RENDER_OK;
}

static RenderError select_surface_format(VkPhysicalDevice phys_device, VkSurfaceKHR surface, VkFormat target_format, VkColorSpaceKHR target_color_space, VkSurfaceFormatKHR* surface_format) {
    u32 surface_format_count;
    
    vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device,
                                         surface,
                                         &surface_format_count,
                                         NULL);

    if (surface_format_count == 0) {
        printf("Surface formats count is zero\n");
        return RENDER_INTERNAL_ERROR;
    }

    VkSurfaceFormatKHR* surface_formats = Calloc(VkSurfaceFormatKHR, surface_format_count);

    vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device,
                                         surface,
                                         &surface_format_count,
                                         surface_formats);
    
    for (u32 i = 0; i < surface_format_count; i++) {
        VkSurfaceFormatKHR format = surface_formats[i];

        if (format.format     == target_format  &&
            format.colorSpace == target_color_space) {
            *surface_format = format;
            break;
        }
    }

    return RENDER_OK;
}

static RenderError select_present_mode(VkPhysicalDevice phys_device, VkSurfaceKHR surface, VkPresentModeKHR target, VkPresentModeKHR fallback, VkPresentModeKHR* present_mode) {
    u32 present_mode_count;

    vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device,
                                              surface,
                                              &present_mode_count,
                                              NULL);
    
    if (present_mode_count == 0) {
        printf("Present modes count is zero\n");
        return RENDER_INTERNAL_ERROR;
    }
    
    VkPresentModeKHR* present_modes = Calloc(VkPresentModeKHR, present_mode_count);

    vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device,
                                              surface,
                                              &present_mode_count,
                                              present_modes);

    bool target_found = false;

    for (u32 i = 0; i < present_mode_count; i++) {
        VkPresentModeKHR mode = present_modes[i];

        if (mode == target) {
            *present_mode = mode;
            target_found  = true;
            // printf("Target present mode FOUND\n");
            break;
        }
    }

    if (!target_found) {
        for (u32 i = 0; i < present_mode_count; i++) {
            VkPresentModeKHR mode = present_modes[i];

            if (mode == fallback) {
                *present_mode = mode;
                // printf("Fallback present mode FOUND\n");
                break;
            }
        }
    }

    return RENDER_OK;
}

static RenderError create_swapchain(VkSurfaceCapabilitiesKHR surface_capabilities, VkSurfaceFormatKHR surface_format, VkPresentModeKHR present_mode) {
    u32 min_image_count = surface_capabilities.minImageCount + 1;

    if (min_image_count > surface_capabilities.maxImageCount) {
        min_image_count = surface_capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_info = {
        .sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext                  = NULL,
        .flags                  = 0,
        .surface                = App.surface,
        .minImageCount          = min_image_count,
        .imageFormat            = surface_format.format,
        .imageColorSpace        = surface_format.colorSpace,
        .imageExtent            = App.extent,
        .imageArrayLayers       = 1,
        .imageUsage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode       = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount  = 0,
        .pQueueFamilyIndices    = NULL,
        .preTransform           = surface_capabilities.currentTransform,
        .compositeAlpha         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode            = present_mode,
        .clipped                = VK_TRUE,
        .oldSwapchain           = VK_NULL_HANDLE
    };
    
    u32 queue_family_indices[2] {App.queues.graphics, App.queues.present};

    if (App.queues.graphics != App.queues.present) {
        swapchain_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount = 2;
        swapchain_info.pQueueFamilyIndices   = queue_family_indices;
    }

    VkResult result = vkCreateSwapchainKHR(App.device,
                                           &swapchain_info,
                                           NULL,
                                           &App.swapchain);

    if (result != VK_SUCCESS) {
        printf("Cannot create swapchain. %i\n", result);
        return RENDER_INTERNAL_ERROR;
    }

    return RENDER_OK;
}

static void create_descriptor_set_layout(VkDescriptorSetLayoutBinding* bindings, u32 bindings_count, VkDescriptorSetLayout* dsl) {
    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = bindings_count,
        .pBindings    = bindings
    };

    vkCreateDescriptorSetLayout(App.device, &layout_info, NULL, dsl);
}

VkDescriptorPool get_descriptor_pool(VkDescriptorType type, u32 frame_index) {
    if (hash_table_contains(&App.pool_table[frame_index], type)) {
        return App.pool_table[frame_index][type];
    } else {
        VkDescriptorPool pool;

        VkDescriptorPoolSize pool_sizes = {
            .type = type,
            .descriptorCount = POOL_SIZE,
        };

        VkDescriptorPoolCreateInfo info = {
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags         = 0,
            .maxSets       = POOL_SIZE,
            .poolSizeCount = 1,
            .pPoolSizes    = &pool_sizes,
        };

        vkCreateDescriptorPool(App.device, &info, NULL, &pool);

        hash_table_add(&App.pool_table[frame_index], type, pool);

        return pool;
    }
}

Material* material_make(Shader* shader, RenderError* err) {
    u32 mat_index = 0;
    
    Material* material = list_append_empty(&App.materials, &mat_index);
    material->shader    = shader;
    for (u32 j = 0; j < MAX_FRAMES; j++) {
        material->pools[j] = Calloc(VkDescriptorPool, shader->set_count);
        material->sets[j]  = Calloc(VkDescriptorSet, shader->set_count);

        for(u32 i = 0; i < shader->set_count; i++) {
            VkDescriptorPool pool = get_descriptor_pool(shader->data_sets[i].type, j);

            material->pools[j][i] = pool;

            VkDescriptorSetAllocateInfo info = {
                .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool     = pool,
                .descriptorSetCount = 1,
                .pSetLayouts        = &shader->data_sets[i].layout
            };

            VkResult res = vkAllocateDescriptorSets(App.device, &info, &material->sets[j][i]);

            if (res != VK_SUCCESS) {
                printf("Cannot allocate descriptor set for material. Vulkan error: %d.", res);
                *err = RENDER_INTERNAL_ERROR;
                return NULL;
            }
        }
    }

    for (u32 i = 0; i < MAX_FRAMES; i++) {
        material->uniform_buffers[i] = Calloc(u32, shader->set_count);

        for (u32 j = 0; j < shader->set_count; j++) {
            ShaderDataSet descr = shader->data_sets[j];
            u32 buffer_index = 0;
            GpuBuffer* buffer = buffer_allocate(descr.buffer_size, descr.usage, descr.mem_flags, err, &buffer_index);

            if (*err != RENDER_OK) {
                return NULL;
            }

            material->uniform_buffers[i][j] = buffer_index;

            VkDescriptorBufferInfo buffer_info = {
                .buffer = buffer->buffer,
                .offset = 0,
                .range  = descr.buffer_size,
            };

            VkWriteDescriptorSet descriptor_writes[] = {
            {
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = material->sets[i][j],
                .dstBinding      = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = descr.type,
                .pBufferInfo     = &buffer_info,
            },
            };

            vkUpdateDescriptorSets(App.device, 1, descriptor_writes, 0, NULL);
        }
    }

    *err = RENDER_OK;

    return material;
}

void material_destroy(Material* material) {
    for (u32 i = 0; i < MAX_FRAMES; i++) {
        for (u32 j = 0; j < material->shader->set_count; j++) {
            // buffer_free(&material->uniform_buffers[i][j]);
            vkDestroyDescriptorPool(App.device, material->pools[i][j], NULL);
        }
    }
}

RenderError render_pass_make(VkAttachmentDescription* attachments, u32 attachment_count, VkSubpassDescription* subpasses, u32 subpass_count, RenderPass* pass) {
    pass->attachments      = Calloc(VkAttachmentDescription, attachment_count);
    pass->subpasses        = Calloc(VkSubpassDescription, subpass_count);
    pass->attachment_count = attachment_count;
    pass->subpass_count    = subpass_count;

    memcpy(pass->attachments, attachments, sizeof(VkAttachmentDescription) * attachment_count);
    memcpy(pass->subpasses, subpasses, sizeof(VkSubpassDescription) * subpass_count);

    VkRenderPassCreateInfo pass_info = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext           = NULL,
        .flags           = 0,
        .attachmentCount = attachment_count,
        .pAttachments    = pass->attachments,
        .subpassCount    = subpass_count,
        .pSubpasses      = pass->subpasses,
        .dependencyCount = 0,
        .pDependencies   = NULL
    };

    VkResult result = vkCreateRenderPass(App.device,
                                         &pass_info,
                                         NULL,
                                         &pass->pass);

    if (result != VK_SUCCESS) {
        printf("Cannot create render pass. Vulkan error: %d.\n", result);
        Free(pass->attachments);
        Free(pass->subpasses);
        return RENDER_INTERNAL_ERROR;
    }

    return RENDER_OK;
}

u32 render_pass_get_or_create(VkAttachmentDescription* attachments, u32 attachment_count, VkSubpassDescription* subpasses, u32 subpass_count, RenderPass* pass) {
    RenderPass descr = {
        .pass             = VK_NULL_HANDLE,
        .attachments      = attachments,
        .attachment_count = attachment_count,
        .subpasses        = subpasses,
        .subpass_count    = subpass_count,
    };

    auto render_passes_match = [descr](RenderPass p) {
        if (p.pass == VK_NULL_HANDLE)                     return false;
        if (p.attachment_count != descr.attachment_count) return false;
        if (p.subpass_count != descr.subpass_count)       return false;

        if (p.attachments == NULL) return false;
        if (p.subpasses == NULL)   return false;

        if (memcmp(p.attachments, descr.attachments, sizeof(VkAttachmentDescription) * p.attachment_count) != 0) return false;

        return true;
    };

    u32 pass_index = 0;

    if (list_contains(&App.render_passes, render_passes_match, &pass_index)) {
        *pass = App.render_passes[pass_index];
        return pass_index;
    }

    RenderError err = render_pass_make(attachments, attachment_count, subpasses, subpass_count, pass);

    if (err != RENDER_OK) {
        return u32_max;
    }

    pass_index = list_append(&App.render_passes, *pass);

    return pass_index;
}

RenderPass* render_pass_get(u32 index) {
    Assert(index < App.render_passes.count, "Render pass index is out of range.\n");

    return &App.render_passes[index];
}

void render_pass_destroy(RenderPass* pass) {
    vkDestroyRenderPass(App.device, pass->pass, NULL);
}

#if defined(WIN32)
#define INSTANCE_EXTENSIONS_COUNT 2
const char* INSTANCE_EXTENSIONS[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
};

GlassErrorCode glass_create_vulkan_surface(Window* window, VkInstance vk_instance, VkSurfaceKHR* surface) {
    VkWin32SurfaceCreateInfoKHR surface_info = {
        .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext     = NULL,
        .hinstance = glass_win32_get_instance(window),
        .hwnd      = glass_win32_get_window_handle(window),
    };

    VkResult result = vkCreateWin32SurfaceKHR(vk_instance,
                                     &surface_info,
                                     NULL,
                                     surface);
    
    if (result != VK_SUCCESS) {
        printf("Cannot create vulkan surface. Vulkan error: %d.\n", result);
        return GLASS_INTERNAL_ERROR;
    }

    return GLASS_OK;
}

const char** glass_get_vulkan_instance_extensions(u32* count) {
    *count = INSTANCE_EXTENSIONS_COUNT;
    return INSTANCE_EXTENSIONS;
}
#endif