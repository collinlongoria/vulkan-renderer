// Collin Longoria
// 10/16/24

#include "HelloTriangle.h"

#include <iostream>
#include <stdexcept>
#include <cstring>

// constants for window resolution
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

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

    // enable validation layers, or set them to 0
    if(enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size()); // number of global layers to enable
        createInfo.ppEnabledLayerNames = validationLayers.data(); // pointer to an array of names of layers to enable
    }
    else {
        createInfo.enabledLayerCount = 0;
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

    // Error check the result - these always return either VK_SUCCESS or an error code
    if(result != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void HelloTriangle::setupDebugMessenger() {
    if(!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // Optional

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
    // Destory vulkan instance
    vkDestroyInstance(instance, nullptr);

    // standard glfw destroy window
    glfwDestroyWindow(window);

    // standard glfw terminate
    glfwTerminate();
}

