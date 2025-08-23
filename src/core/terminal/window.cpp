// core/terminal/window.cpp -- Used in conjunction with Terminal to provide virtual 'windows', rendering surfaces that can be painted on and moved around.

// SPDX-FileType: SOURCE
// SPDX-FileCopyrightText: Copyright 2025 Raine Simmons <gc@gravecat.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/prefs.hpp"
#include "core/terminal/colour-maps.hpp"
#include "core/terminal/terminal.hpp"
#include "core/terminal/window.hpp"

namespace gorp {

// Creates a new Window of the specified size and position.
Window::Window(Vector2 new_size, Vector2 new_pos) : pos_(new_pos), render_texture_(nullptr), size_(new_size)
{
    if (new_size.x < 1) new_size.x = 1;
    if (new_size.y < 1) new_size.y = 1;
    if (new_size.x < 0) new_size.x = 0;
    if (new_size.y < 0) new_size.y = 0;

    Prefs &pref = prefs();
    sf::Vector2u window_size(size_.x * pref.tile_scale() * Terminal::TILE_SIZE,
        size_.y * pref.tile_scale() * Terminal::TILE_SIZE);
    render_texture_ = std::make_unique<sf::RenderTexture>(window_size);
}

// Destructor, explicitly frees memory used.
Window::~Window() { render_texture_.reset(nullptr); }

// Draws a box around a Window.
void Window::box(Colour colour)
{
    for (int x = 0; x <= size_.x; x++)
    {
        for (int y = 0; y <= size_.y; y++)
        {
            Glyph glyph = static_cast<Glyph>(0);
            if (x == 0)
            {
                if (y == 0) glyph = Glyph::BOX_LDR;
                else if (y  == size_.y - 1) glyph = Glyph::BOX_LUR;
                else glyph = Glyph::BOX_LV;
            }
            else if (x == size_.x - 1)
            {
                if (y == 0) glyph = Glyph::BOX_LDL;
                else if (y == size_.y - 1) glyph = Glyph::BOX_LUL;
                else glyph = Glyph::BOX_LV;
            }
            else if (y == 0 || y == size_.y - 1) glyph = Glyph::BOX_LH;
            else continue;

            put(glyph, {x, y}, colour);
        }
    }
}

// Clears/fills a Window.
void Window::clear(Colour col)
{ render_texture_->clear(ColourMap::colour_to_sf(col)); }

// Gets the central column and row of this Window.
Vector2 Window::get_middle() const { return size_ / 2; }

// Moves this Window to new coordinates.
void Window::move(Vector2 new_pos) { pos_ = new_pos; }

// Read-only access to the Window's position.
Vector2 Window::pos() const { return pos_; }

// Prints a string at given coordinates.
void Window::print(std::string str, Vector2 pos, Colour colour, Font font) { terminal().print(*render_texture_, str, pos, colour, font); }

// Writes a character on the Window.
void Window::put(int ch, Vector2 pos, Colour colour, Font font)
{
    if (pos.x < 0 || pos.y < 0 || pos.x >= size_.x || pos.y >= size_.y) return;
    terminal().put(*render_texture_, ch, pos, colour, font);
}

// As above, but using a Glyph enum.
void Window::put(Glyph gl, Vector2 pos, Colour colour, Font font) { put(static_cast<int>(gl), pos, colour, font); }

// Erases one or more tiles, or draws a coloured rectangle.
void Window::rect(Vector2 pos, Vector2 size, Colour col)
{
    if (!size.x || !size.y) return;
    Prefs &pref = prefs();
    sf::RectangleShape rectangle(sf::Vector2f(size.x * Terminal::TILE_SIZE * pref.tile_scale(), size.y * Terminal::TILE_SIZE * pref.tile_scale()));
    rectangle.setPosition(sf::Vector2f(pos.x * Terminal::TILE_SIZE * pref.tile_scale(), pos.y * Terminal::TILE_SIZE * pref.tile_scale()));
    rectangle.setFillColor(ColourMap::colour_to_sf(col));
    render_texture_->draw(rectangle);
}

// Retrieves the SFML render texture for this Window.
sf::RenderTexture& Window::render_texture()
{
    if (!render_texture_) throw std::runtime_error("Attempt to retrieve null window render texture!");
    return *render_texture_;
}

// Read-only access to the Window's size.
Vector2 Window::size() const { return size_; }

}   // namespace gorp
