/*
 * Copyright (C) 2021 eYs3D Corporation
 * All rights reserved.
 *
 * ColorPaletteConfig - Configuration manager for color palette zNear/zFar overrides
 *
 * This class allows users to override the default zNear (calculated from ZDTable)
 * and zFar (default 1000) values through a configuration file.
 *
 * Config file location: settings/APC_ColorPaletteConfig_Default.ini
 *
 * Device-specific sections:
 *   [ColorPalette.80363]  - Device-specific configuration
 *
 * NOTE: This class uses inih library (https://github.com/benhoyt/inih) for INI parsing.
 */

#ifndef COLORPALETTECONFIG_H
#define COLORPALETTECONFIG_H

#include <string>
#include <map>
#include <vector>

// Config file path (relative to bin directory)
#define COLORPALETTE_CONFIG_FILENAME "./../settings/APC_ColorPaletteConfig_Default.ini"

// Config section and key names
#define COLORPALETTE_CFG_SECTION_GLOBAL "ColorPalette"
#define COLORPALETTE_CFG_ZNEAR "zNear"
#define COLORPALETTE_CFG_ZFAR "zFar"

// Validation limits
// Color palette has maximum 16384 entries (0-16383), matching COLOR_PALETTE_MAX_COUNT
#define COLORPALETTE_MAX_Z_VALUE 16383
#define COLORPALETTE_MIN_Z_VALUE 0
#define COLORPALETTE_DEFAULT_ZFAR 1000

class ColorPaletteConfig
{
public:
    // Device-specific configuration values
    struct DeviceConfig {
        int zNear;      // 0 = use ZDTable calculated value
        int zFar;       // 0 = use default (1000)
        bool isValid;
        std::string errorMessage;

        DeviceConfig() : zNear(0), zFar(0), isValid(true) {}
    };

    // Singleton access
    static ColorPaletteConfig* GetInstance();

    // Load configuration from file (call once at startup)
    bool LoadConfig();

    // Get device-specific configuration
    // Returns config for device section if exists, otherwise global section
    DeviceConfig GetDeviceConfig(const std::string& deviceModel) const;

    // Check if config was loaded successfully
    bool IsConfigLoaded() const { return m_configLoaded; }

    // Apply configuration for a specific device
    // deviceModel: e.g., "80363"
    // zdTableZNear: calculated zNear from ZDTable
    // defaultZFar: default zFar value (typically 1000)
    // Returns adjusted zNear and zFar through reference parameters
    void ApplyConfig(const std::string& deviceModel,
                     int zdTableZNear, int defaultZFar,
                     int& outZNear, int& outZFar);

    // Validate a single value against limits
    static bool ValidateZValue(int value, std::string& errorMsg);

    // Validate zNear/zFar pair (zFar must be > zNear if both are set)
    static bool ValidateZRange(int zNear, int zFar, std::string& errorMsg);

private:
    ColorPaletteConfig();
    ~ColorPaletteConfig() = default;

    // Prevent copying
    ColorPaletteConfig(const ColorPaletteConfig&) = delete;
    ColorPaletteConfig& operator=(const ColorPaletteConfig&) = delete;

    // Load configuration from file using inih library
    bool LoadFromFile(const std::string& filePath);

    // Validate device configuration
    bool ValidateDeviceConfig(DeviceConfig& config);

    // Utility functions
    static bool StartsWith(const std::string& str, const std::string& prefix);
    static bool FileExists(const std::string& filePath);
    static std::string ToLower(const std::string& str);

    // State
    bool m_configLoaded;
    std::string m_loadedFilePath;

    // Global configuration (fallback)
    DeviceConfig m_globalConfig;

    // Device-specific configurations (key = device model, e.g., "80363")
    std::map<std::string, DeviceConfig> m_deviceConfigs;
};

#endif // COLORPALETTECONFIG_H
