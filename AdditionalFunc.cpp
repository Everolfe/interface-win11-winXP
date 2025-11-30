#include"AdditionalFunc.hpp"
std::string getExecutablePath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    return std::string(buffer).substr(0, pos);
}

bool loadTextureFromResources(sf::Texture& texture, const std::string& filename) {
    std::string resourcesPath = getExecutablePath() + "\\resources\\";
    std::string fullPath = resourcesPath + filename;

    if (texture.loadFromFile(fullPath)) {
        std::cout << "Successfully loaded: " << fullPath << std::endl;
        return true;
    }

    std::vector<std::string> alternativePaths = {
        filename,
        "resources/" + filename,
        "../resources/" + filename,
        "../../resources/" + filename
    };

    for (const auto& path : alternativePaths) {
        if (texture.loadFromFile(path)) {
            std::cout << "Successfully loaded from alternative path: " << path << std::endl;
            return true;
        }
    }

    std::cerr << "Failed to load texture: " << filename << std::endl;
    std::cerr << "Tried paths:" << std::endl;
    std::cerr << "  " << fullPath << std::endl;
    for (const auto& path : alternativePaths) {
        std::cerr << "  " << path << std::endl;
    }

    return false;
}

bool loadFontFromResources(sf::Font& font, const std::string& filename) {
    std::string resourcesPath = getExecutablePath() + "\\resources\\";
    std::string fullPath = resourcesPath + filename;

    if (font.loadFromFile(fullPath)) {
        std::cout << "Successfully loaded: " << fullPath << std::endl;
        return true;
    }

    std::vector<std::string> alternativePaths = {
        filename,
        "resources/" + filename,
        "../resources/" + filename,
        "../../resources/" + filename
    };

    for (const auto& path : alternativePaths) {
        if (font.loadFromFile(path)) {
            std::cout << "Successfully loaded from alternative path: " << path << std::endl;
            return true;
        }
    }

    std::cerr << "Failed to load font: " << filename << std::endl;
    return false;
}