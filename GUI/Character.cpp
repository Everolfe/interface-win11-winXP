#include"Character.hpp"
Character::Character(const sf::Vector2f& winSize) :
    scale(1.0f), targetScale(1.0f), animationSpeed(0.1f),
    isPointing(false), randomMoveTimer(0.0f), randomMoveInterval(3.0f),
    isMovingToButton(false), facingRight(true),
    windowSize(winSize)
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    randomTarget = sf::Vector2f(windowSize.x * 0.5f, windowSize.y * 0.5f);
}

bool Character::loadTextures(const std::string& normalFilename, const std::string& pointingFilename, const std::string& walkingFilename)
{

    if (!loadTextureFromResources(normalTexture, normalFilename))
    {
        std::cerr << "Failed to load normal character texture!" << std::endl;
        createDefaultTexture(normalTexture);
    }

    if (!loadTextureFromResources(pointingTexture, pointingFilename))
    {
        std::cerr << "Failed to load pointing character texture!" << std::endl;
        createDefaultTexture(pointingTexture);
        sf::Uint8* pixels = new sf::Uint8[100 * 100 * 4];
        for (int i = 0; i < 100 * 100 * 4; i += 4)
        {
            pixels[i] = 0;     
            pixels[i + 1] = 255; 
            pixels[i + 2] = 0;   
            pixels[i + 3] = 255; 
        }
        pointingTexture.update(pixels);
        delete[] pixels;
    }

    if (!loadTextureFromResources(walkingTexture, walkingFilename))
    {
        std::cerr << "Failed to load walking texture!" << std::endl;
        createDefaultTexture(walkingTexture); 

        sf::Uint8* pixels = new sf::Uint8[100 * 100 * 4];
        for (int i = 0; i < 100 * 100 * 4; i += 4)
        {
            pixels[i] = 0;     
            pixels[i + 1] = 0; 
            pixels[i + 2] = 255; 
            pixels[i + 3] = 255;
        }
        walkingTexture.update(pixels);
        delete[] pixels;
    }
    else
    {
        totalFrames = 28;
        frameWidth = walkingTexture.getSize().x / totalFrames;
        frameHeight = walkingTexture.getSize().y;
    }

    sprite.setTexture(normalTexture);
    sprite.setOrigin(normalTexture.getSize().x / 2.0f, normalTexture.getSize().y / 2.0f);
    return true;
}

void Character::createDefaultTexture(sf::Texture& texture)
{
    texture.create(100, 100);
    sf::Uint8* pixels = new sf::Uint8[100 * 100 * 4];
    for (int i = 0; i < 100 * 100 * 4; i += 4)
    {
        pixels[i] = 255;     
        pixels[i + 1] = 0;   
        pixels[i + 2] = 0;   
        pixels[i + 3] = 255; 
    }
    texture.update(pixels);
    delete[] pixels;
}

void Character::setPointing(bool pointing)
{
    if (pointing && !isPointing)
    {
        sprite.setTexture(pointingTexture);
        isPointing = true;
    }
    else if (!pointing && isPointing)
    {
        sprite.setTexture(normalTexture);
        isPointing = false;
    }
}

void Character::setButtonTarget(const sf::Vector2f& buttonPos)
{
    isMovingToButton = true;
    targetPosition = buttonPos;
    targetPosition.y -= windowSize.y * 0.08f;
}

void Character::setButtonTargetLeft(const sf::Vector2f& buttonPos, const sf::Vector2f& buttonSize)
{
    isMovingToButton = true;

    // Учитываем размер текстуры персонажа
    sf::FloatRect charBounds = sprite.getGlobalBounds();

    targetPosition = sf::Vector2f(
        buttonPos.x - charBounds.width * 0.7f,  
        buttonPos.y - windowSize.y * 0.08f      
    );
}

void Character::setRandomMovement()
{
    isMovingToButton = false;
    generateNewRandomTarget();
}

void Character::generateNewRandomTarget()
{

    float margin = std::min(windowSize.x, windowSize.y) * 0.1f; 
    randomTarget = sf::Vector2f(
        margin + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / (windowSize.x - 2 * margin))),
        margin + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / (windowSize.y - 2 * margin)))
    );

    targetPosition = randomTarget;
    randomMoveTimer = 0.0f;
    randomMoveInterval = 2.0f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / 3.0f));
}

