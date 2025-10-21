#define DVK_USE_PLATFORM_WIN32_KHR
#define Vulkan

#include <vulkan/vulkan.h>
#include "glass.h"
#include <cstdio>
#include "basic.h"
#include <cstring>
#define GAME_MATH_IMPLEMENTATION
#include "Vector2.h"
#include "Vector3.h"
#include "Matrix4.h"
#include "geometry.h"
#include <time.h>
#include <stdlib.h>

#define VERTEX_BUFFERS_INITIAL_LENGTH 8
#define VERTEX_BUFFERS_STEP           8
#define UBO_ALIGNMENT                 256
#define MAX_FRAMES                    2

typedef struct QueueFamilies {
    bool graphics_supported;
    bool present_supported;
    u32  graphics;
    u32  present;
} QueueFamilies;

typedef struct Camera2D {
    Vector2 position;
    float   size;
    float   left;
    float   right;
    float   top;
    float   bottom;
} Camera2D;

typedef struct Vertex {
    float position[2];
    u8    color[4];
} Vertex;

typedef struct UniformBuffer {
    VkBuffer       buffer;
    VkDeviceMemory mem;
    void*          mapped;
} UniformBuffer;

typedef struct PerFrameData {
    Matrix4 view;
    Matrix4 proj;
    float   time;
    float   dt;
} PerFrameData;

typedef struct Transform2d {
    Vector2 position;
    Vector2 scale;
    float   rotation;
} Transform2d;

typedef struct Matrices {
    Matrix4 model;
    Matrix4 mvp;
} Matrices;

Window*          WINDOW{};
VkInstance       VK_INSTANCE{};
VkSurfaceKHR     VK_SURFACE{};
QueueFamilies    QUEUES{};
VkPhysicalDevice VK_PHYS_DEVICE{};
VkDevice         VK_DEVICE{};
VkSwapchainKHR   VK_SWAPCHAIN{};
VkCommandPool    VK_COMMAND_POOL{};
VkCommandBuffer  VK_COMMAND_BUFFER{};
VkRenderPass     VK_RENDER_PASS{};
VkFramebuffer*   VK_FRAME_BUFFERS{};
VkExtent2D       VK_EXTENT{};
VkPipeline       VK_PIPELINE{};
VkPipelineLayout VK_PIPELINE_LAYOUT{};
VkQueue          VK_GRAPHICS_QUEUE{};
VkQueue          VK_PRESENT_QUEUE{};
VkViewport       VK_VIEWPORT{};
VkRect2D         VK_SCISSORS{};
VkBuffer         VERTEX_BUFFER{};
VkDeviceMemory   VERTEX_BUFFER_MEMORY{};
VkBuffer         INDEX_BUFFER{};
VkDeviceMemory   INDEX_BUFFER_MEMORY{};
VkDescriptorSetLayout PER_FRAME_SET_LAYOUT{};
VkDescriptorSetLayout MATRIX_SET_LAYOUT{};
UniformBuffer         PER_FRAME_UBO[MAX_FRAMES];
VkDescriptorPool      DESCRIPTOR_POOL[MAX_FRAMES];
VkDescriptorSet       DESCRIPTOR_SET[MAX_FRAMES];
VkDescriptorPool      MATRIX_DESCRIPTOR_POOL;
VkDescriptorSet       MATRIX_DESCRIPTOR_SET;
VkShaderModule        VERT{};
VkShaderModule        FRAG{};

Transform2d*   TRANSFORMS;
Matrices*      MATRICES;
VkBuffer       MATRIX_BUFFER;
VkDeviceMemory MATRIX_MEMORY;
u32            TRANSFORMS_COUNT  = 0;
u32            TRANSFORMS_LENGTH = 0;

VkFence     VK_SINGLE_FRAME_FENCE;
VkSemaphore VK_SEMAPHORE_IMAGE_READY;
VkSemaphore VK_SEMAPHORE_RENDER_FINISHED[MAX_FRAMES];

Matrix4 VIEW;
Matrix4 PROJECTION;

Camera2D CAMERA = {};

double TIME_DOUBLE = 0.0l;
float  TIME        = 0.0f;
float  DT          = 0.0f;

void print_instance_extensions();

static inline float frand01() {
    return (float)rand() / RAND_MAX;
}

static inline float frand(float min, float max) {
    float t = frand01();
    return (1.0f - t) * min + t * max;
}

static inline void matrix4_mvp(Camera2D* camera, Vector2 position, float rotation, Vector2 scale, Matrix4* mat);

VkResult create_vk_instance(const char** extensions, u32 extensions_count);
int      get_phys_device();
int      select_queues(QueueFamilies* queues);
int      create_logical_device(const char** extensions, u32 extensions_count);
int      select_surface_format(VkFormat target_format, VkColorSpaceKHR target_color_space, VkSurfaceFormatKHR* surface_format);
int      select_present_mode(VkPresentModeKHR target, VkPresentModeKHR fallback, VkPresentModeKHR* present_mode);
int      create_swapchain(VkSurfaceCapabilitiesKHR surface_capabilities, VkSurfaceFormatKHR surface_format, VkPresentModeKHR present_mode);
int      create_render_pipeline(VkSurfaceFormatKHR surface_format);


void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* memory);
void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
void set_memory(VkDeviceMemory memory, VkDeviceSize size, void* src);
void destroy_buffer(VkBuffer buffer, VkDeviceMemory memory);
bool create_shader_module(VkDevice device, const char* name, VkShaderModule* shader_module);
void create_vertex_buffer(Vertex* vertices, u32 vertex_count, VkBuffer* buffer, VkDeviceMemory* memory);
void create_index_buffer(u16* indices, u32 index_count, VkBuffer* buffer, VkDeviceMemory* memory);
void create_descriptor_set_layout(VkDescriptorSetLayoutBinding* bindings, u32 bindings_count, VkDescriptorSetLayout* dsl);
void create_uniform_buffer(UniformBuffer* ub, u32 size);
void create_descriptor_set(VkDescriptorSet* descriptor_set, u32 image_index);
void update_per_frame_data(UniformBuffer* ubo, PerFrameData* data);
void camera2d_make(Vector2 position, float size, Camera2D* cam);
void camera2d_update(Camera2D* cam);
void camera2d_move(Camera2D* cam, Vector2 dir);

