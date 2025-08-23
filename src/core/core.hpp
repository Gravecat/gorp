// core/core.hpp -- Main program entry, initialization and cleanup routines, along with pointers to the key subsystems of the game.

// SPDX-FileType: SOURCE
// SPDX-FileCopyrightText: Copyright 2025 Raine Simmons <gc@gravecat.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/global.hpp"

namespace gorp {

//class Game;     // defined in core/game.hpp
class Guru;     // defined in core/guru.hpp
class Prefs;    // defined in misc/prefs.hpp
class Terminal; // defined in core/terminal.hpp

class Core {
public:
    static constexpr int    CORE_INFO =     0;  // General logging information.
    static constexpr int    CORE_WARN =     1;  // Warnings, non-fatal stuff.
    static constexpr int    CORE_ERROR =    2;  // Serious errors. Shit is going down.
    static constexpr int    CORE_CRITICAL = 3;  // Critical system failure.

    std::string     datafile(const std::string file);   // Returns the full path to a specified game data file.
//    Game&           game() const;               // Returns a reference to the Game manager object.
    Guru&           guru() const;               // Returns a reference to the Guru Meditation error-handling/logging object.
    bool            guru_exists() const;        // Checks if the Guru Meditation object currently exists.
    void            log(const std::string &str, int type = Core::CORE_INFO);    // Logs a message in the system log, or prints it to std::cout.
    void            nonfatal(std::string error, int type);  // Reports a non-fatal error, which will be logged but won't halt execution unless it cascades.
    Prefs&          prefs() const;              // Returns a reference to the Prefs object.
    Terminal&       terminal() const;           // Returns a reference to the Terminal handler object.

    static Core&    core(); // Returns a reference to the singleton Core object.
    void            init_core(std::vector<std::string> parameters);   // Sets up the core game classes and data, and the terminal subsystem.
    void            destroy_core(int exit_code);    // Destroys the singleton Core object and ends execution.

private:
            Core();             // Constructor, sets up the Core object.
    void    cleanup();          // Attempts to gracefully clean up memory and subsystems.
    void    find_gamedata();    // Attempts to locate the gamedata folder.
    void    great_googly_moogly_its_all_gone_to_shit(); // Applies the most powerful possible method to kill the process, in event of emergency.

    std::string gamedata_location;  // The path of the game's data files.

//    std::unique_ptr<Game>       game_ptr_;      // Pointer to the Game manager object, which handles the current game state.
    std::unique_ptr<Guru>       guru_ptr_;      // Pointer to the Guru Meditation object, which handles errors and logging.
    std::unique_ptr<Prefs>      prefs_ptr_;     // Pointer to the Prefs object, which records simple user preferences.
    std::unique_ptr<Terminal>   terminal_ptr_;  // Pointer to the Terminal object, which handles rendering on a real or virtual terminal window.
};

Core&   core(); // A shortcut to using Core::core().

}   // namespace gorp
