#include "SettingStore.h"

// Generic setter
void SettingStore::SetValue(const std::string& key, std::any value)
{
    m_settings[key] = std::move(value);
}

// Convenience setters
void SettingStore::SetBool(const std::string& key, bool value)
{
    SetValue(key, value);
}

void SettingStore::SetInt(const std::string& key, int value)
{
    SetValue(key, value);
}

void SettingStore::SetFloat(const std::string& key, float value)
{
    SetValue(key, value);
}

void SettingStore::SetString(const std::string& key, const std::string& value)
{
    SetValue(key, value);
}

// Generic getter
std::any SettingStore::GetValue(const std::string& key, const std::any& defaultValue) const
{
    auto it = m_settings.find(key);
    if (it != m_settings.end())
    {
        return it->second;
    }
    return defaultValue;
}

// Convenience getters with default value
bool SettingStore::GetBool(const std::string& key, bool defaultValue) const
{
    auto it = m_settings.find(key);
    if (it != m_settings.end())
    {
        try
        {
            return std::any_cast<bool>(it->second);
        }
        catch (const std::bad_any_cast&)
        {
            // In case the type is wrong, return default
        }
    }
    return defaultValue;
}

int SettingStore::GetInt(const std::string& key, int defaultValue) const
{
    auto it = m_settings.find(key);
    if (it != m_settings.end())
    {
        try
        {
            return std::any_cast<int>(it->second);
        }
        catch (const std::bad_any_cast&)
        {
        }
    }
    return defaultValue;
}

float SettingStore::GetFloat(const std::string& key, float defaultValue) const
{
    auto it = m_settings.find(key);
    if (it != m_settings.end())
    {
        try
        {
            return std::any_cast<float>(it->second);
        }
        catch (const std::bad_any_cast&)
        {
        }
    }
    return defaultValue;
}

std::string SettingStore::GetString(const std::string& key, const std::string& defaultValue) const
{
    auto it = m_settings.find(key);
    if (it != m_settings.end())
    {
        try
        {
            return std::any_cast<std::string>(it->second);
        }
        catch (const std::bad_any_cast&)
        {
        }
    }
    return defaultValue;
}

// Check if a key exists
bool SettingStore::Has(const std::string& key) const
{
    return m_settings.find(key) != m_settings.end();
}