void transform2d_to_model(Transform2d* transform, Matrix4* model);
void transforms_init(u32 initial_size);
u32  transforms_append(Transform2d transform);
void transforms_set(u32 index, Transform2d* transform);

static inline void update_matrices(Transform2d* transforms, Matrices* matrices, u32 count, VkDeviceMemory mem);

int main(int argc, char** argv) {
    srand(time(NULL));
    const char* name         = "Hello";

    GlassErrorCode err = glass_create_window(100, 100, 1280, 720, name, &WINDOW);
    if (err != GLASS_OK) {
        printf("Cannot create window. %d\n", err);
        return 1;
    }


    u32 instance_extensions_count;
    const char** instance_extensions = glass_get_vulkan_instance_extensions(&instance_extensions_count);

    VkResult result = create_vk_instance(instance_extensions, instance_extensions_count);

    if (result != VK_SUCCESS) {
        printf("Cannot create vk instance. %d\n", result);
        return 1;
    }


    // get physical device
    int res = get_phys_device();

    if (res != 0) {
        printf("Cannot get physical device.\n");
        return 1;
    }

    err = glass_create_vulkan_surface(WINDOW, VK_INSTANCE, &VK_SURFACE);

    if (err != GLASS_OK) {
        printf("Cannot create win32 surface. %i\n", err);
        return 1;
    }

    res = select_queues(&QUEUES);

    if (res != 0) {
        printf("Cannot select queue families. %d\n", res);
        return 1;
    }

    // create logical device
    u32   enabled_extensions_count = 1;
    const char* extension_names[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    res = create_logical_device(extension_names, enabled_extensions_count);

    if (res != 0) {
        printf("Cannot create logical device. %d\n", res);
        return 1;
    }

    // select surface format and present mode
    VkSurfaceCapabilitiesKHR surface_capabilities;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VK_PHYS_DEVICE,
                                              VK_SURFACE,
                                              &surface_capabilities);

    VK_EXTENT = surface_capabilities.currentExtent;
    auto max_extent = surface_capabilities.maxImageExtent;
    auto min_extent = surface_capabilities.minImageExtent;

    printf("Current extent: x:%u, y:%u\n", VK_EXTENT.width, VK_EXTENT.height);
    printf("Max extent: x:%u, y:%u\n", max_extent.width, max_extent.height);
    printf("Min extent: x:%u, y:%u\n", min_extent.width, min_extent.height);
    printf("Min image count: %u\n", surface_capabilities.minImageCount);
    printf("Max image count: %u\n", surface_capabilities.maxImageCount);
    printf("Max image array layers: %u\n", surface_capabilities.maxImageArrayLayers);

    VkSurfaceFormatKHR surface_format{};

    res = select_surface_format(VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, &surface_format);

    if (res != 0) {
        printf("Cannot select surface format. %d\n", res);
        return 1;
    }
    
    VkPresentModeKHR present_mode;

    res = select_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR, &present_mode);

    if (res != 0) {
        printf("Cannot select present mode. %d\n", res);
        return 1;
    }

    printf("Present mode: %i\n", present_mode);

    res = create_swapchain(surface_capabilities, surface_format, present_mode);

    if (res != 0) {
        return 1;
    }

    printf("Swapchain created.\n");

    res = create_render_pipeline(surface_format);

    if (res != 0) {
        printf("Cannot create render pipeline. %d\n", res);
        return 1;
    }

    printf("Pipeline created.\n");

    // get image views
    u32 image_count;

    vkGetSwapchainImagesKHR(VK_DEVICE, 
                            VK_SWAPCHAIN,
                            &image_count,
                            NULL);
    
    VkImage* images = Calloc(VkImage, image_count);

    result = vkGetSwapchainImagesKHR(VK_DEVICE,
                                     VK_SWAPCHAIN,
                                     &image_count,
                                     images);

    if (result != VK_SUCCESS) {
        printf("Cannot get swapchain images. %d\n", result);
        return 1;
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

        result = vkCreateImageView(VK_DEVICE, 
                                   &image_view_info,
                                   NULL,
                                   &image_view);
        
        if (result != VK_SUCCESS) {
            printf("Cannot create image view for image %d. %d\n", i, result);
            return 1;
        }
        
        image_views[i] = image_view;
    }

    printf("Image views created.\n");

    // create frame buffers
    VK_FRAME_BUFFERS = Calloc(VkFramebuffer, image_count);

    for (u32 i = 0; i < image_count; i++) {
        VkFramebuffer frame_buffer;

        VkFramebufferCreateInfo frame_buffer_info = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext           = NULL,
            .flags           = 0,
            .renderPass      = VK_RENDER_PASS,
            .attachmentCount = 1,
            .pAttachments    = &image_views[i],
            .width           = VK_EXTENT.width,
            .height          = VK_EXTENT.height,
            .layers          = 1
        };

        result = vkCreateFramebuffer(VK_DEVICE,
                                    &frame_buffer_info,
                                    NULL,
                                    &frame_buffer);

        if (result != VK_SUCCESS) {
            printf("Cannot create frame buffer. %d\n", result);
            return 1;
        }

        VK_FRAME_BUFFERS[i] = frame_buffer;
    }

    printf("Frame buffers created.\n");

    // create command pool
    VkCommandPoolCreateInfo command_pool_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = NULL,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = QUEUES.graphics
    };

    result = vkCreateCommandPool(VK_DEVICE,
                                 &command_pool_info,
                                 NULL,
                                 &VK_COMMAND_POOL);
                                
    if (result != VK_SUCCESS) {
        printf("Cannot create command bool. %d\n", result);
        return 1;
    }

    VkCommandBufferAllocateInfo command_buffer_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = NULL,
        .commandPool        = VK_COMMAND_POOL,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    result = vkAllocateCommandBuffers(VK_DEVICE,
                                      &command_buffer_info,
                                      &VK_COMMAND_BUFFER);

    if (result != VK_SUCCESS) {
        printf("Cannot allocate command buffer. %d\n", result);
        return 1;
    }

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

    vkCreateFence    (VK_DEVICE, &single_frame_fence_info,        NULL, &VK_SINGLE_FRAME_FENCE);
    vkCreateSemaphore(VK_DEVICE, &image_ready_semaphore_info,     NULL, &VK_SEMAPHORE_IMAGE_READY);
    
    for (u32 i = 0; i < MAX_FRAMES; i++) {
        vkCreateSemaphore(VK_DEVICE, &render_finished_semaphore_info, NULL, &VK_SEMAPHORE_RENDER_FINISHED[i]);
    }

    vkGetDeviceQueue(VK_DEVICE,
                     QUEUES.graphics,
                     0,
                     &VK_GRAPHICS_QUEUE);

    vkGetDeviceQueue(VK_DEVICE,
                     QUEUES.present,
                     0,
                     &VK_PRESENT_QUEUE);

    Vertex vertices[] = {
        {{   0.5f,  -0.5f }, { 255, 0,   0,   255 }},
        {{   0.5f,   0.5f }, { 0,   255, 0,   255 }},
        {{  -0.5f,   0.5f }, { 0,   0,   255, 255 }},
        {{  -0.5f,  -0.5f }, { 255, 255, 255, 255 }},
    };

    u16 indices[] = {
        0, 1, 2, 0, 2, 3
    };

    create_vertex_buffer(vertices, 4, &VERTEX_BUFFER, &VERTEX_BUFFER_MEMORY);
    create_index_buffer(indices, 6, &INDEX_BUFFER, &INDEX_BUFFER_MEMORY);

    for (u32 i = 0; i < MAX_FRAMES; i++) {
        create_uniform_buffer(&PER_FRAME_UBO[i], sizeof(PerFrameData));
    }

    transforms_init(32);

    for (u32 i = 0; i < MAX_FRAMES; i++) {
        create_descriptor_set(&DESCRIPTOR_SET[i], i);
    }

    VkDescriptorPoolSize pool_sizes[] = {
        {
            .type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1
        },
    };

    VkDescriptorPoolCreateInfo pool_info = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets       = 1,
        .poolSizeCount = 1,
        .pPoolSizes    = pool_sizes,
    };

    vkCreateDescriptorPool(VK_DEVICE, &pool_info, NULL, &MATRIX_DESCRIPTOR_POOL);

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = MATRIX_DESCRIPTOR_POOL,
        .descriptorSetCount = 1,
        .pSetLayouts        = &MATRIX_SET_LAYOUT,
    };

    vkAllocateDescriptorSets(VK_DEVICE, &alloc_info, &MATRIX_DESCRIPTOR_SET);

    // Update descriptor set
    VkDescriptorBufferInfo buffer_info = {
        .buffer = MATRIX_BUFFER,
        .offset = 0,
        .range  = sizeof(Matrices) * TRANSFORMS_LENGTH,
    };

    VkWriteDescriptorSet descriptor_writes[] = {
    {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = MATRIX_DESCRIPTOR_SET,
        .dstBinding      = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo     = &buffer_info,
    },
    };

    vkUpdateDescriptorSets(VK_DEVICE, 1, descriptor_writes, 0, NULL);

    camera2d_make(vector2_make(0, 0), 4, &CAMERA);
    camera2d_update(&CAMERA);

    Transform2d transform;

    transform.rotation = 0;
    transform.position = {{0, 0}};
    transform.scale    = {{1, 1}};

    const float rotation_speed = 10.0f;
    float speed                = 2.0f;
    u32 tindex = transforms_append(transform);

    double last_time = glass_get_time();

    while (true) {
        if (glass_is_button_pressed(GLASS_SCANCODE_ESCAPE)) {
            glass_exit();
            break;
        }

        if (glass_exit_required())
            break;
        
        double current_time = glass_get_time();
        double actual_dt    = current_time - last_time;
               TIME_DOUBLE  += actual_dt;
        int    fps          = (int)(1.0l / actual_dt);
        last_time = current_time;
        char buf[128];
        float dt   = (float)actual_dt;
              DT   = dt;
              TIME = float(TIME_DOUBLE);
        sprintf(buf, "fps: %i, dt:%f, time:%f", fps, dt, TIME);
        glass_set_window_title(WINDOW, buf);

        transform.rotation += rotation_speed * dt;

        if (glass_is_button_pressed(GLASS_SCANCODE_W)) {
            transform.position.y += speed * dt;
        } else if (glass_is_button_pressed(GLASS_SCANCODE_S)) {
            transform.position.y -= speed * dt;
        }

        if (glass_is_button_pressed(GLASS_SCANCODE_A)) {
            transform.position.x -= speed * dt;
        } else if (glass_is_button_pressed(GLASS_SCANCODE_D)) {
            transform.position.x += speed * dt;
        }

        float camera_speed = 2.0f;
        Vector2 camera_move = {{0, 0}};

        if (glass_is_button_pressed(GLASS_SCANCODE_UP)) {
            camera_move.y += camera_speed * dt;
        } else if (glass_is_button_pressed(GLASS_SCANCODE_DOWN)) {
            camera_move.y -= camera_speed * dt;
        }

        if (glass_is_button_pressed(GLASS_SCANCODE_LEFT)) {
            camera_move.x -= camera_speed * dt;
        } else if (glass_is_button_pressed(GLASS_SCANCODE_RIGHT)) {
            camera_move.x += camera_speed * dt;
        }

        if (glass_is_button_pressed(GLASS_SCANCODE_SPACE)) {
            Transform2d trans = {
                .position = {{frand(-2, 2), frand(-2, 2)}},
                .scale    = {{1, 1}},
                .rotation = 45.0f,
            };

            transforms_append(trans);
        }

        printf("Transforms count: %d\n", TRANSFORMS_COUNT);

        camera2d_move(&CAMERA, camera_move);

        transforms_set(tindex, &transform);
        update_matrices(TRANSFORMS, MATRICES, TRANSFORMS_COUNT, MATRIX_MEMORY);
        glass_main_loop();
    }

    printf("Exit begin.\n");

    vkDeviceWaitIdle(VK_DEVICE);

    destroy_buffer(VERTEX_BUFFER, VERTEX_BUFFER_MEMORY);
    destroy_buffer(INDEX_BUFFER, INDEX_BUFFER_MEMORY);
    destroy_buffer(MATRIX_BUFFER, MATRIX_MEMORY);

    for (u32 i = 0; i < MAX_FRAMES; i++) {
        vkDestroyDescriptorPool(VK_DEVICE, DESCRIPTOR_POOL[i], NULL);
        vkDestroyBuffer(VK_DEVICE, PER_FRAME_UBO[i].buffer, NULL);
        vkFreeMemory(VK_DEVICE, PER_FRAME_UBO[i].mem, NULL);
    }

    vkDestroyDescriptorPool(VK_DEVICE, MATRIX_DESCRIPTOR_POOL, NULL);

    vkDestroyDescriptorSetLayout(VK_DEVICE, PER_FRAME_SET_LAYOUT, NULL);
    vkDestroyDescriptorSetLayout(VK_DEVICE, MATRIX_SET_LAYOUT, NULL);
    
    vkDestroySemaphore(VK_DEVICE, VK_SEMAPHORE_IMAGE_READY, NULL);
    vkDestroyFence(VK_DEVICE, VK_SINGLE_FRAME_FENCE, NULL);
    for (u32 i = 0; i < MAX_FRAMES; i++) {
        vkDestroySemaphore(VK_DEVICE, VK_SEMAPHORE_RENDER_FINISHED[i], NULL);
    }

    vkDestroyCommandPool(VK_DEVICE, VK_COMMAND_POOL, NULL);

    for (u32 i = 0; i < image_count; i++) {
        vkDestroyFramebuffer(VK_DEVICE, VK_FRAME_BUFFERS[i], NULL);
        vkDestroyImageView(VK_DEVICE, image_views[i], NULL);
    }

    vkDestroyPipeline(VK_DEVICE, VK_PIPELINE, NULL);
    vkDestroyPipelineLayout(VK_DEVICE, VK_PIPELINE_LAYOUT, NULL);
    vkDestroyRenderPass(VK_DEVICE, VK_RENDER_PASS, NULL);
    vkDestroyShaderModule(VK_DEVICE, VERT, NULL);
    vkDestroyShaderModule(VK_DEVICE, FRAG, NULL);
    vkDestroySwapchainKHR(VK_DEVICE, VK_SWAPCHAIN, NULL);
    vkDestroyDevice(VK_DEVICE, NULL);
    vkDestroySurfaceKHR(VK_INSTANCE,
                        VK_SURFACE,
                        null);
    vkDestroyInstance(VK_INSTANCE, null);

    glass_destroy_window(WINDOW);
    return 0;
}

