// Collin Longoria
// 10/16/24

#include "HelloTriangle.h"

#include <iostream>
#include <stdexcept>
#include <cstring>
#include <optional>
#include <set>

// constants for window resolution
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;


/* Related to Device Extensions */
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
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

    // anything required should be returned as a boolean
    return deviceFeatures.geometryShader && indices.isComplete();

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

    // enabledLayerCount and ppEnabledLayerNames are ignored by up-to-date implementations, still good to set them anyways
    createInfo.enabledExtensionCount = 0;

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

