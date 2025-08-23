// ui/element.cpp -- The Element class is a generic type of UI element that has standard features (such as redrawing), handled by the game manager.

// SPDX-FileType: SOURCE
// SPDX-FileCopyrightText: Copyright 2025 Raine Simmons <gc@gravecat.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/game.hpp"
#include "core/terminal/terminal.hpp"
#include "ui/element.hpp"

namespace gorp {

// Constructor, sets a null window pointer and assigns itself a unique UI ID.
Element::Element() : needs_redraw_(true), window_(nullptr), always_redraw_(false) { id_ = game().unique_ui_id(); }

// Constructor which also calls always_redraw().
Element::Element(bool set_always_redraw) : Element() { always_redraw_ = set_always_redraw; }

// Sets whether or not this element always redraws every frame.
void Element::always_redraw(bool toggle) { always_redraw_ = toggle; }

// Checks if this UI element needs to be redrawn.
bool Element::check_if_needs_redraw() const { return (always_redraw_ || needs_redraw_); }

// Orders this UI element to safely destroy its Window.
void Element::destroy_window()
{
    terminal().remove_window(window_);
    window_ = nullptr;
}

// Retrieves the unique ID of this UI element.
uint32_t Element::id() const { return id_; }

// Tells this UI element it needs to be redrawn.
void Element::needs_redraw(bool toggle) { needs_redraw_ = toggle; }

// Where this UI element reacts to input from the player, if it's at the top of the stack.
bool Element::process_input(int) { return false; }

}   // namespace gorp