GlassErrorCode glass_render() {
    vkWaitForFences(VK_DEVICE, 1, &VK_SINGLE_FRAME_FENCE, VK_TRUE, u64_max);
    vkResetFences(VK_DEVICE, 1, &VK_SINGLE_FRAME_FENCE);

    u32 image_index;
    vkAcquireNextImageKHR(VK_DEVICE, 
                            VK_SWAPCHAIN, 
                            u64_max,
                            VK_SEMAPHORE_IMAGE_READY, 
                            VK_NULL_HANDLE, 
                            &image_index);
    
    vkResetCommandBuffer(VK_COMMAND_BUFFER, 0);

    PerFrameData per_frame_data = {
        .view = VIEW,
        .proj = PROJECTION,
        .time = TIME,
        .dt   = DT
    };

    update_per_frame_data(&PER_FRAME_UBO[image_index], &per_frame_data);
    
    // record command buffer
    VkCommandBufferBeginInfo begin_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = NULL,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL
    };

    vkBeginCommandBuffer(VK_COMMAND_BUFFER, &begin_info);

    VkClearValue clear_color = {
        .color = {{0, 0, 0, 0}}
    };

    VkRenderPassBeginInfo render_pass_begin_info = {
        .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext       = NULL,
        .renderPass  = VK_RENDER_PASS,
        .framebuffer = VK_FRAME_BUFFERS[image_index],
        .renderArea  = {
            .offset = {0, 0},
            .extent = VK_EXTENT
        },
        .clearValueCount = 1,
        .pClearValues    = &clear_color
    };

    vkCmdBeginRenderPass(VK_COMMAND_BUFFER, 
                            &render_pass_begin_info, 
                            VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(VK_COMMAND_BUFFER, VK_PIPELINE_BIND_POINT_GRAPHICS, VK_PIPELINE);
    vkCmdSetViewport(VK_COMMAND_BUFFER, 0, 1, &VK_VIEWPORT);
    vkCmdSetScissor(VK_COMMAND_BUFFER, 0, 1, &VK_SCISSORS);

    // printf("Binding %d buffers\n", VERTEX_BUFFERS_COUNT);
    VkDescriptorSet descriptor_sets[] = {DESCRIPTOR_SET[image_index], MATRIX_DESCRIPTOR_SET};
    vkCmdBindDescriptorSets(VK_COMMAND_BUFFER, VK_PIPELINE_BIND_POINT_GRAPHICS, VK_PIPELINE_LAYOUT, 0, 2, descriptor_sets, 0, NULL);

    const VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(VK_COMMAND_BUFFER, 0, 1, &VERTEX_BUFFER, offsets);
    vkCmdBindIndexBuffer(VK_COMMAND_BUFFER, INDEX_BUFFER, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(VK_COMMAND_BUFFER, 6, TRANSFORMS_COUNT, 0, 0, 0);
    
    vkCmdEndRenderPass(VK_COMMAND_BUFFER);
    auto result = vkEndCommandBuffer(VK_COMMAND_BUFFER);

    if (result != VK_SUCCESS) {
        printf("Cannot create command buffer. %d\n", result);
        return GLASS_RENDER_ERROR;
    }

    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit_info = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext                = NULL,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &VK_SEMAPHORE_IMAGE_READY,
        .pWaitDstStageMask    = wait_stages,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &VK_COMMAND_BUFFER,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &VK_SEMAPHORE_RENDER_FINISHED[image_index]
    };

    result = vkQueueSubmit(VK_GRAPHICS_QUEUE, 1, &submit_info, VK_SINGLE_FRAME_FENCE);

    if (result != VK_SUCCESS) {
        printf("Cannot submit queue. %d\n", result);
        return GLASS_RENDER_ERROR;
    }

    VkPresentInfoKHR present_info = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &VK_SEMAPHORE_RENDER_FINISHED[image_index],
        .swapchainCount     = 1,
        .pSwapchains        = &VK_SWAPCHAIN,
        .pImageIndices      = &image_index
    };

    result = vkQueuePresentKHR(VK_PRESENT_QUEUE, &present_info);

    if (result != VK_SUCCESS) {
        printf("Cannot draw. %d\n", result);
        return GLASS_RENDER_ERROR;
    }

    return GLASS_OK;
}

