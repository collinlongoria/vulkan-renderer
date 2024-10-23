// Collin Longoria
// 10/16/24

#include "HelloTriangle.h"

#include <iostream> // err, endl, cout
#include <stdexcept> // error handling
#include <cstring> // strcmp
#include <limits> // numeric_limits
#include <optional> // optional
#include <set> // set
#include <algorithm> // clamp
#include <fstream> // file

// constants for window resolution
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

/* Related to shader module creation */
static VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

/* Related to loading binary data from files */
static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary); // ate - start reading at end of file; binary - read as a binary file

    if(!file.is_open()) {
        throw std::runtime_error("Could not open file");
    }

    // read the size of the file and allocate a buffer
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    // jump to start of file and read bytes
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    // close and return bytes
    file.close();
    return buffer;
}

/* Related to Device Extensions */
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME // advantage to using this micro is the compiler will catch mispellings
};


/* Related to Debug Messenger: */

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

// static member function with the PFN_vkDebugUtilsMessengerCallbackEXT prototype
/*
 * VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT <-- Diagnostic message
 * VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT <-- Informational message like the creation of a resource
 * VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT <-- Message about behavior that is not necessarily an error, but very likely a bug in application
 * VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT <-- Message about behavior that is invalid and may cause crashes
 */
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT; // structure type
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT; // specifies all the types of severities the callback will be called for
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT; // specifies which types of messages the callback is notified about
    createInfo.pfnUserCallback = debugCallback; // pointer to the callback function
    createInfo.pUserData = nullptr; // optional - will be passed along to the callback function
}

/*
 * Debug struct (created in application) requires the vkCreateDebugUtilsMessengerEXT function
 * This function is an extension, so it needs to be looked up using vkGetInstanceProcAddr
 */
static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if(func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}


/* Related to Physical Devices and Queue Families: */

// struct for queue family indices
struct QueueFamilyIndices {
    // because the presentation is a queue specific feature, the problem is actually about finding a queue family that supports presenting to the surface we need
    // its actually possible that the queue families supporting drawing commands and the ones supporting presentation do not overlap
    // therefore we have to take into account that there could be a distinct presentation queue
    // (most likely, these will be the same)
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    // generic check for optional
    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices;

