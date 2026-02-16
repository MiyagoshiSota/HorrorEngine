#pragma once

#include <string>
#include <unordered_map>
#include <any>

class SettingStore
{
public:
    // Generic setter
    void SetValue(const std::string& key, std::any value);

    // Convenience setters
    void SetBool(const std::string& key, bool value);
    void SetInt(const std::string& key, int value);
    void SetFloat(const std::string& key, float value);
    void SetString(const std::string& key, const std::string& value);

    // Generic getter
    std::any GetValue(const std::string& key, const std::any& defaultValue = {}) const;

    // Convenience getters with default value
    bool GetBool(const std::string& key, bool defaultValue = false) const;
    int GetInt(const std::string& key, int defaultValue = 0) const;
    float GetFloat(const std::string& key, float defaultValue = 0.0f) const;
    std::string GetString(const std::string& key, const std::string& defaultValue = "") const;

    // Check if a key exists
    bool Has(const std::string& key) const;

private:
    std::unordered_map<std::string, std::any> m_settings;
};