GlassErrorCode glass_on_resize(u32 width, u32 height) {
    return GLASS_OK;
}

void print_instance_extensions() {
    u32 ext_count {};

    vkEnumerateInstanceExtensionProperties(null, &ext_count, null);

    printf("extensions count: %i\n", ext_count);

    VkExtensionProperties* extensions;
    extensions = Calloc(VkExtensionProperties, ext_count);

    vkEnumerateInstanceExtensionProperties(null, &ext_count, extensions);

    for (u32 i = 0; i < ext_count; i++) {
        printf("%s\n", extensions[i].extensionName);
    }
}

bool create_shader_module(VkDevice device, const char* name, VkShaderModule* shader_module) {
    u32  size;
    u32* code;
    FILE* file = fopen(name, "rb");

    if (!file) {
        printf("Cannot open the file %s", name);
        return false;
    }

    fseek(file, 0, SEEK_END);
    size = ftell(file);

    if (size == 0) {
        fclose(file);
        return false;
    }

    fseek(file, 0, SEEK_SET);

    char* data = Malloc(char, size);

    if (!data) {
        fclose(file);
        return false;
    }

    fread(data, 1, size, file);
    fclose(file);

    code = (u32*)data;

    VkShaderModule module = {};

    VkShaderModuleCreateInfo info = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = NULL,
        .flags    = 0,
        .codeSize = size,
        .pCode    = code
    };

    VkResult result = vkCreateShaderModule(device,
                                           &info,
                                           NULL,
                                           &module);

    if (result != VK_SUCCESS) {
        printf("Cannot create shader module %s. %d", name, result);
        return false;
    }

    *shader_module = module;

    return true;
}

