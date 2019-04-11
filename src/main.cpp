#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <array>
#include <vector>
#include <cassert>
#include <algorithm>

const int WIDTH = 800;
const int HEIGHT = 600;

const std::array<const char*, 1> validation_layers = {
    "VK_LAYER_LUNARG_standard_validation"
};

#ifdef NDEBUG
const bool enable_vk_validation = false;
#else
const bool enable_vk_validation = true;
#endif

bool check_validation_layer_support() 
{
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for (const char* layer_name: validation_layers) 
    {
        if (available_layers.end() == std::find_if(available_layers.begin(), available_layers.end(), 
                                                    [layer_name] (const VkLayerProperties& other) { 
                                                        return strcmp(layer_name, other.layerName) == 0;
                                                    }))
        {
            return false;
        }
    }

    return true;
}

std::vector<const char*> get_required_extensions()
{
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
    if (enable_vk_validation)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }   

    return extensions;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    void* p_user_data
)
{
    std::cerr << "Validation layer ";
    if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        std::cerr << "WARNING";
    }
    else if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        std::cerr << "INFO";
    }
    else if (message_severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        std::cerr << "ERROR";
    }
    
    std::cerr << ": " << p_callback_data->pMessage << std::endl;
    return VK_FALSE; 
}

static VkDebugUtilsMessengerEXT create_debug_messenger(VkInstance instance)
{
    VkDebugUtilsMessengerCreateInfoEXT create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debug_callback;
    create_info.pUserData = nullptr;

    VkDebugUtilsMessengerEXT debug_messenger;
    if (vkCreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &debug_messenger) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create vk debug messenger");
    }

    return debug_messenger;
}

static VkInstance create_instance()
{
    if (enable_vk_validation)
    {
        if (!check_validation_layer_support())
        {
            throw std::runtime_error("Validation layers requested but not available");
        }
    }

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Triangle";
    app_info.applicationVersion = 1;
    app_info.pEngineName = "";
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_MAKE_VERSION(1, 0, VK_HEADER_VERSION);

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    if (enable_vk_validation)
    {
        create_info.enabledLayerCount = validation_layers.size();
        create_info.ppEnabledLayerNames = validation_layers.data();
    }
    else
    {
        create_info.enabledLayerCount = 0;
    }

    std::vector<const char*> extensions = get_required_extensions();
    create_info.enabledExtensionCount = extensions.size();
    create_info.ppEnabledExtensionNames = extensions.data();

    VkInstance instance;
    if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create vk instance");
    }

    return instance;
}

struct VulkanApp
{
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
};

static VulkanApp create_vulkan_app()
{
    VulkanApp app;
    app.instance = create_instance();
    if (enable_vk_validation)
        app.debug_messenger = create_debug_messenger(app.instance);
}

static void destroy_vulkan_app(const VulkanApp& app)
{
    if (enable_vk_validation)
        vkDestroyDebugUtilsMessengerEXT(app.instance, app.debug_messenger, nullptr);
    
    vkDestroyInstance(app.instance, nullptr);
}

int main()
{
    GLFWwindow *window;
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

    VulkanApp vk_app = create_vulkan_app();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    destroy_vulkan_app(vk_app);

    glfwDestroyWindow(window);

    glfwTerminate();

    return EXIT_SUCCESS;
}