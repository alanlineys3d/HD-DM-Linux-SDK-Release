/*
 * Copyright (C) 2021 eYs3D Corporation
 * All rights reserved.
 *
 * ColorPaletteConfig implementation using inih library
 *
 * ============================================================================
 * DEVELOPER GUIDE: Adding ColorPaletteConfig Support to New Device Modules
 * ============================================================================
 *
 * This configuration system allows device-specific zNear/zFar overrides for
 * color palette visualization. Follow these steps to add support for a new
 * camera module:
 *
 * STEP 1: Add INI Configuration Section
 * --------------------------------------
 * Edit: settings/APC_ColorPaletteConfig_Default.ini
 *
 * Add a new section with your device model name (case-insensitive):
 *
 *     [ColorPalette.YOUR_DEVICE]
 *     # YOUR_DEVICE specific configuration
 *     zNear=150
 *     zFar=1500
 *
 * Note: Section names are case-insensitive. [ColorPalette.HYPATIA] and
 * [ColorPalette.hypatia] are equivalent.
 *
 * STEP 2: Override PreparePointCloudInfo() in Device Model
 * ---------------------------------------------------------
 * Edit: model/module/CVideoDeviceModel_YourDevice.h
 *
 * Add the override declaration:
 *
 *     virtual int PreparePointCloudInfo() override;
 *
 * STEP 3: Implement the Override
 * -------------------------------
 * Edit: model/module/CVideoDeviceModel_YourDevice.cpp
 *
 * Add include:
 *
 *     #include "ColorPaletteConfig.h"
 *
 * Implement the method:
 *
 *     int CVideoDeviceModel_YourDevice::PreparePointCloudInfo()
 *     {
 *         // Call base class implementation first
 *         int ret = CVideoDeviceModel::PreparePointCloudInfo();
 *         if (ret != APC_OK) {
 *             return ret;
 *         }
 *
 *         // Apply ColorPaletteConfig overrides
 *         int nZNear, nZFar;
 *         m_pVideoDeviceController->GetPreviewOptions()->GetZRange(nZNear, nZFar);
 *
 *         int configuredZNear, configuredZFar;
 *         ColorPaletteConfig::GetInstance()->ApplyConfig(
 *             "YOUR_DEVICE",          // Must match INI section name
 *             m_zdTableInfo.nZNear,   // ZDTable calculated zNear
 *             nZFar,                  // Current zFar (default 1000)
 *             configuredZNear,        // Output: final zNear
 *             configuredZFar          // Output: final zFar
 *         );
 *
 *         m_pVideoDeviceController->GetPreviewOptions()->SetZRange(
 *             configuredZNear, configuredZFar);
 *         m_pVideoDeviceController->AdjustZRange();
 *
 *         // Set DefaultZRange so reset button uses configured values
 *         m_pVideoDeviceController->GetPreviewOptions()->SetDefaultZRange(
 *             configuredZNear, configuredZFar);
 *
 *         return APC_OK;
 *     }
 *
 * REFERENCE IMPLEMENTATION:
 * -------------------------
 * See CVideoDeviceModel_80363.cpp for a complete working example.
 *
 * CONFIGURATION BEHAVIOR:
 * -----------------------
 * - zNear=0 in INI: Uses ZDTable calculated minimum
 * - zFar=0 in INI:  Uses default value (1000mm)
 * - Values are clamped to valid range [1, 16383] mm
 * - zFar must be greater than zNear (auto-adjusted if needed)
 * - Device sections override global [ColorPalette] section
 *
 * ============================================================================
 */

#include "ColorPaletteConfig.h"
#include "inih/INIReader.h"
#include <fstream>
#include <cstdio>
#include <algorithm>
#include <cctype>

ColorPaletteConfig* ColorPaletteConfig::GetInstance()
{
    static ColorPaletteConfig instance;
    return &instance;
}

ColorPaletteConfig::ColorPaletteConfig()
    : m_configLoaded(false)
{
}

bool ColorPaletteConfig::FileExists(const std::string& filePath)
{
    std::ifstream f(filePath.c_str());
    return f.good();
}

bool ColorPaletteConfig::StartsWith(const std::string& str, const std::string& prefix)
{
    return str.size() >= prefix.size() &&
           str.compare(0, prefix.size(), prefix) == 0;
}