void create_vertex_buffer(Vertex* vertices, u32 vertex_count, VkBuffer* buffer, VkDeviceMemory* memory) {
    u64 size = sizeof(vertices[0]) * vertex_count;

    VkBuffer       staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(size, 
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &staging_buffer,
                  &staging_buffer_memory);
    
    void* data;
    vkMapMemory(VK_DEVICE, staging_buffer_memory, 0, size, 0, &data);
    memcpy(data, vertices, size);
    vkUnmapMemory(VK_DEVICE, staging_buffer_memory);

    create_buffer(size, 
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  buffer,
                  memory);

    copy_buffer(staging_buffer, *buffer, size);

    vkDestroyBuffer(VK_DEVICE, staging_buffer, NULL);
    vkFreeMemory(VK_DEVICE, staging_buffer_memory, NULL);
}

void create_descriptor_set_layout(VkDescriptorSetLayoutBinding* bindings, u32 bindings_count, VkDescriptorSetLayout* dsl) {
    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = bindings_count,
        .pBindings    = bindings
    };

    vkCreateDescriptorSetLayout(VK_DEVICE, &layout_info, NULL, dsl);
}

void create_uniform_buffer(UniformBuffer* ub, u32 size) {
    VkBufferCreateInfo buffer_info = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = size,
        .usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    vkCreateBuffer(VK_DEVICE, &buffer_info, NULL, &ub->buffer);

    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(VK_DEVICE, ub->buffer, &mem_req);

    VkMemoryAllocateInfo alloc_info = {
        .sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_req.size
    };
    
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(VK_PHYS_DEVICE, &mem_props);
    
    uint32_t mem_type_index = -1;
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((mem_req.memoryTypeBits & (1 << i)) && 
            (mem_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
            (mem_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            mem_type_index = i;
            break;
        }
    }
    
    alloc_info.memoryTypeIndex = mem_type_index;
    vkAllocateMemory(VK_DEVICE, &alloc_info, NULL, &ub->mem);
    vkBindBufferMemory(VK_DEVICE, ub->buffer, ub->mem, 0);
    
    vkMapMemory(VK_DEVICE, ub->mem, 0, size, 0, &ub->mapped);
}

void create_descriptor_set(VkDescriptorSet* descriptor_set, u32 image_index) {
    // Create descriptor pool
    VkDescriptorPoolSize pool_sizes[] = {
        {
            .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1
        },
    };

    VkDescriptorPoolCreateInfo pool_info = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets       = 1,
        .poolSizeCount = 1,
        .pPoolSizes    = pool_sizes,
    };

    vkCreateDescriptorPool(VK_DEVICE, &pool_info, NULL, &DESCRIPTOR_POOL[image_index]);

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = DESCRIPTOR_POOL[image_index],
        .descriptorSetCount = 1,
        .pSetLayouts        = &PER_FRAME_SET_LAYOUT,
    };

    vkAllocateDescriptorSets(VK_DEVICE, &alloc_info, descriptor_set);

    // Update descriptor set
    VkDescriptorBufferInfo view_proj_buffer_info = {
        .buffer = PER_FRAME_UBO[image_index].buffer,
        .offset = 0,
        .range  = sizeof(PerFrameData),
    };

    VkWriteDescriptorSet descriptor_writes[] = {
    {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = *descriptor_set,
        .dstBinding      = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo     = &view_proj_buffer_info,
    },
    };

    vkUpdateDescriptorSets(VK_DEVICE, 1, descriptor_writes, 0, NULL);
}

void update_per_frame_data(UniformBuffer* ubo, PerFrameData* data) {
    memcpy(ubo->mapped, data, sizeof(PerFrameData));
}

