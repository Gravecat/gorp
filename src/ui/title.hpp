// ui/title.hpp -- The title screen, displays the main menu and such.

// SPDX-FileType: SOURCE
// SPDX-FileCopyrightText: Copyright 2025 Raine Simmons <gc@gravecat.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/global.hpp"
#include "SFML/Audio/Music.hpp"

namespace gorp {

class Window;   // defined in core/terminal/window.hpp

class TitleScreen {
public:
    enum class TitleOption : uint8_t { NEW_GAME, LOAD_GAME, QUIT };

                TitleScreen();  // Initializes the title screen by loading static data.
                ~TitleScreen(); // Destructor, explicitly frees memory used.
    TitleOption render();       // Renders the title screen, and returns the user's chosen action.

private:
    void    load_title_data();  // Loads the backronym and random phrase for the title screen.
    void    redraw();           // Redraws the title screen.
    void    render_test();      // Render speed test.

    std::string                 backronym_;             // The randomly-assembled 'backronym' for GORP chosen this time around.
    bool                        blinking_;              // Whether or not the title-screen dragon is blinking.
    std::vector<char>           music_vec_;             // Where the actual data for the title-screen music is stored in memory.
    std::unique_ptr<sf::Music>  music_;                 // The title-screen music. Must be declared AFTER music_vec_. See destructor comment in title.cpp.
    std::string                 phrase_;                // The randomly-chosen phrase for the title screen.
    Window*                     title_screen_window_;   // The window where we render the title screen.
};

}   // namespace gorp
