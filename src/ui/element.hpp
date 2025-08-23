// ui/element.hpp -- The Element class is a generic type of UI element that has standard features (such as redrawing), handled by the game manager.

// SPDX-FileType: SOURCE
// SPDX-FileCopyrightText: Copyright 2025 Raine Simmons <gc@gravecat.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/global.hpp"

namespace gorp {

class Window;   // defined in core/window.hpp

class Element {
public:
                    Element();              // Constructor, sets a null window pointer and assigns itself a unique UI ID.
                    Element(bool set_always_redraw);    // Constructor which also calls always_redraw().
    virtual         ~Element() = default;   // Virtual destructor
    void            always_redraw(bool toggle = true);  // Sets whether or not this element always redraws every frame.
    bool            check_if_needs_redraw() const;      // Checks if this UI element needs to be redrawn.
    void            destroy_window();       // Orders this UI element to safely destroy its Window.
    uint32_t        id() const;             // Retrieves the unique ID of this UI element.
    void            needs_redraw(bool toggle = true);   // Tells this UI element if it needs to be redrawn.
    virtual bool    process_input(int key); // Where this UI element reacts to input from the player, if it's at the top of the stack.
    virtual void    recreate_window() = 0;  // (Re)creates the render window for this UI element.
    virtual void    render() = 0;           // Renders this UI element on the screen.

protected:
    bool        needs_redraw_;  // Does this Element need to be redrawn?
    Window*     window_;        // The UI element's render window.

private:
    bool        always_redraw_; // Whether or not this UI element always redraws every frame.
    uint32_t    id_;            // The unique ID of this UI element.
};

}   // namespace gorp