void camera2d_make(Vector2 position, float size, Camera2D* cam) {
    cam->position = position;
    cam->size     = size;
    cam->left     = position.x - size;
    cam->right    = position.x + size;
    cam->top      = position.y + size;
    cam->bottom   = position.y - size;
}

void camera2d_update(Camera2D* cam) {
    cam->left   = cam->position.x - cam->size;
    cam->right  = cam->position.x + cam->size;
    cam->top    = cam->position.y + cam->size;
    cam->bottom = cam->position.y - cam->size;

    VIEW       = matrix4_transform_2d(-cam->position.x, -cam->position.y);
    PROJECTION = matrix4_ortho_2d(cam->left, cam->right, cam->top, cam->bottom);
}

void camera2d_move(Camera2D* cam, Vector2 dir) {
    cam->position.x += dir.x;
    cam->position.y += dir.y;

    // printf("Camera position: (%f, %f)\n", cam->position.x, cam->position.y);
    camera2d_update(cam);
}

VkResult create_vk_instance(const char** extensions, u32 extensions_count) {
    const char* validation_layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

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
        .enabledLayerCount       = 1,
        .ppEnabledLayerNames     = validation_layers,
        .enabledExtensionCount   = extensions_count,
        .ppEnabledExtensionNames = extensions,
    };

    VkResult result = vkCreateInstance(&vk_info, null, &VK_INSTANCE);

    return result;
}

int get_phys_device() {
    u32               phys_devices_count;
    VkPhysicalDevice* devices;
    vkEnumeratePhysicalDevices(VK_INSTANCE, &phys_devices_count, null);
    devices = Calloc(VkPhysicalDevice, phys_devices_count);
    vkEnumeratePhysicalDevices(VK_INSTANCE, &phys_devices_count, devices);

    printf("GPU count: %i\n", phys_devices_count);

    if (phys_devices_count == 0) {
        return 1;
    }

    for (u32 i = 0; i < phys_devices_count; i++) {
        u32 extensions_count;

        vkEnumerateDeviceExtensionProperties(devices[i],
                                             NULL,
                                             &extensions_count,
                                             NULL);
        
        VkExtensionProperties* extension_properties = Calloc(VkExtensionProperties, extensions_count);

        vkEnumerateDeviceExtensionProperties(devices[i],
                                             NULL,
                                             &extensions_count,
                                             extension_properties);

        bool swapchain_supported = false;

        for (u32 j = 0; j < extensions_count; j++) {
            if (strcmp(extension_properties[j].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
                swapchain_supported = true;
                break;
            }
        }

        Free(extension_properties);

        if (swapchain_supported) {
            VK_PHYS_DEVICE = devices[i];
            return 0;
        }
    }

    return 1;
}

int select_queues(QueueFamilies* queues) {
    u32 family_count;

    vkGetPhysicalDeviceQueueFamilyProperties(VK_PHYS_DEVICE, &family_count, NULL);

    VkQueueFamilyProperties* family_properties = Calloc(VkQueueFamilyProperties, family_count);

    vkGetPhysicalDeviceQueueFamilyProperties(VK_PHYS_DEVICE, &family_count, family_properties);

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

        vkGetPhysicalDeviceSurfaceSupportKHR(VK_PHYS_DEVICE,
                                             i,
                                             VK_SURFACE,
                                             &present_supported);

        if (present_supported && present_queue_found == false) {
            present_queue_index = i;
            present_queue_found = true;
        }

        if (graphics_queue_found && present_queue_found) break;
    }

    if (!graphics_queue_found) {
        printf("Graphics queue was not found\n");
        return 1;
    }

    if (!present_queue_found) {
        printf("Present queue was not found\n");
        return 2;
    }

    queues->graphics_supported = graphics_queue_found;
    queues->graphics           = graphics_queue_index;
    queues->present_supported  = present_queue_found;
    queues->present            = present_queue_index;

    return 0;
}

