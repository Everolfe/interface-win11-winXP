#pragma once
#include "AdditionalFunc.hpp"

class Character {
private:

    bool isMovingToMouse = false;
    sf::Vector2f mouseTarget;
    
    int frameWidth;
    int frameHeight;
    int currentFrame = 0;
    int totalFrames = 28;
    bool useCharging = false;
    float frameTime = 0.1f;   
    float frameTimer = 0.0f;

    sf::Sprite sprite;
    sf::Texture normalTexture;
    sf::Texture pointingTexture;
    sf::Texture walkingTexture;
    float scale;
    float targetScale;
    sf::Vector2f position;
    sf::Vector2f targetPosition;
    float animationSpeed;
    bool isPointing;
    float baseAnimationSpeed;

    sf::Vector2f randomTarget;
    float randomMoveTimer;
    float randomMoveInterval;
    bool isMovingToButton;
    bool facingRight;
    bool followMouse = true;


    sf::Vector2f windowSize;

    // Вспомогательные методы
    void createDefaultTexture(sf::Texture& texture);
    void generateNewRandomTarget();

public:
    Character(const sf::Vector2f& winSize);

    bool loadTextures(const std::string& normalFilename, const std::string& pointingFilename, const std::string& walkingFilename);

    void setPointing(bool pointing);
    void setButtonTarget(const sf::Vector2f& buttonPos);
    void setButtonTargetLeft(const sf::Vector2f& buttonPos, const sf::Vector2f& buttonSize);
    void setRandomMovement();

    bool isPointingNow() const;
    void setPosition(float x, float y);
    void setTargetPosition(float x, float y);
    void setScale(float newScale);
    void setInitialScale(float newScale);
    void setWindowSize(const sf::Vector2f& newSize);

    void update(float deltaTime);
    void draw(sf::RenderWindow& window);

    void setAnimationSpeed(float speed);
    void setBaseAnimationSpeed(float speed); 
    float getAnimationSpeed() const;
    sf::Vector2f getPosition() const;

    void moveToMouse(const sf::Vector2f& mousePos);
};
