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
    void drawFrame();
    void cleanup();

    // HELPER FUNCTIONS FOR VULKAN RENDERING
    void createInstance(); // creates the vulkan instance
    std::vector<const char *> getRequiredExtensions(); // gets a list of all required extensions
    void validateExtensions(const char** extensions, uint32_t num_extensions); // validates all extensions are available
    bool checkValidationLayerSupport(); // checks for validation layer support
    void setupDebugMessenger(); // creates the debug messenger
    void pickPhysicalDevice(); // looks for and selects a graphics card in the system that supports required features
    void createLogicalDevice(); // creates a logical device
    void createSurface(); // creates the surface object
    void createSwapChain(); // creates the swap chain
    void createImageViews(); // creates the image views and populates the related vector
    void createRenderPass(); // create render pass object
    void createGraphicsPipeline(); // creates the graphics pipeline
    void createFramebuffers(); // creates frame buffers
    void createCommandPool(); // creates command pool
    void createCommandBuffer(); // creates the command buffer
    void recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index); // writes commands we want to execute into a command buffer
    void createSyncObjects();

    // VARIABLES
    GLFWwindow* window; // The main window using GLFW
    VkInstance instance; // The instance is a connection between application and the Vulkan library
    VkDebugUtilsMessengerEXT debugMessenger; // The debug messenger informs Vulkan about the callback function
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // Storage for selected graphics card
    VkDevice device; // logical device to interface with physical device
    VkQueue graphicsQueue; // handle to interface with queue
    VkSurfaceKHR surface; // Abstract type of surface to present rendered images to. Must be created right after instance creation, because it can influence physical device selection
    VkQueue presentQueue; // handle to interface with presentation queue
    VkSwapchainKHR swapChain; // swap chain object
    std::vector<VkImage> swapChainImages; // storage for handles to the VkImages in the swap chain
    VkFormat swapChainImageFormat; // image format of the swap chain
    VkExtent2D swapChainExtent; // extent of the swap chain
    std::vector<VkImageView> swapChainImageViews; // container for image views, so VkImage can be accessed
    VkRenderPass renderPass; // render pass object
    VkPipelineLayout pipelineLayout; // defines the rendering pipeline
    VkPipeline graphicsPipeline; // graphics pipeline object
    std::vector<VkFramebuffer> swapChainFramebuffers; // vector to hold the framebuffers
    VkCommandPool commandPool; // manages the memory that is used to store the buffers and command buffers are allocated from it
    VkCommandBuffer commandBuffer; // command buffer
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
};

#endif //HELLOTRIANGLE_H
