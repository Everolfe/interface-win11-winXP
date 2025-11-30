#include "Button.hpp"

void Button::setPosition(float x, float y)
{
    shape.setPosition(x - shape.getSize().x / 2, y - shape.getSize().y / 2);
    updateTextPosition();
}

void Button::setRelativePosition(float relX, float relY)
{
    relativePosition = sf::Vector2f(relX, relY);
}

void Button::setRelativeSize(float relWidth, float relHeight)
{
    relativeSize = sf::Vector2f(relWidth, relHeight);
}

void Button::setSize(float width, float height)
{
    shape.setSize(sf::Vector2f(width, height));
    updateTextPosition();
}

void Button::setTextSize(unsigned int size)
{
    textSize = size;
    text.setCharacterSize(size);
    updateTextPosition();
}

void Button::updateTextPosition()
{
    sf::FloatRect textBounds = text.getLocalBounds();
    sf::FloatRect shapeBounds = shape.getGlobalBounds();

    text.setPosition(
        shapeBounds.left + (shapeBounds.width - textBounds.width) / 2 - textBounds.left,
        shapeBounds.top + (shapeBounds.height - textBounds.height) / 2 - textBounds.top - 5
    );
}

sf::Vector2f Button::getPosition() const
{
    return shape.getPosition() + shape.getSize() / 2.0f;
}

bool Button::handleEvent(const sf::Event& event, const sf::RenderWindow& window)
{
    if (event.type == sf::Event::MouseMoved)
    {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        isHovered = shape.getGlobalBounds().contains(mousePos);
    }
    else if (event.type == sf::Event::MouseButtonPressed)
    {
        if (event.mouseButton.button == sf::Mouse::Left)
        {
            sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            if (shape.getGlobalBounds().contains(mousePos))
            {
                isClicked = true;
                animationProgress = 0.0f;
                return true;
            }
        }
    }
    else if (event.type == sf::Event::MouseButtonReleased)
    {
        if (event.mouseButton.button == sf::Mouse::Left && isClicked)
        {
            isClicked = false;
            if (onClick) onClick();
        }
    }
    return false;
}

void Button::update(float deltaTime)
{
    if (isClicked)
    {
        animationProgress += deltaTime * 8.0f;
        if (animationProgress > 1.0f) animationProgress = 1.0f;

        float scale = 1.0f - 0.1f * animationProgress;
        shape.setScale(scale, scale);
        text.setScale(scale, scale);
        shape.setFillColor(clickColor);
    }
    else if (isHovered)
    {
        animationProgress += deltaTime * 4.0f;
        if (animationProgress > 1.0f) animationProgress = 1.0f;

        shape.setFillColor(sf::Color(
            normalColor.r + (hoverColor.r - normalColor.r) * animationProgress,
            normalColor.g + (hoverColor.g - normalColor.g) * animationProgress,
            normalColor.b + (hoverColor.b - normalColor.b) * animationProgress
        ));
        shape.setScale(1.05f, 1.05f);
        text.setScale(1.05f, 1.05f);
    }
    else
    {
        animationProgress -= deltaTime * 4.0f;
        if (animationProgress < 0.0f) animationProgress = 0.0f;

        shape.setFillColor(sf::Color(
            normalColor.r + (hoverColor.r - normalColor.r) * animationProgress,
            normalColor.g + (hoverColor.g - normalColor.g) * animationProgress,
            normalColor.b + (hoverColor.b - normalColor.b) * animationProgress
        ));
        shape.setScale(1.0f, 1.0f);
        text.setScale(1.0f, 1.0f);
    }
}

void Button::draw(sf::RenderWindow& window) const
{
    window.draw(shape);
    window.draw(text);
}

void Button::setOnClick(std::function<void()> callback)
{
    onClick = callback;
}

bool Button::isMouseOver(const sf::Vector2f& mousePos) const
{
    return shape.getGlobalBounds().contains(mousePos);
}
void Button::updateSize(const sf::Vector2f& windowSize)
{
    float absX = relativePosition.x * windowSize.x;
    float absY = relativePosition.y * windowSize.y;
    float absWidth = relativeSize.x * windowSize.x;
    float absHeight = relativeSize.y * windowSize.y;

    setPosition(absX, absY);
    setSize(absWidth, absHeight);
    setTextSize(static_cast<unsigned int>(windowSize.y * 0.035f));
}

void Button::resetState()
{
    isClicked = false;
    isHovered = false;
}

void Button::setText(const std::string& newText) {
    text.setString(newText);
    updateTextPosition();
}

void Button::setEnabled(bool enabled) {
    isEnabled = enabled;
    if (!enabled) {
        normalColor = sf::Color(100, 100, 100);
        hoverColor = sf::Color(100, 100, 100);
        clickColor = sf::Color(80, 80, 80);
    }
    else {
        normalColor = sf::Color(70, 130, 180);
        hoverColor = sf::Color(100, 149, 237);
        clickColor = sf::Color(65, 105, 225);
    }
    shape.setFillColor(normalColor);
}

bool Button::isEnabledF() const {
    return isEnabled;
}