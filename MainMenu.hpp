#pragma once
#include"Character.hpp"
#include "Button.hpp"
#include"Lab1.hpp"
#include"Lab2.hpp"
#include"Lab3.hpp"
#include"Lab4.hpp"
#include"Lab5.hpp"
#include"Lab6.hpp"
#include"State.hpp"

class MainMenu {
private:
    sf::RenderWindow& window;
    sf::Font font;
    sf::Text title;
    Character character;
    std::vector<Button> buttons;
    std::vector<std::unique_ptr<LabWindow>> labWindows;

    sf::Texture backgroundTexture;
    sf::Sprite background;

    int currentlyHoveredButton;
    sf::Vector2f lastButtonTarget;

    bool isTransitioning;
    float transitionProgress;

    sf::Vector2f windowSize;

    State nextState;

    void updateButtonsSizeAndPosition();
    void createButtons();
    void createLabWindows();
    void startTransitionToLab(int labIndex);

public:
    MainMenu(sf::RenderWindow& win);

    LabWindow* getLabByIndex(int index);
    void updateWindowSize();
    void handleEvent(const sf::Event& event);
    void update(float deltaTime);
    void draw();
    void resetButtonState();

    bool isInTransition() const;
    State getNextState() const;
    void resetState();
};