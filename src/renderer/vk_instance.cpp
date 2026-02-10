#include "vk_instance.h"
#include "core/log.h"
#include "platform/window.h"
#include "utils/macros.h"
#include "memory/arena.h"
#include <string.h>

// Validation layer name
static const char *VALIDATION_LAYERS[]  = {"VK_LAYER_KHRONOS_validation"};
static const u32 VALIDATION_LAYER_COUNT = 1;

// Debug callback
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
  VkDebugUtilsMessageSeverityFlagBitsEXT severity,
  VkDebugUtilsMessageTypeFlagsEXT type,
  const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data)
{
  (void)type;
  (void)user_data;

  if(severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    LOG_ERROR("[Vulkan] %s", callback_data->pMessage);
  } else if(severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    LOG_WARN("[Vulkan] %s", callback_data->pMessage);
  } else if(severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    LOG_INFO("[Vulkan] %s", callback_data->pMessage);
  } else {
    LOG_DEBUG("[Vulkan] %s", callback_data->pMessage);
  }

  return VK_FALSE;
}

// Create debug messenger
static VkResult
create_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT *messenger)
{
  VkDebugUtilsMessengerCreateInfoEXT create_info = {
    .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                       | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                   | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                   | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = debug_callback,
    .pUserData       = NULL,
  };

  // Load function
  PFN_vkCreateDebugUtilsMessengerEXT func
    = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");

  if(func) {
    return func(instance, &create_info, NULL, messenger);
  }

  return VK_ERROR_EXTENSION_NOT_PRESENT;
}

// Destroy debug messenger
static void
destroy_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger)
{
  PFN_vkDestroyDebugUtilsMessengerEXT func
    = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");

  if(func) {
    func(instance, messenger, NULL);
  }
}

bool vk_instance_check_validation_support(void)
{
  u32 layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, NULL);

  ArenaTemp scratch = arena_scratch_begin();
  VkLayerProperties *available_layers
    = ARENA_PUSH_ARRAY(scratch.arena, VkLayerProperties, layer_count);
  if(!available_layers) {
    arena_temp_end(scratch);
    return false;
  }

  vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

  bool all_found = true;
  for(u32 i = 0; i < VALIDATION_LAYER_COUNT; i++) {
    bool found = false;
    for(u32 j = 0; j < layer_count; j++) {
      if(strcmp(VALIDATION_LAYERS[i], available_layers[j].layerName) == 0) {
        found = true;
        break;
      }
    }
    if(!found) {
      all_found = false;
      break;
    }
  }

  arena_temp_end(scratch);
  return all_found;
}

VkResult
vk_instance_create(const VkInstanceConfig *config, VkInstanceContext *ctx)
{
  LOG_INFO("Creating Vulkan instance");

  ctx->instance           = VK_NULL_HANDLE;
  ctx->debug_messenger    = VK_NULL_HANDLE;
  ctx->validation_enabled = config->enable_validation;

  // Check validation layer support
  if(config->enable_validation && !vk_instance_check_validation_support()) {
    LOG_WARN("Validation layers requested but not available");
    ctx->validation_enabled = false;
  }

  // Application info
  VkApplicationInfo app_info = {
    .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName   = config->app_name,
    .applicationVersion = config->app_version,
    .pEngineName        = "No Engine",
    .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion         = VK_API_VERSION_1_2,
  };

  // Get required extensions
  u32 window_extension_count = 0;
  const char **window_extensions
    = window_get_required_extensions(&window_extension_count);
  if(!window_extensions || window_extension_count == 0) {
    LOG_ERROR("No window system Vulkan extensions available");
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  // Build extension list using scratch arena
  ArenaTemp scratch   = arena_scratch_begin();
  u32 extension_count = window_extension_count;
  const char **extensions
    = ARENA_PUSH_ARRAY(scratch.arena, const char *, extension_count + 2);
  if(!extensions) {
    arena_temp_end(scratch);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  for(u32 i = 0; i < window_extension_count; i++) {
    extensions[i] = window_extensions[i];
  }

  // Add portability extension for MoltenVK
  extensions[extension_count++]
    = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;

  if(ctx->validation_enabled) {
    extensions[extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
  }

  // Log extensions
  LOG_DEBUG("Required instance extensions:");
  for(u32 i = 0; i < extension_count; i++) {
    LOG_DEBUG("  - %s", extensions[i]);
  }

  // Instance create info
  VkInstanceCreateInfo create_info = {
    .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo        = &app_info,
    .enabledExtensionCount   = extension_count,
    .ppEnabledExtensionNames = extensions,
    .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
  };

  if(ctx->validation_enabled) {
    create_info.enabledLayerCount   = VALIDATION_LAYER_COUNT;
    create_info.ppEnabledLayerNames = VALIDATION_LAYERS;
    LOG_INFO("Validation layers enabled");
  } else {
    create_info.enabledLayerCount = 0;
  }

  // Create instance
  VkResult result = vkCreateInstance(&create_info, NULL, &ctx->instance);
  arena_temp_end(scratch);

  if(result != VK_SUCCESS) {
    LOG_ERROR("Failed to create Vulkan instance: %d", result);
    return result;
  }

  LOG_INFO("Vulkan instance created");

  // Create debug messenger
  if(ctx->validation_enabled) {
    result = create_debug_messenger(ctx->instance, &ctx->debug_messenger);
    if(result != VK_SUCCESS) {
      LOG_WARN("Failed to create debug messenger: %d", result);
    } else {
      LOG_INFO("Debug messenger created");
    }
  }

  return VK_SUCCESS;
}

void vk_instance_destroy(VkInstanceContext *ctx)
{
  if(ctx->debug_messenger != VK_NULL_HANDLE) {
    destroy_debug_messenger(ctx->instance, ctx->debug_messenger);
    LOG_DEBUG("Debug messenger destroyed");
  }

  if(ctx->instance != VK_NULL_HANDLE) {
    vkDestroyInstance(ctx->instance, NULL);
    LOG_INFO("Vulkan instance destroyed");
  }

  ctx->instance        = VK_NULL_HANDLE;
  ctx->debug_messenger = VK_NULL_HANDLE;
}