std::string ColorPaletteConfig::ToLower(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool ColorPaletteConfig::LoadConfig()
{
    if (FileExists(COLORPALETTE_CONFIG_FILENAME)) {
        fprintf(stdout, "[ColorPaletteConfig] Loading config: %s\n",
                COLORPALETTE_CONFIG_FILENAME);
        return LoadFromFile(COLORPALETTE_CONFIG_FILENAME);
    } else {
        fprintf(stdout, "[ColorPaletteConfig] No config file found, using defaults (zNear=ZDTable, zFar=1000)\n");
        m_configLoaded = false;
        return false;
    }
}

bool ColorPaletteConfig::LoadFromFile(const std::string& filePath)
{
    INIReader reader(filePath);

    if (reader.ParseError() != 0) {
        fprintf(stderr, "[ColorPaletteConfig] Failed to parse config file: %s (error: %d)\n",
                filePath.c_str(), reader.ParseError());
        return false;
    }

    m_loadedFilePath = filePath;
    m_deviceConfigs.clear();

    // Load global [ColorPalette] section first
    m_globalConfig.zNear = reader.GetInteger(COLORPALETTE_CFG_SECTION_GLOBAL, COLORPALETTE_CFG_ZNEAR, 0);
    m_globalConfig.zFar = reader.GetInteger(COLORPALETTE_CFG_SECTION_GLOBAL, COLORPALETTE_CFG_ZFAR, 0);

    if (!ValidateDeviceConfig(m_globalConfig)) {
        fprintf(stderr, "[ColorPaletteConfig] Global section validation failed: %s\n",
                m_globalConfig.errorMessage.c_str());
        m_globalConfig = DeviceConfig(); // Reset to defaults
    }

    fprintf(stdout, "[ColorPaletteConfig] Global config: zNear=%d, zFar=%d\n",
            m_globalConfig.zNear, m_globalConfig.zFar);

    // Load device-specific sections [ColorPalette.XXXXX]
    // Note: INIReader converts section names to lowercase internally
    std::string sectionPrefix = ToLower(std::string(COLORPALETTE_CFG_SECTION_GLOBAL) + ".");

    // Get all sections from INIReader (returns lowercase names)
    std::vector<std::string> sections = reader.Sections();

    for (const auto& sectionName : sections) {
        if (StartsWith(sectionName, sectionPrefix)) {
            // Extract device model from section name (e.g., "colorpalette.80363" -> "80363")
            std::string deviceModel = sectionName.substr(sectionPrefix.length());

            DeviceConfig deviceConfig;
            deviceConfig.zNear = reader.GetInteger(sectionName, COLORPALETTE_CFG_ZNEAR, 0);
            deviceConfig.zFar = reader.GetInteger(sectionName, COLORPALETTE_CFG_ZFAR, 0);

            if (ValidateDeviceConfig(deviceConfig)) {
                m_deviceConfigs[deviceModel] = deviceConfig;
                fprintf(stdout, "[ColorPaletteConfig] Device %s config: zNear=%d, zFar=%d\n",
                        deviceModel.c_str(), deviceConfig.zNear, deviceConfig.zFar);
            } else {
                fprintf(stderr, "[ColorPaletteConfig] Device %s config validation failed: %s\n",
                        deviceModel.c_str(), deviceConfig.errorMessage.c_str());
            }
        }
    }

    m_configLoaded = true;

    fprintf(stdout, "[ColorPaletteConfig] Loaded %zu device-specific config(s)\n", m_deviceConfigs.size());

    return true;
}

bool ColorPaletteConfig::ValidateDeviceConfig(DeviceConfig& config)
{
    config.errorMessage.clear();
    config.isValid = true;

    // Validate zNear
    if (config.zNear != 0) {
        std::string error;
        if (!ValidateZValue(config.zNear, error)) {
            config.errorMessage = "zNear: " + error;
            config.isValid = false;
            return false;
        }
    }

    // Validate zFar
    if (config.zFar != 0) {
        std::string error;
        if (!ValidateZValue(config.zFar, error)) {
            config.errorMessage = "zFar: " + error;
            config.isValid = false;
            return false;
        }
    }

    // Validate range: zFar must be > zNear if both are set
    if (config.zNear > 0 && config.zFar > 0) {
        std::string error;
        if (!ValidateZRange(config.zNear, config.zFar, error)) {
            config.errorMessage = error;
            config.isValid = false;
            return false;
        }
    }

    return true;
}

bool ColorPaletteConfig::ValidateZValue(int value, std::string& errorMsg)
{
    char buffer[256];

    if (value < COLORPALETTE_MIN_Z_VALUE) {
        snprintf(buffer, sizeof(buffer), "Value %d is below minimum %d",
                 value, COLORPALETTE_MIN_Z_VALUE);
        errorMsg = buffer;
        return false;
    }

    if (value > COLORPALETTE_MAX_Z_VALUE) {
        snprintf(buffer, sizeof(buffer), "Value %d exceeds maximum %d (color palette limit)",
                 value, COLORPALETTE_MAX_Z_VALUE);
        errorMsg = buffer;
        return false;
    }

    return true;
}

bool ColorPaletteConfig::ValidateZRange(int zNear, int zFar, std::string& errorMsg)
{
    if (zFar <= zNear) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "zFar (%d) must be greater than zNear (%d)",
                 zFar, zNear);
        errorMsg = buffer;
        return false;
    }
    return true;
}

