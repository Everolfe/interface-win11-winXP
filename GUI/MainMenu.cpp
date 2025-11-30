#include"MainMenu.hpp"


MainMenu::MainMenu(sf::RenderWindow& win)
    : window(win)
    , windowSize(win.getSize().x, win.getSize().y)
    , character(windowSize)
    , isTransitioning(false)
    , transitionProgress(0.0f)
    , currentlyHoveredButton(-1)
    , nextState(State::MainMenu)
{
    // Выводим текущую директорию для отладки
    std::cout << "Current directory: " << std::filesystem::current_path() << std::endl;
    std::cout << "Executable path: " << getExecutablePath() << std::endl;

    // Загрузка шрифта
    if (!loadFontFromResources(font, "font.ttf"))
    {
        std::cerr << "Using default font." << std::endl;
        font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");
    }

    // Настройка заголовка
    title.setFont(font);
    title.setString("Laboratory Works Interface");
    title.setCharacterSize(static_cast<unsigned int>(windowSize.y * 0.06f));
    title.setFillColor(sf::Color::White);
    title.setStyle(sf::Text::Bold);
    title.setPosition(windowSize.x * 0.5f - title.getGlobalBounds().width / 2, windowSize.y * 0.05f);

    // Загрузка фона
    if (!loadTextureFromResources(backgroundTexture, "background.jpg"))
    {
        backgroundTexture.create(static_cast<unsigned int>(windowSize.x), static_cast<unsigned int>(windowSize.y));
        sf::Uint8* pixels = new sf::Uint8[800 * 600 * 4];
        for (int i = 0; i < 800 * 600 * 4; i += 4)
        {
            pixels[i] = 30;
            pixels[i + 1] = 30;
            pixels[i + 2] = 70;
            pixels[i + 3] = 255;
        }
        backgroundTexture.update(pixels);
        delete[] pixels;
    }
    background.setTexture(backgroundTexture);
    background.setScale(
        windowSize.x / backgroundTexture.getSize().x,
        windowSize.y / backgroundTexture.getSize().y
    );

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // Загрузка персонажа
    character.loadTextures("character_normal.png", "character_pointing.png", "walk.png");

    character.setPosition(windowSize.x * 0.5f, windowSize.y * 0.4f);
    character.setInitialScale(windowSize.y * 0.0010f);
    character.setRandomMovement();

    // Создание кнопок и окон лабораторных работ
    createButtons();
    createLabWindows();
}

LabWindow* MainMenu::getLabByIndex(int index)
{
    if (index >= 0 && index < labWindows.size())
    {
        return labWindows[index].get();
    }
    return nullptr;
}

void MainMenu::updateWindowSize()
{
    windowSize = sf::Vector2f(window.getSize().x, window.getSize().y);

    background.setScale(
        windowSize.x / backgroundTexture.getSize().x,
        windowSize.y / backgroundTexture.getSize().y
    );

    character.setWindowSize(windowSize);

    updateButtonsSizeAndPosition();

    title.setCharacterSize(static_cast<unsigned int>(windowSize.y * 0.06f));
    title.setPosition(windowSize.x * 0.5f - title.getGlobalBounds().width / 2, windowSize.y * 0.05f);
}

void MainMenu::updateButtonsSizeAndPosition()
{
    for (int i = 0; i < buttons.size(); ++i)
    {
        float buttonX = windowSize.x * 0.75f;
        float buttonY = windowSize.y * 0.25f + i * windowSize.y * 0.1f;
        float buttonWidth = windowSize.x * 0.3f;
        float buttonHeight = windowSize.y * 0.08f;

        buttons[i].setPosition(buttonX, buttonY);
        buttons[i].setSize(buttonWidth, buttonHeight);
        buttons[i].setTextSize(static_cast<unsigned int>(windowSize.y * 0.035f));
    }
}

void MainMenu::createButtons()
{
    std::vector<std::string> buttonLabels = {
        "Lab 1: Battery",
        "Lab 2: PCI Bus",
        "Lab 3: Discks",
        "Lab 4: Web Camera",
        "Lab 5: USB",
        "Lab 6: Bluetooth",
        "Exit"
    };

    for (int i = 0; i < buttonLabels.size(); ++i)
    {
        Button btn(buttonLabels[i], font);
        float buttonX = windowSize.x * 0.75f;
        float buttonY = windowSize.y * 0.25f + i * windowSize.y * 0.1f;
        float buttonWidth = windowSize.x * 0.3f;
        float buttonHeight = windowSize.y * 0.08f;

        btn.setPosition(buttonX, buttonY);
        btn.setSize(buttonWidth, buttonHeight);
        btn.setTextSize(static_cast<unsigned int>(windowSize.y * 0.035f));
        buttons.push_back(btn);
    }
}

