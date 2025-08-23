// core/game.hpp -- The Game object is the central game manager, handling the main loop, saving/loading, and starting of new games.

// SPDX-FileType: SOURCE
// SPDX-FileCopyrightText: Copyright 2025 Raine Simmons <gc@gravecat.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/global.hpp"

namespace gorp {

class Element;      // defined in ui/element.hpp
class TitleScreen;  // defined in ui/title.hpp

class Game {
public:
                Game();             // Constructor, sets up the game manager.
                ~Game();            // Destructor, cleans up attached classes.
    uint32_t    add_element(std::unique_ptr<Element> element);  // Adds a new UI element to the screen.
    void        begin();            // Starts the game, in the form of a title screen followed by the main game loop.
    void        delete_element(uint32_t id);    // Deletes a specified UI element.
    Element&    element(uint32_t id) const;     // Retrieves a specified UI element.
    void        leave_game();       // Shuts things down cleanly and exits the game.
    uint32_t    unique_ui_id();     // Returns a new, unique UI element ID.

private:
    void    main_loop();        // brøether, may i have the lööps
    void    clear_elements();   // Clears all UI elements.
    void    new_game();         // Sets up for a new game!

    std::unique_ptr<TitleScreen>    title_screen_ptr_;  // Pointer to the title screen object.
    std::vector<std::unique_ptr<Element>>   ui_elements_;       // The UI elements on screen right now.
    uint32_t                        ui_element_id_counter_;     // The counter for generating unique UI element IDs.
};

Game&   game(); // A shortcut instead of using core().game()

}   // namespace gorp
