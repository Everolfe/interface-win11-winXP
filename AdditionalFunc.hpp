#pragma once
//#define WIN32_LEAN_AND_MEAN
//#include <SFML/Graphics.hpp>
//#include <SFML/Audio.hpp>
//#include <vector>
//#include <memory>
//#include <iostream>
//#include <filesystem>
//#include <Windows.h>

#include "Source.hpp"
std::string getExecutablePath();

bool loadTextureFromResources(sf::Texture& texture, const std::string& filename);

bool loadFontFromResources(sf::Font& font, const std::string& filename);