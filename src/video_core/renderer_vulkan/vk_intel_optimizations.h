// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vulkan/vulkan.hpp>

namespace Vulkan::IntelOptimizations {

// Intel Iris Xe-specific Vulkan optimization hints
// These leverage Intel's driver optimizations for integrated GPUs

struct IntelDeviceFeatures {
    bool is_intel_igpu = false;
    bool supports_memory_priority = false;
    bool supports_async_present = false;
    
    // Intel-specific performance hints
    static constexpr VkDeviceSize INTEL_IGPU_STAGING_THRESHOLD = 64_MB;
    static constexpr VkDeviceSize INTEL_IGPU_DEVICE_THRESHOLD = 128_MB;
    
    // Shared memory optimization thresholds
    static constexpr float INTEL_MEMORY_PRESSURE_RATIO = 0.75f;  // Use 75% max for iGPU
    static constexpr u32 INTEL_MAX_DESCRIPTOR_SETS = 4096;        // Lower for iGPU
    static constexpr u32 INTEL_PREFERRED_QUEUE_COUNT = 1;         // Single queue for iGPU
};

// Check if running on Intel Iris Xe
inline bool IsIntelIrisXe(vk::PhysicalDeviceProperties const& props) {
    if (props.vendorID != 0x8086) // Intel vendor ID
        return false;
        
    // Check for 11th/12th gen Iris Xe (deviceID range)
    const u32 device_id = props.deviceID;
    return (device_id >= 0x9A40 && device_id <= 0x9AFF) ||  // Tiger Lake
           (device_id >= 0x4600 && device_id <= 0x46FF);    // Alder/Rocket Lake
}

// Apply Intel-specific Vulkan memory allocation hints
inline VkMemoryPropertyFlags GetIntelOptimalMemoryFlags(bool is_intel_igpu, 
                                                         bool is_staging) {
    if (!is_intel_igpu) {
        return is_staging ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                         : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }
    
    // For Intel iGPU: prefer HOST_CACHED for staging (shared RAM)
    return is_staging ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                       VK_MEMORY_PROPERTY_HOST_CACHED_BIT     // Intel: cache coherent
                     : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | 
                       VK_MEMORY_PROPERTY_HOST_CACHED_BIT;    // Intel: still cached
}

// Get optimal present mode for Intel iGPU
inline vk::PresentModeKHR GetIntelOptimalPresentMode(std::vector<vk::PresentModeKHR> const& available_modes) {
    // FIFO is best for iGPU - built-in pacing, no tearing, lower CPU overhead.
    // Spec guarantees FIFO support.
    return vk::PresentModeKHR::eFifo;
}

// Get optimal swapchain image count for Intel iGPU  
inline u32 GetIntelOptimalSwapchainImageCount(vk::SurfaceCapabilitiesKHR const& caps) {
    // Double buffer for iGPU (saves ~512MB VRAM, lower latency)
    u32 count = 2;
    if (count < caps.minImageCount) count = caps.minImageCount;
    if (caps.maxImageCount > 0 && count > caps.maxImageCount) count = caps.maxImageCount;
    return count;
}

} // namespace Vulkan::IntelOptimizations
