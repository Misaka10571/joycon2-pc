#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>

// GL/GR Button Mapping Configuration
enum class ButtonMapping {
    NONE,
    L3, R3,
    L1, R1,
    L2, R2,
    CROSS, CIRCLE, SQUARE, TRIANGLE,
    SHARE, OPTIONS,
    DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT
};

struct GLGRLayout {
    std::string name;
    ButtonMapping glMapping = ButtonMapping::NONE;
    ButtonMapping grMapping = ButtonMapping::NONE;
};

struct ProControllerConfig {
    std::vector<GLGRLayout> layouts;
    int activeLayoutIndex = 0;
};

struct MouseConfig {
    bool chatKeyEnabled = true;
    float fastSensitivity = 1.0f;
    float normalSensitivity = 0.6f;
    float slowSensitivity = 0.3f;
    float scrollSpeed = 40.0f;
};

struct AppConfig {
    ProControllerConfig proConfig;
    MouseConfig mouseConfig;
};

// Button mapping string conversion helpers
inline std::string ButtonMappingToString(ButtonMapping mapping) {
    switch (mapping) {
    case ButtonMapping::NONE:       return "NONE";
    case ButtonMapping::L3:         return "L3";
    case ButtonMapping::R3:         return "R3";
    case ButtonMapping::L1:         return "L1";
    case ButtonMapping::R1:         return "R1";
    case ButtonMapping::L2:         return "L2";
    case ButtonMapping::R2:         return "R2";
    case ButtonMapping::CROSS:      return "CROSS";
    case ButtonMapping::CIRCLE:     return "CIRCLE";
    case ButtonMapping::SQUARE:     return "SQUARE";
    case ButtonMapping::TRIANGLE:   return "TRIANGLE";
    case ButtonMapping::SHARE:      return "SHARE";
    case ButtonMapping::OPTIONS:    return "OPTIONS";
    case ButtonMapping::DPAD_UP:    return "DPAD_UP";
    case ButtonMapping::DPAD_DOWN:  return "DPAD_DOWN";
    case ButtonMapping::DPAD_LEFT:  return "DPAD_LEFT";
    case ButtonMapping::DPAD_RIGHT: return "DPAD_RIGHT";
    default: return "NONE";
    }
}

inline ButtonMapping StringToButtonMapping(const std::string& str) {
    static const std::map<std::string, ButtonMapping> m = {
        {"NONE", ButtonMapping::NONE}, {"L3", ButtonMapping::L3}, {"R3", ButtonMapping::R3},
        {"L1", ButtonMapping::L1}, {"R1", ButtonMapping::R1}, {"L2", ButtonMapping::L2}, {"R2", ButtonMapping::R2},
        {"CROSS", ButtonMapping::CROSS}, {"CIRCLE", ButtonMapping::CIRCLE},
        {"SQUARE", ButtonMapping::SQUARE}, {"TRIANGLE", ButtonMapping::TRIANGLE},
        {"SHARE", ButtonMapping::SHARE}, {"OPTIONS", ButtonMapping::OPTIONS},
        {"DPAD_UP", ButtonMapping::DPAD_UP}, {"DPAD_DOWN", ButtonMapping::DPAD_DOWN},
        {"DPAD_LEFT", ButtonMapping::DPAD_LEFT}, {"DPAD_RIGHT", ButtonMapping::DPAD_RIGHT}
    };
    auto it = m.find(str);
    return (it != m.end()) ? it->second : ButtonMapping::NONE;
}

inline const char* ButtonMappingNames[] = {
    "NONE", "L3", "R3", "L1", "R1", "L2", "R2",
    "CROSS", "CIRCLE", "SQUARE", "TRIANGLE",
    "SHARE", "OPTIONS",
    "DPAD_UP", "DPAD_DOWN", "DPAD_LEFT", "DPAD_RIGHT"
};
inline constexpr int ButtonMappingCount = 17;

// Simple JSON serialization
inline std::string ConfigToJSON(const AppConfig& config) {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"activeLayoutIndex\": " << config.proConfig.activeLayoutIndex << ",\n";
    oss << "  \"layouts\": [\n";
    for (size_t i = 0; i < config.proConfig.layouts.size(); ++i) {
        const auto& l = config.proConfig.layouts[i];
        oss << "    { \"name\": \"" << l.name
            << "\", \"gl\": \"" << ButtonMappingToString(l.glMapping)
            << "\", \"gr\": \"" << ButtonMappingToString(l.grMapping) << "\" }";
        if (i + 1 < config.proConfig.layouts.size()) oss << ",";
        oss << "\n";
    }
    oss << "  ],\n";
    oss << "  \"mouse\": {\n";
    oss << "    \"chatKeyEnabled\": " << (config.mouseConfig.chatKeyEnabled ? "true" : "false") << ",\n";
    oss << "    \"fastSensitivity\": " << config.mouseConfig.fastSensitivity << ",\n";
    oss << "    \"normalSensitivity\": " << config.mouseConfig.normalSensitivity << ",\n";
    oss << "    \"slowSensitivity\": " << config.mouseConfig.slowSensitivity << ",\n";
    oss << "    \"scrollSpeed\": " << config.mouseConfig.scrollSpeed << "\n";
    oss << "  }\n";
    oss << "}";
    return oss.str();
}