int create_logical_device(const char** extensions, u32 extensions_count) {
    VkPhysicalDeviceFeatures device_features = {
        .fillModeNonSolid  = VK_TRUE,
        .samplerAnisotropy = VK_TRUE
    };

    if (QUEUES.present == QUEUES.graphics) {
        float priority = 1.0f;
        VkDeviceQueueCreateInfo queue_create_info = {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = QUEUES.graphics,
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

        VkResult result = vkCreateDevice(VK_PHYS_DEVICE,
                                &device_create_info,
                                NULL,
                                &VK_DEVICE);

        if (result != VK_SUCCESS) {
            printf("Cannot create vkdevice. %d", result);
            return 1;
        }
    } else {
        VkDeviceQueueCreateInfo queue_create_info[2] = {};
        float priority = 1.0f;
        queue_create_info[0] = {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = QUEUES.graphics,
            .queueCount       = 1,
            .pQueuePriorities = &priority
        };

        queue_create_info[1] = {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = QUEUES.present,
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

        VkResult result = vkCreateDevice(VK_PHYS_DEVICE,
                                &device_create_info,
                                NULL,
                                &VK_DEVICE);

        if (result != VK_SUCCESS) {
            printf("Cannot create vkdevice. %d", result);
            return 1;
        }
    }

    return 0;
}

int select_surface_format(VkFormat target_format, VkColorSpaceKHR target_color_space, VkSurfaceFormatKHR* surface_format) {
    u32 surface_format_count;
    
    vkGetPhysicalDeviceSurfaceFormatsKHR(VK_PHYS_DEVICE,
                                         VK_SURFACE,
                                         &surface_format_count,
                                         NULL);

    if (surface_format_count == 0) {
        printf("Surface formats count is zero\n");
        return 1;
    }

    VkSurfaceFormatKHR* surface_formats = Calloc(VkSurfaceFormatKHR, surface_format_count);

    vkGetPhysicalDeviceSurfaceFormatsKHR(VK_PHYS_DEVICE,
                                         VK_SURFACE,
                                         &surface_format_count,
                                         surface_formats);
    
    for (u32 i = 0; i < surface_format_count; i++) {
        VkSurfaceFormatKHR format = surface_formats[i];

        if (format.format     == target_format  &&
            format.colorSpace == target_color_space) {
            *surface_format = format;
            printf("FORMAT_FOUND\n");
            break;
        }
    }

    return 0;
}

int select_present_mode(VkPresentModeKHR target, VkPresentModeKHR fallback, VkPresentModeKHR* present_mode) {
    u32 present_mode_count;

    vkGetPhysicalDeviceSurfacePresentModesKHR(VK_PHYS_DEVICE,
                                              VK_SURFACE,
                                              &present_mode_count,
                                              NULL);
    
    if (present_mode_count == 0) {
        printf("Present modes count is zero\n");
        return 1;
    }
    
    VkPresentModeKHR* present_modes = Calloc(VkPresentModeKHR, present_mode_count);

    vkGetPhysicalDeviceSurfacePresentModesKHR(VK_PHYS_DEVICE,
                                              VK_SURFACE,
                                              &present_mode_count,
                                              present_modes);

    bool target_found = false;

    for (u32 i = 0; i < present_mode_count; i++) {
        VkPresentModeKHR mode = present_modes[i];

        if (mode == target) {
            *present_mode = mode;
            target_found  = true;
            printf("Target present mode FOUND\n");
            break;
        }
    }

    if (!target_found) {
        for (u32 i = 0; i < present_mode_count; i++) {
            VkPresentModeKHR mode = present_modes[i];

            if (mode == fallback) {
                *present_mode = mode;
                printf("Fallback present mode FOUND\n");
                break;
            }
        }
    }

    return 0;
}

int create_swapchain(VkSurfaceCapabilitiesKHR surface_capabilities, VkSurfaceFormatKHR surface_format, VkPresentModeKHR present_mode) {
    u32 min_image_count = surface_capabilities.minImageCount + 1;

    if (min_image_count > surface_capabilities.maxImageCount) {
        min_image_count = surface_capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_info = {
        .sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext                  = NULL,
        .flags                  = 0,
        .surface                = VK_SURFACE,
        .minImageCount          = min_image_count,
        .imageFormat            = surface_format.format,
        .imageColorSpace        = surface_format.colorSpace,
        .imageExtent            = VK_EXTENT,
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
    
    u32 queue_family_indices[2] {QUEUES.graphics, QUEUES.present};

    if (QUEUES.graphics != QUEUES.present) {
        swapchain_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount = 2;
        swapchain_info.pQueueFamilyIndices   = queue_family_indices;
    }

    VkResult result = vkCreateSwapchainKHR(VK_DEVICE,
                                           &swapchain_info,
                                           NULL,
                                           &VK_SWAPCHAIN);

    if (result != VK_SUCCESS) {
        printf("Cannot create swapchain. %i\n", result);
        return result;
    }

    return 0;
}

int create_render_pipeline(VkSurfaceFormatKHR surface_format) {
    if (!create_shader_module(VK_DEVICE, "vert.spv", &VERT)) {
        printf("Failed to create vertex shader.\n");
        return 1;
    }

    if (!create_shader_module(VK_DEVICE, "frag.spv", &FRAG)) {
        printf("Failed to create fragment shader.\n");
        return 2;
    }

    VkPipelineShaderStageCreateInfo vert_shader_info = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext  = NULL,
        .flags  = 0,
        .stage  = VK_SHADER_STAGE_VERTEX_BIT,
        .module = VERT,
        .pName  = "main",
        .pSpecializationInfo = NULL
    };

    VkPipelineShaderStageCreateInfo frag_shader_info = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext  = NULL,
        .flags  = 0,
        .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = FRAG,
        .pName  = "main",
        .pSpecializationInfo = NULL
    };

    VkPipelineShaderStageCreateInfo shader_stages[2] = {vert_shader_info, frag_shader_info};

    VkVertexInputBindingDescription vertex_binding = {
        .binding   = 0,
        .stride    = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkVertexInputAttributeDescription vertex_attributes[2] = {
        {
            .location = 0,                          // inPosition
            .binding  = 0,
            .format   = VK_FORMAT_R32G32_SFLOAT,
            .offset   = offsetof(Vertex, position)
        },
        {
            .location = 1,                            // inColor  
            .binding  = 0,
            .format   = VK_FORMAT_R8G8B8A8_UNORM,
            .offset   = offsetof(Vertex, color)
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

    VK_VIEWPORT = {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = (float)VK_EXTENT.width,
        .height   = (float)VK_EXTENT.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VK_SCISSORS = {
        .offset = {0, 0},
        .extent = VK_EXTENT
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

    create_descriptor_set_layout(frame_bindings, 1, &PER_FRAME_SET_LAYOUT);
    create_descriptor_set_layout(matrix_bindings, 1, &MATRIX_SET_LAYOUT);

    VkDescriptorSetLayout set_layouts[] = {PER_FRAME_SET_LAYOUT, MATRIX_SET_LAYOUT};

    VkPipelineLayoutCreateInfo layout_info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = NULL,
        .flags                  = 0,
        .setLayoutCount         = 2,
        .pSetLayouts            = set_layouts,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = NULL
    };

    VkResult result = vkCreatePipelineLayout(VK_DEVICE,
                                    &layout_info,
                                    NULL,
                                    &VK_PIPELINE_LAYOUT);

    if (result != VK_SUCCESS) {
        printf("Cannot create pipeline layout. %d", result);
        return 3;
    }

    printf("Pipeline layout created.\n");

    VkAttachmentDescription color_attachment = {
        .flags          = 0,
        .format         = surface_format.format,
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

    VkRenderPassCreateInfo pass_info = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext           = NULL,
        .flags           = 0,
        .attachmentCount = 1,
        .pAttachments    = &color_attachment,
        .subpassCount    = 1,
        .pSubpasses      = &subpass,
        .dependencyCount = 0,
        .pDependencies   = NULL
    };

    result = vkCreateRenderPass(VK_DEVICE,
                                &pass_info,
                                NULL,
                                &VK_RENDER_PASS);

    if (result != VK_SUCCESS) {
        printf("Cannot create render pass. %d\n", result);
        return 4;
    }

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
        .layout              = VK_PIPELINE_LAYOUT,
        .renderPass          = VK_RENDER_PASS,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1
    };

    result = vkCreateGraphicsPipelines(VK_DEVICE,
                                       VK_NULL_HANDLE,
                                       1,
                                       &pipeline_info,
                                       NULL,
                                       &VK_PIPELINE);

    return 0;
}

void transform2d_to_model(Transform2d* transform, Matrix4* model) {
    *model = matrix4_trs_2d(transform->position.x, 
                            transform->position.y,
                            radians(transform->rotation),
                            transform->scale.x,
                            transform->scale.y);
}

void transforms_init(u32 initial_size) {
    TRANSFORMS        = Calloc(Transform2d, initial_size);
    MATRICES          = Calloc(Matrices, initial_size);
    TRANSFORMS_COUNT  = 0;
    TRANSFORMS_LENGTH = initial_size;

    create_buffer(sizeof(Matrices) * initial_size, 
                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &MATRIX_BUFFER,
                  &MATRIX_MEMORY);
}

void transforms_resize(u32 new_size) {
    TRANSFORMS        = Realloc(Transform2d, TRANSFORMS, sizeof(Transform2d) * new_size);
    MATRICES          = Realloc(Matrices, MATRICES, sizeof(Matrices) * new_size);
    TRANSFORMS_LENGTH = new_size;

    vkWaitForFences(VK_DEVICE, 1, &VK_SINGLE_FRAME_FENCE, VK_TRUE, u64_max);

    destroy_buffer(MATRIX_BUFFER, MATRIX_MEMORY);

    create_buffer(sizeof(Matrices) * new_size, 
                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &MATRIX_BUFFER,
                  &MATRIX_MEMORY);

    set_memory(MATRIX_MEMORY, sizeof(Matrices) * new_size, MATRICES);

    VkDescriptorBufferInfo buffer_info = {
        .buffer = MATRIX_BUFFER,
        .offset = 0,
        .range  = sizeof(Matrices) * TRANSFORMS_LENGTH,
    };

    VkWriteDescriptorSet descriptor_writes[] = {
    {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = MATRIX_DESCRIPTOR_SET,
        .dstBinding      = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo     = &buffer_info,
    },
    };

    vkUpdateDescriptorSets(VK_DEVICE, 1, descriptor_writes, 0, NULL);
}

u32 transforms_append(Transform2d transform) {
    if (TRANSFORMS_COUNT >= TRANSFORMS_LENGTH) {
        transforms_resize(TRANSFORMS_COUNT * 2);
    }

    u32 index = TRANSFORMS_COUNT;
    TRANSFORMS[index] = transform;
    transform2d_to_model(&transform, &MATRICES[index].model);
    matrix4_mvp(&CAMERA, transform.position, transform.rotation, transform.scale, &MATRICES[index].mvp);
    TRANSFORMS_COUNT++;

    return index;
}

void transforms_set(u32 index, Transform2d* transform) {
    TRANSFORMS[index] = *transform;
}

void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* memory) {
    VkBufferCreateInfo buffer_info = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = size,
        .usage       = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    vkCreateBuffer(VK_DEVICE, &buffer_info, NULL, buffer);

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(VK_DEVICE, *buffer, &mem_requirements);

    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(VK_PHYS_DEVICE, &mem_properties);
    
    u32 mem_type_index = 0;

    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((mem_requirements.memoryTypeBits & (1 << i)) && 
            (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            mem_type_index = i;
            break;
        }
    }

    VkMemoryAllocateInfo alloc_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = mem_requirements.size,
        .memoryTypeIndex = mem_type_index
    };

    vkAllocateMemory(VK_DEVICE, &alloc_info, NULL, memory);

    vkBindBufferMemory(VK_DEVICE, *buffer, *memory, 0);
}

void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandBufferAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = VK_COMMAND_POOL,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(VK_DEVICE, &alloc_info, &command_buffer);

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

    vkCmdCopyBuffer(command_buffer, src, dst, 1, &copy_region);
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &command_buffer
    };

    vkQueueSubmit(VK_GRAPHICS_QUEUE, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(VK_GRAPHICS_QUEUE);

    vkFreeCommandBuffers(VK_DEVICE, VK_COMMAND_POOL, 1, &command_buffer);
}

void set_memory(VkDeviceMemory memory, VkDeviceSize size, void* src) {
    void* data;
    vkMapMemory(VK_DEVICE, memory, 0, size, 0, &data);
    memcpy(data, src, size);
    vkUnmapMemory(VK_DEVICE, memory);
}

void destroy_buffer(VkBuffer buffer, VkDeviceMemory memory) {
    vkDestroyBuffer(VK_DEVICE, buffer, NULL);
    vkFreeMemory(VK_DEVICE, memory, NULL);
}

void create_index_buffer(u16* indices, u32 index_count, VkBuffer* buffer, VkDeviceMemory* memory) {
    u64 size = sizeof(indices[0]) * index_count;

    VkBuffer       staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(size, 
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &staging_buffer,
                  &staging_buffer_memory);
    
    void* data;
    vkMapMemory(VK_DEVICE, staging_buffer_memory, 0, size, 0, &data);
    memcpy(data, indices, size);
    vkUnmapMemory(VK_DEVICE, staging_buffer_memory);

    create_buffer(size, 
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  buffer,
                  memory);

    copy_buffer(staging_buffer, *buffer, size);

    vkDestroyBuffer(VK_DEVICE, staging_buffer, NULL);
    vkFreeMemory(VK_DEVICE, staging_buffer_memory, NULL);
}

static inline void matrix4_mvp(Camera2D* camera, Vector2 position, float rotation, Vector2 scale, Matrix4* mat) {
    *mat = matrix4_mvp(camera->position.x, -camera->position.y, camera->left, camera->right, camera->top, camera->bottom, position.x, position.y, radians(rotation), scale.x, scale.y);
}

static inline void update_matrices(Transform2d* transforms, Matrices* matrices, u32 count, VkDeviceMemory mem) {
    for (u32 i = 0; i < count; i++) {
        transform2d_to_model(&transforms[i], &MATRICES[i].model);
        matrix4_mvp(&CAMERA, transforms[i].position, transforms[i].rotation, transforms[i].scale, &MATRICES[i].mvp);
    }
    VkDeviceSize size = sizeof(Matrices) * count;
    set_memory(mem, size, matrices);
}