void MainMenu::createLabWindows()
{
      labWindows.push_back(std::make_unique<Lab1>(window, font));
      labWindows.push_back(std::make_unique<Lab2>(window, font)); // Добавляем Lab2
      labWindows.push_back(std::make_unique<Lab3>(window, font));
      labWindows.push_back(std::make_unique<Lab4>(window, font));
      labWindows.push_back(std::make_unique<Lab5>(window, font));
      labWindows.push_back(std::make_unique<Lab6>(window, font));
}

void MainMenu::handleEvent(const sf::Event& event)
{
    if (isTransitioning) return;

    if (event.type == sf::Event::MouseMoved)
    {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        int previouslyHovered = currentlyHoveredButton;
        currentlyHoveredButton = -1;

        // Проверяем наведение на кнопки
        for (size_t i = 0; i < buttons.size(); ++i)
        {
            if (buttons[i].isMouseOver(mousePos))
            {
                currentlyHoveredButton = i;
                break;
            }
        }

        if (currentlyHoveredButton != -1)
        {
            sf::Vector2f buttonPos = buttons[currentlyHoveredButton].getPosition();
            sf::Vector2f buttonSize = buttons[currentlyHoveredButton].getSize();
            character.setButtonTargetLeft(buttonPos, buttonSize);
            character.setPointing(true);
        }
        else
        {
            character.setPointing(false);
            character.moveToMouse(mousePos);
        }
    }

    for (size_t i = 0; i < buttons.size(); ++i)
    {
        if (buttons[i].handleEvent(event, window))
        {
            if (i < labWindows.size())
            {
                startTransitionToLab(i);
            }
            else if (i == buttons.size() - 1)
            {
                window.close();
            }
            for (auto& btn : buttons)
                btn.resetState();
        }
    }
}


void MainMenu::startTransitionToLab(int labIndex)
{
    isTransitioning = true;
    transitionProgress = 0.0f;

    sf::Vector2f buttonPos = buttons[labIndex].getPosition();
    sf::Vector2f buttonSize = buttons[labIndex].getSize();
    character.setButtonTargetLeft(buttonPos, buttonSize);

    switch (labIndex) {
    case 0: nextState = State::Lab1; break;
    case 1: nextState = State::Lab2; break;
    case 2: nextState = State::Lab3; break;
    case 3: nextState = State::Lab4; break;
    case 4: nextState = State::Lab5; break;
    case 5: nextState = State::Lab6; break;
    case 6: 
        window.close();
        nextState = State::MainMenu;
        break;
    }
}

void MainMenu::update(float deltaTime)
{
    character.update(deltaTime);

    if (isTransitioning)
    {
        transitionProgress += deltaTime * 0.5f;

        if (transitionProgress >= 1.0f)
        {
            isTransitioning = false;
            std::cout << "Starting lab..." << std::endl;
            character.setTargetPosition(400, 300);
            character.setRandomMovement();
        }
    }

    for (auto& button : buttons)
    {
        button.update(deltaTime);
    }
}

void MainMenu::draw()
{
    windowSize = sf::Vector2f(window.getSize().x, window.getSize().y);

    static sf::Vector2f lastWindowSize = windowSize;
    if (windowSize != lastWindowSize)
    {
        updateWindowSize();
        lastWindowSize = windowSize;
    }

    window.draw(background);

    if (isTransitioning)
    {
        sf::RectangleShape overlay(windowSize);
        overlay.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(150 * transitionProgress)));
        window.draw(overlay);
    }

    title.setPosition(windowSize.x * 0.5f - title.getGlobalBounds().width / 2, windowSize.y * 0.05f);
    window.draw(title);

    for (auto& button : buttons)
    {
        button.draw(window);
    }

    character.draw(window);

    if (isTransitioning)
    {
        float circleRadius = std::min(windowSize.x, windowSize.y) * 0.5f * transitionProgress;
        sf::CircleShape transitionCircle(circleRadius);
        transitionCircle.setOrigin(circleRadius, circleRadius);
        transitionCircle.setPosition(windowSize.x * 0.5f, windowSize.y * 0.5f);
        transitionCircle.setFillColor(sf::Color::Black);
        window.draw(transitionCircle);
    }
}

void MainMenu::resetButtonState()
{
    currentlyHoveredButton = -1;
    character.setRandomMovement();
    character.setPointing(false);
}

bool MainMenu::isInTransition() const
{
    return isTransitioning;
}

State MainMenu::getNextState() const
{
    return nextState;
}

void MainMenu::resetState()
{
    nextState = State::MainMenu;
}