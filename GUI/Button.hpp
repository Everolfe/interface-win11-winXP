#pragma once
//#include <SFML/Graphics.hpp>
//#include <functional>
//#include <iostream>

#include "../Source.hpp"
class Button
{
private:
    sf::RectangleShape shape;
    
    sf::Text text;
    unsigned int textSize;

    sf::Color normalColor;
    sf::Color hoverColor;
    sf::Color clickColor;

    bool isHovered;
    bool isClicked;
    bool isEnabled = true;
    float animationProgress;


    sf::Vector2f relativePosition; 
    sf::Vector2f relativeSize;

public:
    Button(const std::string& label, const sf::Font& font)
        : isHovered(false), isClicked(false), animationProgress(0.0f), textSize(20),
        relativePosition(0.5f, 0.5f), relativeSize(0.2f, 0.1f) 
    {
        text.setFont(font);
        text.setString(label);
        text.setCharacterSize(textSize);
        text.setFillColor(sf::Color::White);

        normalColor = sf::Color(70, 130, 180);
        hoverColor = sf::Color(100, 149, 237);
        clickColor = sf::Color(65, 105, 225);

        shape.setFillColor(normalColor);
        shape.setOutlineThickness(2);
        shape.setOutlineColor(sf::Color::White);

        setSize(250, 50);
    }


    void setPosition(float x, float y);
    void setSize(float width, float height);
    void setTextSize(unsigned int size);
    sf::Vector2f getPosition() const;

    bool handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void update(float deltaTime);
    void draw(sf::RenderWindow& window) const;

    void setOnClick(std::function<void()> callback);
    bool isMouseOver(const sf::Vector2f& mousePos) const;
    void updateSize(const sf::Vector2f& windowSize);
    void setRelativePosition(float relX, float relY);
    void setRelativeSize(float relWidth, float relHeight);
    void resetState();
    sf::Vector2f getSize() const { return shape.getSize(); }
    void setText(const std::string& newText);

    void setEnabled(bool enabled);

    bool isEnabledF() const;


private:
    std::function<void()> onClick;
    void updateTextPosition();
};