// Helper to extract a JSON string value
inline std::string ExtractJsonString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    auto pos = json.find(searchKey);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    auto start = json.find('"', pos + 1);
    if (start == std::string::npos) return "";
    auto end = json.find('"', start + 1);
    if (end == std::string::npos) return "";
    return json.substr(start + 1, end - start - 1);
}

// Helper to extract a JSON number value
inline double ExtractJsonNumber(const std::string& json, const std::string& key, double defaultVal = 0.0) {
    std::string searchKey = "\"" + key + "\"";
    auto pos = json.find(searchKey);
    if (pos == std::string::npos) return defaultVal;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return defaultVal;
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    try { return std::stod(json.substr(pos)); } catch (...) { return defaultVal; }
}

// Helper to extract a JSON bool value
inline bool ExtractJsonBool(const std::string& json, const std::string& key, bool defaultVal = false) {
    std::string searchKey = "\"" + key + "\"";
    auto pos = json.find(searchKey);
    if (pos == std::string::npos) return defaultVal;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return defaultVal;
    auto rest = json.substr(pos + 1);
    if (rest.find("true") < rest.find("false")) return true;
    return false;
}

inline bool JSONToConfig(const std::string& json, AppConfig& config) {
    // Parse activeLayoutIndex
    config.proConfig.activeLayoutIndex = static_cast<int>(ExtractJsonNumber(json, "activeLayoutIndex", 0));

    // Parse layouts array
    config.proConfig.layouts.clear();
    auto layoutsPos = json.find("\"layouts\"");
    if (layoutsPos != std::string::npos) {
        auto arrStart = json.find('[', layoutsPos);
        auto arrEnd = json.find(']', arrStart);
        if (arrStart != std::string::npos && arrEnd != std::string::npos) {
            std::string arrStr = json.substr(arrStart, arrEnd - arrStart + 1);
            size_t objPos = 0;
            while ((objPos = arrStr.find('{', objPos)) != std::string::npos) {
                auto objEnd = arrStr.find('}', objPos);
                if (objEnd == std::string::npos) break;
                std::string objStr = arrStr.substr(objPos, objEnd - objPos + 1);
                
                GLGRLayout layout;
                layout.name = ExtractJsonString(objStr, "name");
                layout.glMapping = StringToButtonMapping(ExtractJsonString(objStr, "gl"));
                layout.grMapping = StringToButtonMapping(ExtractJsonString(objStr, "gr"));
                config.proConfig.layouts.push_back(layout);
                objPos = objEnd + 1;
            }
        }
    }

    // Parse mouse config
    auto mousePos = json.find("\"mouse\"");
    if (mousePos != std::string::npos) {
        auto mouseStart = json.find('{', mousePos);
        auto mouseEnd = json.find('}', mouseStart);
        if (mouseStart != std::string::npos && mouseEnd != std::string::npos) {
            std::string mouseStr = json.substr(mouseStart, mouseEnd - mouseStart + 1);
            config.mouseConfig.chatKeyEnabled = ExtractJsonBool(mouseStr, "chatKeyEnabled", true);
            config.mouseConfig.fastSensitivity = (float)ExtractJsonNumber(mouseStr, "fastSensitivity", 1.0);
            config.mouseConfig.normalSensitivity = (float)ExtractJsonNumber(mouseStr, "normalSensitivity", 0.6);
            config.mouseConfig.slowSensitivity = (float)ExtractJsonNumber(mouseStr, "slowSensitivity", 0.3);
            config.mouseConfig.scrollSpeed = (float)ExtractJsonNumber(mouseStr, "scrollSpeed", 40.0);
        }
    }

    return true;
}

class ConfigManager {
public:
    static ConfigManager& Instance() {
        static ConfigManager inst;
        return inst;
    }

    AppConfig config;
    const std::string configFile = "joycon2_config.json";

    bool Load() {
        std::ifstream file(configFile);
        if (!file.is_open()) return false;
        std::stringstream ss;
        ss << file.rdbuf();
        return JSONToConfig(ss.str(), config);
    }

    void Save() {
        std::ofstream file(configFile);
        if (file.is_open()) {
            file << ConfigToJSON(config);
        }
    }

    void EnsureDefaults() {
        if (config.proConfig.layouts.empty()) {
            GLGRLayout defaultLayout;
            defaultLayout.name = "Layout 1";
            defaultLayout.glMapping = ButtonMapping::NONE;
            defaultLayout.grMapping = ButtonMapping::NONE;
            config.proConfig.layouts.push_back(defaultLayout);
            config.proConfig.activeLayoutIndex = 0;
        }
    }

private:
    ConfigManager() = default;
};
