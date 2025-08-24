// core/game.cpp -- The Game object is the central game manager, handling the main loop, saving/loading, and starting of new games.

// SPDX-FileType: SOURCE
// SPDX-FileCopyrightText: Copyright 2025 Raine Simmons <gc@gravecat.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>  // EXIT_SUCCESS

#include "3rdparty/sam/sam.hpp"
#include "core/core.hpp"
#include "core/game.hpp"
#include "core/terminal/terminal.hpp"
#include "ui/element.hpp"
#include "ui/title.hpp"
#include "util/math/random.hpp"
#include "world/codex.hpp"

namespace gorp {

Game::Game() : codex_ptr_(nullptr), title_screen_ptr_(nullptr), ui_element_id_counter_(0) { }

// Destructor, cleans up attached classes.
Game::~Game()
{
    for (unsigned int i = 0; i < ui_elements_.size(); i++)
        ui_elements_.at(i).reset(nullptr);
    title_screen_ptr_.reset(nullptr);
    codex_ptr_.reset(nullptr);
}

// Adds a new UI element to the screen.
uint32_t Game::add_element(std::unique_ptr<Element> element)
{
    const uint32_t id = element.get()->id();
    ui_elements_.push_back(std::move(element));
    return id;
}

// Starts the game, in the form of a title screen followed by the main game loop.
void Game::begin()
{
    codex_ptr_ = std::make_unique<Codex>();
    title_screen_ptr_ = std::make_unique<TitleScreen>();

    const auto result = title_screen_ptr_->render();
    switch(result)
    {
        case TitleScreen::TitleOption::QUIT:
        case TitleScreen::TitleOption::LOAD_GAME:   // Load game is not yet implemented.
            leave_game();
            break;
        case TitleScreen::TitleOption::NEW_GAME:
            new_game();
            break;
    }
    main_loop();
}

// Clears all UI elements.
void Game::clear_elements()
{
    for (unsigned int i = 0; i < ui_elements_.size(); i++)
        ui_elements_.at(i)->destroy_window();
    ui_elements_.clear();
}

// Returns a reference to the Codex object.
Codex& Game::codex() const
{
    if (!codex_ptr_) throw std::runtime_error("Attempt to access null Codex pointer!");
    return *codex_ptr_;
}

// Deletes a specified UI element.
void Game::delete_element(uint32_t id)
{
    for (unsigned int i = 0; i < ui_elements_.size(); i++)
    {
        if (ui_elements_.at(i).get()->id() == id)
        {
            ui_elements_.erase(ui_elements_.begin() + i);
            return;
        }
    }
    throw std::runtime_error("Attempt to delete invalid UI element!");
}

// Retrieves a specified UI element.
Element& Game::element(uint32_t id) const
{
    for (unsigned int i = 0; i < ui_elements_.size(); i++)
    {
        Element* element = ui_elements_.at(i).get();
        if (!element) throw std::runtime_error("Null element on UI stack!");
        if (element->id() == id) return *element;
    }
    throw std::runtime_error("Invalid UI element requested!");
}

// Shuts things down cleanly and exits the game.
void Game::leave_game() { core().destroy_core(EXIT_SUCCESS); }

// brøether, may i have the lööps
void Game::main_loop()
{
    int key = 0;
    while(true)
    {
        // Redraw all UI elements, as needed.
        for (unsigned int i = 0; i < ui_elements_.size(); i++)
        {
            Element* el = ui_elements_.at(i).get();
            if (el->check_if_needs_redraw())
            {
                el->render();
                el->needs_redraw(false);
            }
        }

        key = terminal().get_key();
        switch(key)
        {
            case 0: break;      // Do nothing when no key is pressed.

            case Key::RESIZE:   // Resize 'key' is always handled by the game manager.
                for (unsigned int i = 0; i < ui_elements_.size(); i++)
                {
                    Element *el = ui_elements_.at(i).get();
                    el->recreate_window();
                    el->needs_redraw(true);
                }
                break;

            default:    // For all other keys, cycle through UI from top to bottom, stopping if any UI element has processed the pressed key.
                for (int i = ui_elements_.size() - 1; i >= 0; i--)
                {
                    Element *el = ui_elements_.at(i).get();
                    if (el->process_input(key)) break;
                }
                break;
        }
    }
}

// Sets up for a new game!
void Game::new_game()
{
}

// Returns a new, unique UI element ID.
uint32_t Game::unique_ui_id() { return ++ui_element_id_counter_; }

// A shortcut instead of using core().game()
Game& game() { return core().game(); }

}   // namespace gorp