bool Character::isPointingNow() const
{
    return isPointing;
}

void Character::setPosition(float x, float y)
{
    position = sf::Vector2f(x, y);
    sprite.setPosition(position);
}

void Character::setTargetPosition(float x, float y)
{
    targetPosition = sf::Vector2f(x, y);
}

void Character::setScale(float newScale)
{
    targetScale = newScale;
    sprite.setScale(facingRight ? scale : -scale, scale);
}

void Character::setInitialScale(float newScale)
{
    scale = newScale;
    targetScale = newScale;
    sprite.setScale(scale, scale);
}

void Character::setWindowSize(const sf::Vector2f& newSize)
{
    windowSize = newSize;
}

void Character::setAnimationSpeed(float speed) {
    animationSpeed = speed;
}

void Character::setBaseAnimationSpeed(float speed) {
    baseAnimationSpeed = speed;
    animationSpeed = speed;
}

float Character::getAnimationSpeed() const {
    return animationSpeed;
}

void Character::update(float deltaTime) {
    bool isMoving = false;

    if (isMovingToMouse) {
        sf::Vector2f direction = targetPosition - position;
        float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        if (distance < 5.0f) {
            isMovingToMouse = false;
            setRandomMovement();
        }
        else {
            position += direction * animationSpeed * deltaTime;
            isMoving = true;
        }
    }
    else if (!isMovingToButton) {
        randomMoveTimer += deltaTime;
        if (randomMoveTimer >= randomMoveInterval) {
            generateNewRandomTarget();
        }
        sf::Vector2f direction = targetPosition - position;
        if (std::sqrt(direction.x * direction.x + direction.y * direction.y) > 2.0f) {
            position += direction * animationSpeed * deltaTime;
            isMoving = true;
        }
    }
    else {
        sf::Vector2f direction = targetPosition - position;
        if (std::sqrt(direction.x * direction.x + direction.y * direction.y) > 2.0f) {
            position += direction * animationSpeed * deltaTime;
            isMoving = true;
        }
    }

    // ?? Масштаб и направление
    scale += (targetScale - scale) * animationSpeed * deltaTime;

    if (!isMovingToButton && !isMovingToMouse) {
        sf::Vector2f direction = targetPosition - position;
        if (std::abs(direction.x) > 1.0f) {
            bool shouldFaceRight = direction.x > 0;
            if (shouldFaceRight != facingRight) {
                facingRight = shouldFaceRight;
                sprite.setScale(facingRight ? scale : -scale, scale);
            }
        }
    }
    else if (isMovingToButton) {
        if (!facingRight) {
            facingRight = true;
            sprite.setScale(scale, scale);
        }
    }

    // ?? Анимация
    if (isMoving) {
        sprite.setTexture(walkingTexture);
        frameTimer += deltaTime;
        if (frameTimer >= frameTime) {
            frameTimer = 0.0f;
            currentFrame = (currentFrame + 1) % totalFrames;
        }

        sprite.setTextureRect(sf::IntRect(
            currentFrame * frameWidth,
            0,
            frameWidth,
            frameHeight
        ));
        sprite.setOrigin(frameWidth / 2.0f, frameHeight / 2.0f);
    }
    else {
        // Стоит ? обычная текстура
        sprite.setTexture(normalTexture);
        sprite.setTextureRect(sf::IntRect()); // весь спрайт
        sprite.setOrigin(normalTexture.getSize().x / 2.0f, normalTexture.getSize().y / 2.0f);
    }

    sprite.setPosition(position);
}



void Character::draw(sf::RenderWindow& window)
{
    window.draw(sprite);
}

sf::Vector2f Character::getPosition() const
{
    return position;
}

void Character::moveToMouse(const sf::Vector2f& mousePos)
{
    isMovingToMouse = true;
    isMovingToButton = false; 

    targetPosition = mousePos;


    if ((targetPosition.x - position.x) > 0)
    {
        facingRight = true;
        sprite.setScale(scale, scale);
    }
    else
    {
        facingRight = false;
        sprite.setScale(-scale, scale);
    }
}