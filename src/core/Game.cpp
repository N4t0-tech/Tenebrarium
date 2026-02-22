#include "Game.hpp"
#include "ui/Renderer.hpp"
#include <ncurses.h>

// Color pair indices (defined here, used by Renderer)
// 1 = normal, 2 = selected/highlight, 3 = title, 4 = stats, 5 = selected box
static void initColors() {
    if (!has_colors()) return;
    start_color();
    use_default_colors();
    init_pair(1, COLOR_WHITE,  -1);           // normal
    init_pair(2, COLOR_YELLOW, -1);           // highlighted option
    init_pair(3, COLOR_CYAN,   -1);           // title
    init_pair(4, COLOR_GREEN,  -1);           // stats
    init_pair(5, COLOR_BLACK,  COLOR_CYAN);   // selected class box
}

Game::Game()
    : state_(GameState::MainMenu),
      menuPhase_(MenuPhase::Title),
      running_(true),
      menuSelection_(0),
      classSelection_(0)
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    initColors();
}

Game::~Game() {
    endwin();
}

void Game::run() {
    while (running_) {
        render();
        int key = getch();
        processInput(key);
        update();
    }
}

void Game::processInput(int key) {
    switch (state_) {
        case GameState::MainMenu:
            switch (menuPhase_) {
                case MenuPhase::Title:       inputTitle(key);       break;
                case MenuPhase::NameInput:   inputNameInput(key);   break;
                case MenuPhase::ClassSelect: inputClassSelect(key); break;
            }
            break;
        case GameState::Exploration:
            if (key == 'q' || key == 'Q') running_ = false;
            break;
        case GameState::Combat:
            if (key == 'q' || key == 'Q') running_ = false;
            break;
        case GameState::Inventory:
            if (key == 'q' || key == 'Q') running_ = false;
            break;
        case GameState::QuestLog:
            if (key == 'q' || key == 'Q') running_ = false;
            break;
        case GameState::GameOver:
            if (key == '\n' || key == KEY_ENTER)
                setState(GameState::MainMenu);
            break;
    }
}

void Game::inputTitle(int key) {
    switch (key) {
        case KEY_UP:
        case KEY_LEFT:
            menuSelection_ = (menuSelection_ + 1) % 2;
            break;
        case KEY_DOWN:
        case KEY_RIGHT:
            menuSelection_ = (menuSelection_ + 1) % 2;
            break;
        case '\n':
        case KEY_ENTER:
            if (menuSelection_ == 1) {
                running_ = false;
            } else {
                playerName_.clear();
                menuPhase_ = MenuPhase::NameInput;
                curs_set(1);
            }
            break;
        case 'q':
        case 'Q':
            running_ = false;
            break;
    }
}

void Game::inputNameInput(int key) {
    if (key == '\n' || key == KEY_ENTER) {
        if (!playerName_.empty()) {
            classSelection_ = 0;
            menuPhase_ = MenuPhase::ClassSelect;
            curs_set(0);
        }
        return;
    }

    if (key == KEY_BACKSPACE || key == 127 || key == 8) {
        if (!playerName_.empty())
            playerName_.pop_back();
        return;
    }

    if (key == 27) { // ESC → back to title
        menuPhase_ = MenuPhase::Title;
        curs_set(0);
        return;
    }

    if (key >= 32 && key <= 126 && static_cast<int>(playerName_.size()) < 16) {
        playerName_ += static_cast<char>(key);
    }
}

void Game::inputClassSelect(int key) {
    switch (key) {
        case KEY_LEFT:
            classSelection_ = (classSelection_ + 2) % 3;
            break;
        case KEY_RIGHT:
            classSelection_ = (classSelection_ + 1) % 3;
            break;
        case 27: // ESC → back to name input
            menuPhase_ = MenuPhase::NameInput;
            curs_set(1);
            break;
        case '\n':
        case KEY_ENTER: {
            PlayerClass cls;
            switch (classSelection_) {
                case 0: cls = PlayerClass::Warrior; break;
                case 1: cls = PlayerClass::Mage;    break;
                default: cls = PlayerClass::Ranger; break;
            }
            player_ = std::make_unique<Player>(playerName_, cls);
            setState(GameState::Exploration);
            break;
        }
    }
}

void Game::update() {
    // per-frame game logic (placeholder)
}

void Game::render() {
    clear();

    switch (state_) {
        case GameState::MainMenu:
            switch (menuPhase_) {
                case MenuPhase::Title:
                    Renderer::drawTitle(menuSelection_);
                    break;
                case MenuPhase::NameInput:
                    Renderer::drawNameInput(playerName_);
                    break;
                case MenuPhase::ClassSelect:
                    Renderer::drawClassSelect(classSelection_);
                    break;
            }
            break;
        case GameState::Exploration:
            Renderer::drawExploration();
            break;
        case GameState::Combat:
            Renderer::drawCombat();
            break;
        case GameState::Inventory:
            Renderer::drawInventory();
            break;
        case GameState::QuestLog:
            Renderer::drawQuestLog();
            break;
        case GameState::GameOver:
            Renderer::drawGameOver();
            break;
    }

    refresh();
}

void Game::setState(GameState newState) {
    if (newState == GameState::MainMenu) {
        menuPhase_      = MenuPhase::Title;
        menuSelection_  = 0;
        classSelection_ = 0;
        playerName_.clear();
        curs_set(0);
    }
    state_ = newState;
}
