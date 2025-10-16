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

#define VERTEX_BUFFERS_INITIAL_LENGTH 8
#define VERTEX_BUFFERS_STEP           8
#define UBO_ALIGNMENT                 256

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

typedef struct ViewProjectionUBO {
    alignas(16) Matrix4 view;
    alignas(16) Matrix4 proj;
} ViewProjectionUBO;

typedef struct ModelUBO {
    alignas(16) Matrix4 model;
    alignas(16) Matrix4 mvp;
} ModelUBO;

typedef struct RenderBatch {
    VkBuffer       buffer;
    VkDeviceSize   offsets;
    VkDeviceMemory memory;
    u32            vertex_count;
} RenderBatch;

Window*          WINDOW{};
VkInstance       VK_INSTANCE{};
VkSurfaceKHR     VK_SURFACE{};
VkPhysicalDevice VK_PHYS_DEVICE{};
VkDevice         VK_DEVICE{};
VkSwapchainKHR   VK_SWAPCHAIN{};
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
VkBuffer*        VERTEX_BUFFERS;
VkDeviceSize*    OFFSETS;
VkDeviceMemory*  MEMORY;
u32              VERTEX_BUFFERS_COUNT  = 0;
u32              VERTEX_BUFFERS_LENGTH = 0;
VkDescriptorSetLayout VK_DESCRIPTOR_SET_LAYOUT{};
UniformBuffer         VIEW_PROJ_UBO;
UniformBuffer         MODELS;
VkDescriptorPool      DESCRIPTOR_POOL;
VkDescriptorSet       DESCRIPTOR_SET;

VkFence     VK_SINGLE_FRAME_FENCE{};
VkSemaphore VK_SEMAPHORE_IMAGE_READY{};
VkSemaphore VK_SEMAPHORE_RENDER_FINISHED{};

Matrix4 VIEW;
Matrix4 PROJECTION;
Matrix4 MODEL;

Camera2D CAMERA = {};

double TIME{};

void print_instance_extensions();
void append_vertex_buffer(VkBuffer buffer, VkDeviceSize offset, VkDeviceMemory mem);
void clear_vertex_buffers();
void destroy_vertex_buffers();
bool create_shader_module(VkDevice device, const char* name, VkShaderModule* shader_module);
void create_vertex_buffer(Vertex* vertices, u32 vertex_count);
void create_descriptor_set_layout(VkDescriptorSetLayout* dsl);
void create_uniform_buffer(UniformBuffer* ub, u32 size);
void create_descriptor_set(VkDescriptorSet* descriptor_set);
void update_view_projection(UniformBuffer* ubo, ViewProjectionUBO* view_proj);
void update_model(UniformBuffer* ubo, ModelUBO* model, u32 index);
void camera2d_make(Vector2 position, float size, Camera2D* cam);
void camera2d_update(Camera2D* cam);
void camera2d_move(Camera2D* cam, Vector2 dir);