    // get device's number of dedicated queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    // query for details about each available queue family
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    // find queue family that supports VK_QUEUE_GRAPHICS_BIT
    int i = 0;
    for(const auto& queueFamily : queueFamilies) {
        if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        // check for presentation support
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if(presentSupport) {
            indices.presentFamily = i;
        }

        // early exit if bit has already been found
        if(indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

// helper function to check if device supports all needed extensions
static bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for(const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

/* Related to Swap Chain support */

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// function to query for details of swap chain support
/*
 * Three kinds of properties we need to check:
 * Basic surface capabilties (min/max number of images in swap chain, minmax width and height of images)
 * Surface formats (pixel format, color space)
 * Available presentation modes
 */
static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChainSupportDetails details;

    // query for basic surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    // query supported surface formats
    // list of structs, so follows same ritual of 2 function calls
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if(formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    // query supported presentation modes
    // works exact same way
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if(presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

// function to choose format settings for swap chain
static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // select color mode
    // prefer SRGB, but otherwise just use the first format
    for(const auto& availableFormat : availableFormats) {
        if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

// function to choose presentation mode for the swap chain (arguably the most important setting!)
/*
 * Four possible modes in Vulkan:
 * VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by application are transferred to the screen right away (may result in tearing)
 * VK_PRESENT_MODE_FIFO_KHR: Swap chain is a queue where the display takes an image from the front of the queue when the display is refreshed
 *                           and the program has to wait. This is most similar to vertical sync in modern games. The moment that the display is refreshed
 *                           is known as "vertical blank"
 * VK_PRESENT_MODE_FIFO_RELAXED_KHR: This mode only differs from the previous if the application is late and the queue was empty at the last vertical blank.
 *                                   Instead of waiting for the next vertical blank, the image is transferred right away when it finally arrives. This may result in visible tearing.
 * VK_PRESENT_MODE_MAILBOX_KHR: This is another variation of the second mode. Instead of blocking the application when the queue is full, the images that are already queued are
 *                              simply replaced with the newer ones. This mode can be used to render frames as fast as possible while still avoid tearing, resulting in fewer latency issues
 *                              than standard vertical sync. This is commonly known as "triple buffering", although the existence of three buffers alone does not necessarily mean that the framerate is unlocked.
 */
// only the VK_PRESENT_MODE_FIFO_KHR mode is guaranteed to be available
static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for(const auto& availablePresentMode : availablePresentModes) {
        if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

// function to choose swap chain extent property
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
    // need to query the resolution of the window in pixel

    if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        // clamp function is used here to bound the values of width and height between the allowed min and max extents that are supported by the implementation
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

// helper function to check if a graphics card can perform needed operations
static bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
    /*
     * For the purposes of this, there is no need for any checks. Any Vulkan supporting graphics card will suffice.
     * For keeping sufficient documentation for future reference, this function will check to make sure the graphics
     * card supports geometry shaders.
     */

    // query device properties
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // query optional features
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // query queue families
    QueueFamilyIndices indices = findQueueFamilies(device, surface);

    // validate device has extensions needed
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    // verify swap chain support is adequate
    // for the purposes of this application, need at least one supported image format and one supported presentation mode
    bool swapChainAdequate = false;
    if(extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    // anything required should be returned as a boolean
    return deviceFeatures.geometryShader && indices.isComplete() && extensionsSupported && swapChainAdequate;

    // other ways of finding suitable GPU could be mapping scores to available cards, or simply allow user to choose from list of valid cards.
}


/* Class-Specific Function Definitions: */

void HelloTriangle::run() {
    // this is just the engine loop
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void HelloTriangle::initWindow() {
    // standard GLFW init
    glfwInit();

    // now tell it not to use OpenGL
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // handling resized window takes special care, so disable it for now
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // create the actual window
    window = glfwCreateWindow(WIDTH, HEIGHT, "VulkanRenderer", nullptr, nullptr);

}

void HelloTriangle::initVulkan() {
    // create the vulkan instance
    createInstance();

    // setup the debug messenger
    setupDebugMessenger();

    // creates surface object, right after creating the instance (and setting up debug, but that is irrelevant)
    createSurface();

    // retrieve graphics card
    pickPhysicalDevice();

    // create logical device
    createLogicalDevice();

    // create swap chain
    createSwapChain();

    // create image views
    createImageViews();

    // create render pass
    createRenderPass();

    // create graphics pipeline
    createGraphicsPipeline();

    // create frame buffers
    createFramebuffers();
}

void HelloTriangle::createCommandPool() {

}

void HelloTriangle::createFramebuffers() {
    // resize container to hold all framebuffers
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void HelloTriangle::createRenderPass() {
    // single color buffer attachment represented by one of the images from the swap chain
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // no multisampling, so stick to 1 sample
    // loadOp and storeOp determine what to do with the data in the attachment before rendering and after rendering
    /*
     * following choices for loadOp:
     * VK_ATTACHMENT_LOAD_OP_LOAD: preserve the existing contents of the attachment
     * CK_ATTACHMENT_LOAD_OP_CLEAR: clear the values to a constant at the start
     * VK_ATTACHMENT_LOAD_OP_DONT_CARE: existing contents are undefined; we dont care about them
     */
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    /*
     * choices for storeOp:
     * VK_ATTACHMENT_STORE_OP_STORE: rendered contents will be stored in memory and can be read later
     * VK_ATTACHMENT_STORE_OP_DONT_CARE: contents of the framebuffer will be undefined after the rendering operation
     */
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // define stencil Load and Store Op
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // define pixel layout
    /*
     * layout options are:
     * VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
     * VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Images to be presented in the swap chain
     * VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: Images to be used as destination for a memory opertaion
     */
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // define attachment reference
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0; // specifies which attachment to reference by its index in the attachment desc array
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // intended to use as a color buffer

    // define subpass using subpass description structure
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void HelloTriangle::createGraphicsPipeline() {
    auto vertShaderCode = readFile("shaders/triangle_vert.spv");
    auto fragShaderCode = readFile("shaders/triangle_frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

    // to use shaders, must assign them to a specific pipeline stage through a VkPipelineShaderStageCreateInfo struct
    VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo = {};
    vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageCreateInfo.module = vertShaderModule;
    vertShaderStageCreateInfo.pName = "main";

    // do the same with frag shader
    VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo = {};
    fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageCreateInfo.module = fragShaderModule;
    fragShaderStageCreateInfo.pName = "main";

    // combine into a single array containing the two structs
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageCreateInfo, fragShaderStageCreateInfo};

    // describe format of the vertex data that will be passed to the vertex shader
    /*
     * This is described in two ways:
     * Bindings: spacing between data and whether the data is per-vertex or per-instance
     * Attribute Descriptions: type of the attributes passed to the vertex shader, which binding to load them from and at which offset
     */
    // this application loads vertex data directly in the vertex shader, so specify no vertex data to load for now
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr; // optional, point to an array of structs that describe the details for loading vertex data
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr; // optional, point to an array of structs that describe the details for loading vertex data

    // input assembly struct to describe what kind of geometry will be drawn from the vertices and if primitive restart should be enabled
    /*
     * The former is specified in the topology member and can have values like:
     * VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
     * VK_PRIMITIVE_TOPOLOGY_LINE_LIST: line from every 2 vertices without reuse
     * VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: the end vertex of every line is used as start vertex for the next line
     * VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: triangle from every 3 vertices without reuse
     * VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: the second and third vertex of every triangle are used as first two vertices of the next triangle
     */
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // define viewport
    // the region will almost always be (0,0) to (width, height)
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    // depth values must be within 0 to 1, tho min can be higher than max
    // usually just use default 0 and 1
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    // to draw entire framebuffer, define a scissor rectangle that covers it entirely
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;

    // create struct to define dynamic states
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // only need to specify viewport and scissor count during pipeline creation time
    /* creating them here would look like this:
    *   VkPipelineViewportStateCreateInfo viewportState{};
    *   viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    *   viewportState.viewportCount = 1;
    *   viewportState.pViewports = &viewport;
    *   viewportState.scissorCount = 1;
    *   viewportState.pScissors = &scissor;
    *   they would be immutable and any changes to these values would require a new pipeline to be created with new values
    */
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // define rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE; // if true, frags beyond the near and far planes are clamped to them as opposed to discarding them
    rasterizer.rasterizerDiscardEnable = VK_FALSE; // if true, the geometry never passes through the rasterizer stage (basically disables any output to the frame buffer)
    /*
     * polygon mode determines how fragments are generated for geometry.
     * VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
     * VK_POLYGON_MODE_LINE: polygon edges are drawn as lines
     * VK_POLYGON_MODE_POINT: polygon vertices are drawn as points
     * (using any other mode than fill requires a GPU feature)
     */
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f; // defines thickness of lines in terms of number of fragments (any number greater than 1.0 requires wideLines GPU feature)
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // type of face culling to use (backface culling enabled here)
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // specifies the vertex order for faces to be considered front-facing)
    rasterizer.depthBiasEnable = VK_FALSE; // rasterizer can alter depth values by adding a constant value or biasing them based on fragment slope (useful for shadow mapping)
    rasterizer.depthBiasConstantFactor = 0.0f; // optional
    rasterizer.depthBiasClamp = 0.0f; // optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // optional

    // define multisampling
    // (enabling requires a GPU feature)
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // optional
    multisampling.pSampleMask = nullptr; // optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // optional
    multisampling.alphaToOneEnable = VK_FALSE; // optional

    // you would define depth and/or stencil testing here

    // define color blending
    // mix the old and new value to produce a final color
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
    /*
     * (combine the old and new value using a bitwise operation)
     * colorBlendAttachment.blendEnable = VK_TRUE;
     * colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
     * colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
     * colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
     * colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
     * colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
     * colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
     */

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    // finally, create the pipeline layout object (fill out its form first)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // optional
    pipelineLayoutInfo.setLayoutCount = 0; // optional
    pipelineLayoutInfo.pSetLayouts = nullptr; // optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // optional

    if(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    // create full graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // start by referencing the array of shader stage structs
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    // then reference all of the structures describing the fixed-function stage
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    // pipeline layout
    pipelineInfo.layout = pipelineLayout;
    // reference the render pass and the index of the sub pass
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    // vulkan allows creation of a new graphics pipeline by deriving from an existing pipeline
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // optional
    pipelineInfo.basePipelineIndex = -1; // optional

    // finally, create object
    if(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void HelloTriangle::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for(size_t i = 0; i < swapChainImages.size(); ++i) {
        // create struct for each image view
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];

        // specify how the image data should be interpreted
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // treat images as: 1D textures, 2D textures, 3D textures and cube maps
        createInfo.format = swapChainImageFormat;

        // can swizlle color channels around (for example mapping all channels to the red channel for a monochrome texture; can map 0 or 1 as well)
        // sticking to default mapping for this application
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // describe images prupose and which part of the image should be accessed
        // these will be used as color targets without any mipmapping levels or multiple layers
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if(vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain image view");
        }
    }
}

void HelloTriangle::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

    // also decide how many images we would like to have in the swap chain
    // query for minimum number that it requires to function
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount;
    // however, this can lead to waiting on driver to complete internal opertaions, so it is recommended to always have 1 extra
    imageCount += 1;
    // but ALSO make sure not to exceed the maximum number of images while doing this
    // (0 is a special value that means there is no maximum)
    if(swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    // As usual, must fill out struct to create the object
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1; // amount of layers each image consists of (always 1 - unless developing a stereoscopic 3D application)
    // imageUsage specifies what kind of operations application will use the images in the swap chain for
    // here, used as a color attachment since the application will render directly to them
    // also possible to render images to a seperate image first to perform operations like post-processing, in such a case one might use VK_IMAGE_USAGE_TRANSFER_DST_BIT and use a memory operation to transfer the rendered image to a swap chain image.
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // specify how to handle swap chain images that will be used across multiple queue families
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    // for this application, avoid need for direct transfer of ownership
    if(indices.graphicsFamily != indices.presentFamily) {
        // images can be used across multiple queue families without explicit ownership transfers
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        // image is owned by one queue family at a time and ownership must be explicitly transferred before using it in another queue family (best performance)
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // optional
        createInfo.pQueueFamilyIndices = nullptr; // optional
    }

    // can specify certain transform should be applied to images in the swap chain (if it is supported)
    // to speicfy no transformation, specify current transformation
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

    // specify alpha channel for blending (almost always want to simply ignore the alpha channel)
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    // set the present mode (speaks for itself)
    createInfo.presentMode = presentMode;

    // set clipping mode
    createInfo.clipped = VK_TRUE;

    // possible for swap chain to become invalid or unoptimized while application is running (for example because the window was resized)
    // in this case, it needs to be recreated from scratch and a reference to the old one must be put in the oldSwapChain field.
    // complex topic - this application will simply assume we only ever create one swap chain.
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    // create the object
    if(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    // get VkImages in the swap chain
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    // set format and extent member variables
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}


/*
 * CREATING A SURFACE DIRECTLY - without GLFW (exposing win32 functions and calling them)
 *
 * STEP 1 - includes required:
 * #define CK_USE_PLATFORM_WIN32_KHR
 * #define GLFW_INCLUDE_VULKAN
 * #include <GLFW/glfw3.h>
 * #define GLFW_EXPOSE_NATIVE_WIN32
 * #include <GLFW/glfw3native.h>
 *
 * STEP 2 - populate struct
 * VkWin32SurfaceCreateInfoKHR createINfo{};
 * createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
 * createInfo.hwnd = glfwGetWin32Window(window);
 * createInfo.hinstance = GetModuleHandle(nullptr);
 *
 * STEP 3 - create surface
 * if(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS){
 *      throw std::runtime_error("failed to create window surface!");
 * }
 * (on linux, this is similarly done with vkCreateXcbSurfaceKHR)
 */
void HelloTriangle::createSurface() {
    if(glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void HelloTriangle::createLogicalDevice() {
    // query for available queue families
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

    // create a set of all unique queue families that are necessary for the required queues
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    // assign priority to queue to influence scheduling of command buffer execution
    float queuePriority = 1.0f;

    for(uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // specify the set of device features to use; Queried with vkGetPhysicalDeviceFeatures
    VkPhysicalDeviceFeatures deviceFeatures{};

    // create the main create info structure
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    // VkDeviceCreateInfo points at the vector
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    // enable device extensions
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // enabledLayerCount and ppEnabledLayerNames are ignored by up-to-date implementations, still good to set them anyways
    if(enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    // all done... create the vKCreateDevice
    if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    // retrieve queue handles
    // parameters are logical device, queue family, queue index, and a pointer to the variable to store queue handle in
    // only creating a single queue from this family - simply use index 0
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    // also retrieve presentation queue handle
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void HelloTriangle::pickPhysicalDevice() {
    // query the number of available graphics cards
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    // sanity check: there is at least 1 graphics card available that supports vulkan
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support");
    }

    // allocate array to hold all physical device handles
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // check for a physical device that meets requirements
    for(const auto& device : devices) {
        if(isDeviceSuitable(device, surface)) {
            physicalDevice = device;
            break;
        }
    }

    if(physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU");
    }
}

void HelloTriangle::createInstance() {
    // Check if using validation layers and if they exist
    if(enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers requested, but not available!");
    }

    // Application Information struct
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; // value identifying this structure
    // appInfo.pNext <-- is nullptr OR a pointer to a structure extending this structure
    appInfo.pApplicationName = "Hello Triangle"; // the name of the application
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0); // developer-supplied version number of the application
    appInfo.pEngineName = "No Engine"; // name of the engine (if any) used to create the application
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0); // developer-supplied version number of the engine used
    appInfo.apiVersion = VK_API_VERSION_1_0; // MUST be the HIGHEST version of Vulkan that the application is designed to use

    // Create Information struct
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; // value identifying structure type
    createInfo.pApplicationInfo = &appInfo; // pointer to a VkApplicationInfo structure

    // createInfo.pNext <-- nullptr OR a pointer to a structure extending this structure
    // createInfo.flags <-- bitmask of VkInstanceCreateFlagBits indicating the behavior of the instance

    // call helper function to get list of required extensions
    auto extensions = getRequiredExtensions();

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size()); // number of global extensions to enable
    createInfo.ppEnabledExtensionNames = extensions.data(); // pointer to an array of enabledExtensionCount containing the names of extensions to enable

    // create a debug messenger (done outside the if statement so that it is not destroyed too early)
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

    // enable validation layers, or set them to 0
    if(enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size()); // number of global layers to enable
        createInfo.ppEnabledLayerNames = validationLayers.data(); // pointer to an array of names of layers to enable

        // populate the debug messenger
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    // call helper function to validate extensions
    validateExtensions(extensions.data(), static_cast<uint32_t>(extensions.size()));

    // Create a result from an instance creation call
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

    /* General pattern for Vulkan object creation function parameters
     * 1. Pointer to struct with creation info
     * 2. Pointer to custom allocator callbacks
     * 3. Pointer to the variable that stores the handle to the new object
     */

    //

    // Error check the result - these always return either VK_SUCCESS or an error code
    if(result != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}



void HelloTriangle::setupDebugMessenger() {
    if(!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    // using the function created in the header (above class definition), create the extension object
    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

std::vector<const char *> HelloTriangle::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0; // number of extensions needed by glfw
    const char** glfwExtensions; // for creating an array of extension names

    // glfw function to grab the extensions and update the extension count
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // create vector for extensions
    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if(enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void HelloTriangle::validateExtensions(const char** extensions, uint32_t num_extensions) {
    uint32_t extensionCount = 0;

    // request just the number of extensions (not needed, just for reference)
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    // container of VkExtensionProperties to hold available extensions
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);

    // query for extension details
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    // For reference, print all the extension details
    std::cout << "available extensions:\n";
    for(const auto& extension : availableExtensions) {
        std::cout << "\t" << extension.extensionName << "\n";
    }

    // CHALLENGE: check if all extensions returned by glfwGetRequiredInstanceExtensions are included in the supported extensions list
    std::cout << "required extensions:\n";
    // for each extension needed by glfw, check if it exists. if not, throw error
    for(auto i = 0; i < num_extensions; ++i) {
        // For reference, also print all required extension details
        std::cout << "\t" << extensions[i] << "\n";

        bool found = false;
        for(const auto& availableExtension : availableExtensions) {
            if(strcmp(extensions[i], availableExtension.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if(!found) {
            throw std::runtime_error("extension required by glfw not found!");
        }
    }
}

bool HelloTriangle::checkValidationLayerSupport() {
    // first step is to compile a list of all available layers
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // check if all the layers in validationLayers exist in the availableLayers
    for(const char* layerName : validationLayers) {
        bool layerFound = false;

        for(const auto& layerProperties : availableLayers) {
            if(strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if(!layerFound) {
            return false;
        }
    }

    return true;
}

void HelloTriangle::mainLoop() {
    // standard glfw input poll
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void HelloTriangle::cleanup() {
    // destory frame buffers
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    // destory pipeline
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    // destroy pipeline layout
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    // destory render pass
    vkDestroyRenderPass(device, renderPass, nullptr);

    // destory image views (did not need to destroy images as they were not explicitly created by me)
    for(auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    // swap chain must be destroyed before the device
    vkDestroySwapchainKHR(device, swapChain, nullptr);

    // Can destroy this before instance since logical devices do not interact directly with instances
    vkDestroyDevice(device, nullptr);

    // If the debug validation layer is active, need to destroy that
    // Need to destroy it first since it is directly tied to the instance
    if(enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    // Destroy Vulkan surface (created through GLFW here - but GLFW does not provide special function for destroying a surface)
    vkDestroySurfaceKHR(instance, surface, nullptr);

    // Destory vulkan instance
    vkDestroyInstance(instance, nullptr);

    // standard glfw destroy window
    glfwDestroyWindow(window);

    // standard glfw terminate
    glfwTerminate();
}

