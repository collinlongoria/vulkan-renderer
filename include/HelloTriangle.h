// Collin Longoria
// 10/16/24

#ifndef HELLOTRIANGLE_H
#define HELLOTRIANGLE_H

/* Could call #include <vulkan/vulkan.h>
 * but using this GLFW define does that for us
 * so no need to
 */
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

class HelloTriangle {
public:
    // FUNCTIONS
    void run();
private:
    // FUNCTIONS
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

    // HELPER FUNCTIONS FOR VULKAN RENDERING
    void createInstance(); // creates the vulkan instance
    std::vector<const char *> getRequiredExtensions(); // gets a list of all required extensions
    void validateExtensions(const char** extensions, uint32_t num_extensions); // validates all extensions are available
    bool checkValidationLayerSupport(); // checks for validation layer support
    void setupDebugMessenger(); // creates the debug messenger

    // VARIABLES
    GLFWwindow* window; // The main window using GLFW
    VkInstance instance; // The instance is a connection between application and the Vulkan library
    VkDebugUtilsMessengerEXT debugMessenger; // The debug messenger informs Vulkan about the callback function
};

#endif //HELLOTRIANGLE_H