int main(int argc, char** argv) {
    const char* name         = "Hello";

    GlassErrorCode err = glass_create_window(100, 100, 800, 600, name, &WINDOW);
    if (err != GLASS_OK) {
        printf("Cannot create window. %d\n", err);
        return 1;
    }

    u32 instance_extensions_count;
    const char** instance_extensions = glass_get_vulkan_instance_extensions(&instance_extensions_count);

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
        .enabledExtensionCount   = instance_extensions_count,
        .ppEnabledExtensionNames = instance_extensions,
    };

    auto result = vkCreateInstance(&vk_info, null, &VK_INSTANCE);

    if (result != VK_SUCCESS) {
        printf("Cannot create vk_instance\n");
        return 1;
    }

    // get physical device
    u32               phys_devices_count;
    VkPhysicalDevice* devices;
    vkEnumeratePhysicalDevices(VK_INSTANCE, &phys_devices_count, null);
    devices = Calloc(VkPhysicalDevice, phys_devices_count);
    vkEnumeratePhysicalDevices(VK_INSTANCE, &phys_devices_count, devices);

    printf("GPU count: %i\n", phys_devices_count);

    if (phys_devices_count == 0) {
        return 1;
    }

    // VkPhysicalDevice VK_PHYS_DEVICE;

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
            break;
        }
    }


    err = glass_create_vulkan_surface(WINDOW, VK_INSTANCE, &VK_SURFACE);

    if (err != GLASS_OK) {
        printf("Cannot create win32 surface. %i\n", err);
        return 1;
    }

    // select graphics and present queues
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
        return 1;
    }

    // create logical device
    u32   enabled_extensions_count = 1;
    const char* extension_names[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkPhysicalDeviceFeatures device_features = {
        .fillModeNonSolid  = VK_TRUE,
        .samplerAnisotropy = VK_TRUE
    };

    if (present_queue_index == graphics_queue_index) {
        float priority = 1.0f;
        VkDeviceQueueCreateInfo queue_create_info = {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = graphics_queue_index,
            .queueCount       = 1,
            .pQueuePriorities = &priority
        };

        VkDeviceCreateInfo device_create_info = {
            .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext                   = NULL,
            .queueCreateInfoCount    = 1,
            .pQueueCreateInfos       = &queue_create_info,
            .enabledExtensionCount   = enabled_extensions_count,
            .ppEnabledExtensionNames = extension_names,
            .pEnabledFeatures        = &device_features
        };

        result = vkCreateDevice(VK_PHYS_DEVICE,
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
            .queueFamilyIndex = graphics_queue_index,
            .queueCount       = 1,
            .pQueuePriorities = &priority
        };

        queue_create_info[1] = {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = present_queue_index,
            .queueCount       = 1,
            .pQueuePriorities = &priority
        };

        VkDeviceCreateInfo device_create_info = {
            .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext                   = NULL,
            .queueCreateInfoCount    = 2,
            .pQueueCreateInfos       = queue_create_info,
            .enabledExtensionCount   = enabled_extensions_count,
            .ppEnabledExtensionNames = extension_names,
            .pEnabledFeatures        = &device_features
        };

        result = vkCreateDevice(VK_PHYS_DEVICE,
                                &device_create_info,
                                NULL,
                                &VK_DEVICE);

        if (result != VK_SUCCESS) {
            printf("Cannot create vkdevice. %d", result);
            return 1;
        }
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

    VkSurfaceFormatKHR surface_format = {};
    
    for (u32 i = 0; i < surface_format_count; i++) {
        VkSurfaceFormatKHR format = surface_formats[i];

        if (format.format     == VK_FORMAT_R8G8B8A8_SRGB  &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surface_format = format;
            printf("FORMAT_FOUND\n");
            break;
        }
    }

    printf("Format: %i\n", surface_format.format);
    printf("Color space: %i\n", surface_format.colorSpace);
    
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

    VkPresentModeKHR present_mode = {};
    bool immediate_found = false;

    for (u32 i = 0; i < present_mode_count; i++) {
        VkPresentModeKHR mode = present_modes[i];

        if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            present_mode    = mode;
            immediate_found = true;
            printf("Immediate present mode FOUND\n");
            break;
        }
    }

    if (!immediate_found) {
        for (u32 i = 0; i < present_mode_count; i++) {
            VkPresentModeKHR mode = present_modes[i];

            if (mode == VK_PRESENT_MODE_FIFO_KHR) {
                present_mode    = mode;
                printf("FIFO present mode FOUND\n");
                break;
            }
        }
    }

    printf("Present mode: %i\n", present_mode);
    
    // create swapchain
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
    
    u32 queue_family_indices[2] {graphics_queue_index, present_queue_index};

    if (graphics_queue_index != present_queue_index) {
        swapchain_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount = 2;
        swapchain_info.pQueueFamilyIndices   = queue_family_indices;
    }

    result = vkCreateSwapchainKHR(VK_DEVICE,
                                  &swapchain_info,
                                  NULL,
                                  &VK_SWAPCHAIN);

    if (result != VK_SUCCESS) {
        printf("Cannot create swapchain. %i\n", result);
        return 1;
    }

    printf("Swapchain created.\n");

    // create graphics pipeline
    VkShaderModule vert_shader = {};
    VkShaderModule frag_shader = {};

    if (!create_shader_module(VK_DEVICE, "vert.spv", &vert_shader)) {
        printf("Failed to create vertex shader.\n");
        return 1;
    }

    if (!create_shader_module(VK_DEVICE, "frag.spv", &frag_shader)) {
        printf("Failed to create fragment shader.\n");
        return 1;
    }

    VkPipelineShaderStageCreateInfo vert_shader_info = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext  = NULL,
        .flags  = 0,
        .stage  = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vert_shader,
        .pName  = "main",
        .pSpecializationInfo = NULL
    };

    VkPipelineShaderStageCreateInfo frag_shader_info = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext  = NULL,
        .flags  = 0,
        .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = frag_shader,
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

    // VkPipelineLayout pipeline_layout = {};

    Vertex vertices[] = {
        {{ -0.5f,  0.0f }, { 255, 0,   0,   255 }},
        {{  0.5f,  0.0f }, { 0,   255, 0,   255 }},
        {{  0.0f,  0.5f }, { 0,   0,   255, 255 }},
    };

    create_vertex_buffer(vertices, 3);
    create_descriptor_set_layout(&VK_DESCRIPTOR_SET_LAYOUT);
    create_uniform_buffer(&VIEW_PROJ_UBO, sizeof(ViewProjectionUBO));

    u32 model_size = 1 * UBO_ALIGNMENT;
    create_uniform_buffer(&MODELS, model_size);

    create_descriptor_set(&DESCRIPTOR_SET);

    VkPipelineLayoutCreateInfo layout_info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = NULL,
        .flags                  = 0,
        .setLayoutCount         = 1,
        .pSetLayouts            = &VK_DESCRIPTOR_SET_LAYOUT,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = NULL
    };

    result = vkCreatePipelineLayout(VK_DEVICE,
                                    &layout_info,
                                    NULL,
                                    &VK_PIPELINE_LAYOUT);

    if (result != VK_SUCCESS) {
        printf("Cannot create pipeline layout. %d", result);
        return 1;
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
        return 1;
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

    if (result != VK_SUCCESS) {
        printf("Cannot create graphics pipeline. %i\n", result);
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
    VkCommandPool command_pool;

    VkCommandPoolCreateInfo command_pool_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = NULL,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphics_queue_index
    };

    result = vkCreateCommandPool(VK_DEVICE,
                                 &command_pool_info,
                                 NULL,
                                 &command_pool);
                                
    if (result != VK_SUCCESS) {
        printf("Cannot create command bool. %d\n", result);
        return 1;
    }

    VkCommandBufferAllocateInfo command_buffer_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = NULL,
        .commandPool        = command_pool,
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

    vkCreateSemaphore(VK_DEVICE, &image_ready_semaphore_info,     NULL, &VK_SEMAPHORE_IMAGE_READY);
    vkCreateSemaphore(VK_DEVICE, &render_finished_semaphore_info, NULL, &VK_SEMAPHORE_RENDER_FINISHED);
    vkCreateFence    (VK_DEVICE, &single_frame_fence_info,        NULL, &VK_SINGLE_FRAME_FENCE);

    vkGetDeviceQueue(VK_DEVICE,
                     graphics_queue_index,
                     0,
                     &VK_GRAPHICS_QUEUE);

    vkGetDeviceQueue(VK_DEVICE,
                     present_queue_index,
                     0,
                     &VK_PRESENT_QUEUE);

    camera2d_make(vector2_make(0, 0), 4, &CAMERA);
    camera2d_update(&CAMERA);
    // float left               = camera_position[0] - camera_size;
    // float right              = camera_position[0] + camera_size;
    // float top                = camera_position[1] + camera_size;
    // float bottom             = camera_position[1] - camera_size;

    // VIEW       = matrix4_transform_2d(-camera_position[0], -camera_position[1]);
    // PROJECTION = matrix4_ortho_2d(left, right, top, bottom);
    float angle = 0.0f;
    const float rotation_speed = 10.0f;
    float position[2] = {0, 0};
    float speed = 2.0f;
    MODEL      = matrix4_trs_2d(position[0], position[1], radians(angle), 1, 1);

    ViewProjectionUBO view_proj = {
        .view = VIEW,
        .proj = PROJECTION
    };

    Matrix4 mvp = matrix4_mvp(CAMERA.position.x,
                              CAMERA.position.y,
                              CAMERA.left,
                              CAMERA.right,
                              CAMERA.top,
                              CAMERA.bottom,
                              position[0],
                              position[1],
                              radians(angle),
                              1, 1);

    ModelUBO ubo = {
        .model = MODEL,
        .mvp   = mvp
    };

    update_view_projection(&VIEW_PROJ_UBO, &view_proj);
    update_model(&MODELS, &ubo, 0);

    TIME = glass_get_time();

    while (true) {
        if (glass_is_button_pressed(GLASS_SCANCODE_ESCAPE)) {
            glass_exit();
            break;
        }

        if (glass_exit_required())
            break;
        
        double current_time = glass_get_time();
        double actual_dt    = current_time - TIME;
               TIME         = current_time;
        int    fps          = (int)(1.0l / actual_dt);
        char buf[128];
        float dt = (float)actual_dt;
        sprintf(buf, "fps: %i", fps);
        glass_set_window_title(WINDOW, buf);

        angle += rotation_speed * dt;

        if (glass_is_button_pressed(GLASS_SCANCODE_W)) {
            position[1] += speed * dt;
        } else if (glass_is_button_pressed(GLASS_SCANCODE_S)) {
            position[1] -= speed * dt;
        }

        if (glass_is_button_pressed(GLASS_SCANCODE_A)) {
            position[0] -= speed * dt;
        } else if (glass_is_button_pressed(GLASS_SCANCODE_D)) {
            position[0] += speed * dt;
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

        camera2d_move(&CAMERA, camera_move);

        MODEL = matrix4_trs_2d(position[0], position[1], radians(angle), 1, 1);

        Matrix4 mvp = matrix4_mvp(CAMERA.position.x,
                                  -CAMERA.position.y,
                                  CAMERA.left,
                                  CAMERA.right,
                                  CAMERA.top,
                                  CAMERA.bottom,
                                  position[0],
                                  position[1],
                                  radians(angle),
                                  1, 1);

        ModelUBO ubo = {
            .model = MODEL,
            .mvp   = mvp
        };

        update_model(&MODELS, &ubo, 0);
        glass_main_loop();
    }

    printf("Exit begin.\n");

    vkDeviceWaitIdle(VK_DEVICE);

    destroy_vertex_buffers();
    vkDestroyDescriptorPool(VK_DEVICE, DESCRIPTOR_POOL, NULL);
    vkDestroyDescriptorSetLayout(VK_DEVICE, VK_DESCRIPTOR_SET_LAYOUT, NULL);
    vkDestroyBuffer(VK_DEVICE, VIEW_PROJ_UBO.buffer, NULL);
    vkFreeMemory(VK_DEVICE, VIEW_PROJ_UBO.mem, NULL);

    vkDestroyBuffer(VK_DEVICE, MODELS.buffer, NULL);
    vkFreeMemory(VK_DEVICE, MODELS.mem, NULL);
    
    vkDestroySemaphore(VK_DEVICE, VK_SEMAPHORE_IMAGE_READY, NULL);
    vkDestroySemaphore(VK_DEVICE, VK_SEMAPHORE_RENDER_FINISHED, NULL);
    vkDestroyFence(VK_DEVICE, VK_SINGLE_FRAME_FENCE, NULL);

    vkDestroyCommandPool(VK_DEVICE, command_pool, NULL);

    for (u32 i = 0; i < image_count; i++) {
        vkDestroyFramebuffer(VK_DEVICE, VK_FRAME_BUFFERS[i], NULL);
        vkDestroyImageView(VK_DEVICE, image_views[i], NULL);
    }

    vkDestroyPipeline(VK_DEVICE, VK_PIPELINE, NULL);
    vkDestroyPipelineLayout(VK_DEVICE, VK_PIPELINE_LAYOUT, NULL);
    vkDestroyRenderPass(VK_DEVICE, VK_RENDER_PASS, NULL);
    vkDestroyShaderModule(VK_DEVICE, vert_shader, NULL);
    vkDestroyShaderModule(VK_DEVICE, frag_shader, NULL);
    vkDestroySwapchainKHR(VK_DEVICE, VK_SWAPCHAIN, NULL);
    vkDestroyDevice(VK_DEVICE, NULL);
    vkDestroySurfaceKHR(VK_INSTANCE,
                        VK_SURFACE,
                        null);
    vkDestroyInstance(VK_INSTANCE, null);

    glass_destroy_window(WINDOW);

    Vector3 a = {.x = 1, .y = 2, .z = 3};
    Vector3 b = {.x = 3, .y = 2, .z = 1};

    // Vector3 c = a + b;
    a += b;

    Matrix4 mat = matrix4_make(1, 2, 3, 4,
                               5, 6, 7, 8,
                               9, 10, 11, 12,
                               13, 14, 15, 16);

    Matrix4 mat2 = matrix4_make(1, 2, 3, 4,
                                5, 6, 7, 8,
                                9, 10, 11, 12,
                                13, 14, 15, 16);

    printf("(%f, %f, %f)\n", a.x, a.y, a.z);

    for (u32 i = 0; i < 16; i++) {
        if (i % 4 == 0) printf("\n");

        printf("%f,", mat.e[i]);
    }

    printf("\n");

    for (u32 i = 0; i < 16; i++) {
        if (i % 4 == 0) printf("\n");

        printf("%f,", mat2.e[i]);
    }
    printf("\n");

    Matrix4 mat3 = matrix4_add(&mat, &mat2);

    for (u32 i = 0; i < 16; i++) {
        if (i % 4 == 0) printf("\n");

        printf("%f,", mat3.e[i]);
    }
    printf("\n");

    mat3 = matrix4_sub(&mat3, &mat2);

    for (u32 i = 0; i < 16; i++) {
        if (i % 4 == 0) printf("\n");

        printf("%f,", mat3.e[i]);
    }
    printf("\n");

    mat3 = matrix4_mul(&mat, &mat2);

    for (u32 i = 0; i < 16; i++) {
        if (i % 4 == 0) printf("\n");

        printf("%f,", mat3.e[i]);
    }
    printf("\n");

    float det = matrix4_det(&mat);

    printf("%f\n", det);

    printf("\n");

    float positions[] = {1, 2, 3, 4, 5, 6};  // 24
    Color colors[] = {{255, 255, 255, 255},  // 4
                      {255, 255, 255, 255},  // 4
                      {255, 255, 255, 255}}; // 4

    Shape2D shape;

    shape2d_make(positions, colors, 3, &Allocator_Std, &shape);

    printf("%d\n", shape.vertex_count);

    for (u32 i = 0; i < shape.vertex_count; i++) {
        Vertex2D vertex = shape.vertices[i];
        Color    color  = shape.colors[i];
        printf("%f, %f, %ir, %ig, %ib, %ia\n", vertex.position.x, vertex.position.y, color.r, color.g, color.b, color.a);
    }

    shape2d_free(&shape, &Allocator_Std);

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
    vkCmdBindVertexBuffers(VK_COMMAND_BUFFER, 0, VERTEX_BUFFERS_COUNT, VERTEX_BUFFERS, OFFSETS);
    u32 dynamic_offset = 0 * UBO_ALIGNMENT;
    vkCmdBindDescriptorSets(VK_COMMAND_BUFFER, VK_PIPELINE_BIND_POINT_GRAPHICS, VK_PIPELINE_LAYOUT, 0, 1, &DESCRIPTOR_SET, 1, &dynamic_offset);

    vkCmdDraw(VK_COMMAND_BUFFER, 3, 1, 0, 0);
    
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
        .pSignalSemaphores    = &VK_SEMAPHORE_RENDER_FINISHED
    };

    result = vkQueueSubmit(VK_GRAPHICS_QUEUE, 1, &submit_info, VK_SINGLE_FRAME_FENCE);

    if (result != VK_SUCCESS) {
        printf("Cannot submit queue. %d\n", result);
        return GLASS_RENDER_ERROR;
    }

    VkPresentInfoKHR present_info = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &VK_SEMAPHORE_RENDER_FINISHED,
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

void append_vertex_buffer(VkBuffer buffer, VkDeviceSize offset, VkDeviceMemory mem) {
    if (VERTEX_BUFFERS_COUNT == 0 && !VERTEX_BUFFERS) {
        VERTEX_BUFFERS        = Calloc(VkBuffer, VERTEX_BUFFERS_INITIAL_LENGTH);
        OFFSETS               = Calloc(VkDeviceSize, VERTEX_BUFFERS_INITIAL_LENGTH);
        MEMORY                = Calloc(VkDeviceMemory, VERTEX_BUFFERS_INITIAL_LENGTH);
        VERTEX_BUFFERS_LENGTH = VERTEX_BUFFERS_INITIAL_LENGTH;
    } else if (VERTEX_BUFFERS_COUNT >= VERTEX_BUFFERS_LENGTH) {
        VERTEX_BUFFERS_LENGTH += VERTEX_BUFFERS_STEP;
        VERTEX_BUFFERS         = Realloc(VkBuffer, 
                                         VERTEX_BUFFERS, 
                                         sizeof(VkBuffer) * VERTEX_BUFFERS_LENGTH);
        OFFSETS                = Realloc(VkDeviceSize, 
                                         OFFSETS, 
                                         sizeof(VkDeviceSize) * VERTEX_BUFFERS_LENGTH);
        MEMORY                 = Realloc(VkDeviceMemory, 
                                         OFFSETS, 
                                         sizeof(VkDeviceMemory) * VERTEX_BUFFERS_LENGTH);
    }

    VERTEX_BUFFERS[VERTEX_BUFFERS_COUNT]  = buffer;
    OFFSETS[VERTEX_BUFFERS_COUNT]         = offset;
    MEMORY[VERTEX_BUFFERS_COUNT]          = mem;
    VERTEX_BUFFERS_COUNT                 += 1;
}

void clear_vertex_buffers() {
    for (u32 i = 0; i < VERTEX_BUFFERS_COUNT; i++) {
        vkDestroyBuffer(VK_DEVICE, VERTEX_BUFFERS[i], NULL);
        vkFreeMemory(VK_DEVICE, MEMORY[i], NULL);
    }
    VERTEX_BUFFERS_COUNT = 0;
}

void destroy_vertex_buffers() {
    clear_vertex_buffers();
    Free(VERTEX_BUFFERS);
    Free(OFFSETS);
    Free(MEMORY);
}

void create_vertex_buffer(Vertex* vertices, u32 vertex_count) {
    VkBufferCreateInfo buffer_info = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = sizeof(vertices[0]) * vertex_count,
        .usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VkBuffer vertex_buffer;
    vkCreateBuffer(VK_DEVICE, &buffer_info, NULL, &vertex_buffer);

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(VK_DEVICE, vertex_buffer, &mem_requirements);

    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(VK_PHYS_DEVICE, &mem_properties);
    
    u32 mem_type_index = 0;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

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

    VkDeviceMemory memory;
    vkAllocateMemory(VK_DEVICE, &alloc_info, NULL, &memory);

    vkBindBufferMemory(VK_DEVICE, vertex_buffer, memory, 0);

    void* data;
    vkMapMemory(VK_DEVICE, memory, 0, buffer_info.size, 0, &data);
    memcpy(data, vertices, buffer_info.size);
    vkUnmapMemory(VK_DEVICE, memory);
    append_vertex_buffer(vertex_buffer, 0, memory);
}

void create_descriptor_set_layout(VkDescriptorSetLayout* dsl) {
    VkDescriptorSetLayoutBinding bindings[2] = {};
    
    bindings[0] = {
        .binding            = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount    = 1,
        .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = NULL,
    };
    bindings[1] = {
        .binding            = 1,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .descriptorCount    = 1,
        .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = NULL,
    };

    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 2,
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

void create_descriptor_set(VkDescriptorSet* descriptor_set) {
    // Create descriptor pool
    VkDescriptorPoolSize pool_sizes[2] = {
        {
            .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1
        },
        {
            .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = 1,
        }
    };

    VkDescriptorPoolCreateInfo pool_info = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets       = 1,
        .poolSizeCount = 2,
        .pPoolSizes    = pool_sizes,
    };

    vkCreateDescriptorPool(VK_DEVICE, &pool_info, NULL, &DESCRIPTOR_POOL);

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = DESCRIPTOR_POOL,
        .descriptorSetCount = 1,
        .pSetLayouts        = &VK_DESCRIPTOR_SET_LAYOUT,
    };

    vkAllocateDescriptorSets(VK_DEVICE, &alloc_info, descriptor_set);

    // Update descriptor set
    VkDescriptorBufferInfo view_proj_buffer_info = {
        .buffer = VIEW_PROJ_UBO.buffer,
        .offset = 0,
        .range  = sizeof(ViewProjectionUBO),
    };

    VkDescriptorBufferInfo model_buffer_info = {
        .buffer = MODELS.buffer,
        .offset = 0,
        .range  = sizeof(ModelUBO)
    };

    VkWriteDescriptorSet descriptor_writes[2] = {
    {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = *descriptor_set,
        .dstBinding      = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo     = &view_proj_buffer_info,
    },
    {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = *descriptor_set,
        .dstBinding      = 1,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .pBufferInfo     = &model_buffer_info,
    }
    };

    vkUpdateDescriptorSets(VK_DEVICE, 2, descriptor_writes, 0, NULL);
}

void update_view_projection(UniformBuffer* ubo, ViewProjectionUBO* view_proj) {
    memcpy(ubo->mapped, view_proj, sizeof(ViewProjectionUBO));
}

void update_model(UniformBuffer* ubo, ModelUBO* model, u32 index) {
    u32 offset = index * UBO_ALIGNMENT;
    memcpy((char*)ubo->mapped + offset, model, sizeof(ModelUBO));
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

    ViewProjectionUBO view_proj = {
        .view = VIEW,
        .proj = PROJECTION
    };

    update_view_projection(&VIEW_PROJ_UBO, &view_proj);
}

void camera2d_move(Camera2D* cam, Vector2 dir) {
    cam->position.x += dir.x;
    cam->position.y += dir.y;

    printf("Camera position: (%f, %f)\n", cam->position.x, cam->position.y);
    camera2d_update(cam);
}