ColorPaletteConfig::DeviceConfig ColorPaletteConfig::GetDeviceConfig(const std::string& deviceModel) const
{
    // Check for device-specific config first
    // Use lowercase for case-insensitive matching (INIReader stores sections in lowercase)
    std::string lowerModel = ToLower(deviceModel);
    auto it = m_deviceConfigs.find(lowerModel);
    if (it != m_deviceConfigs.end()) {
        return it->second;
    }

    // Fall back to global config
    return m_globalConfig;
}

void ColorPaletteConfig::ApplyConfig(const std::string& deviceModel,
                                      int zdTableZNear, int defaultZFar,
                                      int& outZNear, int& outZFar)
{
    // Load config if not already loaded (lazy initialization)
    if (!m_configLoaded) {
        LoadConfig();
    }

    // Get device-specific or global config
    DeviceConfig config = GetDeviceConfig(deviceModel);

    // Use lowercase for case-insensitive matching (INIReader stores sections in lowercase)
    bool hasDeviceConfig = (m_deviceConfigs.find(ToLower(deviceModel)) != m_deviceConfigs.end());

    // Determine zNear
    if (config.zNear > 0) {
        outZNear = config.zNear;
        fprintf(stdout, "[ColorPaletteConfig] %s: Using %s config zNear: %d\n",
                deviceModel.c_str(), hasDeviceConfig ? "device" : "global", outZNear);
    } else {
        outZNear = zdTableZNear;
        fprintf(stdout, "[ColorPaletteConfig] %s: Using ZDTable calculated zNear: %d\n",
                deviceModel.c_str(), outZNear);
    }

    // Determine zFar
    if (config.zFar > 0) {
        outZFar = config.zFar;
        fprintf(stdout, "[ColorPaletteConfig] %s: Using %s config zFar: %d\n",
                deviceModel.c_str(), hasDeviceConfig ? "device" : "global", outZFar);
    } else {
        outZFar = defaultZFar;
        fprintf(stdout, "[ColorPaletteConfig] %s: Using default zFar: %d\n",
                deviceModel.c_str(), outZFar);
    }

    // Final safety check: ensure values are within palette limits
    if (outZNear > COLORPALETTE_MAX_Z_VALUE) {
        fprintf(stderr, "[ColorPaletteConfig] zNear %d exceeds max, clamping to %d\n",
                outZNear, COLORPALETTE_MAX_Z_VALUE);
        outZNear = COLORPALETTE_MAX_Z_VALUE;
    }
    if (outZFar > COLORPALETTE_MAX_Z_VALUE) {
        fprintf(stderr, "[ColorPaletteConfig] zFar %d exceeds max, clamping to %d\n",
                outZFar, COLORPALETTE_MAX_Z_VALUE);
        outZFar = COLORPALETTE_MAX_Z_VALUE;
    }

    // Ensure zFar > zNear
    if (outZFar <= outZNear) {
        fprintf(stderr, "[ColorPaletteConfig] zFar <= zNear, adjusting zFar to %d\n",
                outZNear + 1);
        outZFar = outZNear + 1;
        if (outZFar > COLORPALETTE_MAX_Z_VALUE) {
            outZFar = COLORPALETTE_MAX_Z_VALUE;
            outZNear = COLORPALETTE_MAX_Z_VALUE - 1;
        }
    }

    fprintf(stdout, "[ColorPaletteConfig] %s: Applied values: zNear=%d, zFar=%d\n",
            deviceModel.c_str(), outZNear, outZFar);
}
