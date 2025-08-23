// core/guru.hpp -- Guru Meditation error-handling and reporting system.

// SPDX-FileType: SOURCE
// SPDX-FileCopyrightText: Copyright 2025 Raine Simmons <gc@gravecat.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <ctime>
#include <fstream>
#include <sstream>

#include "core/global.hpp"

namespace gorp {

class Guru {
public:
    static constexpr int    GURU_INFO =     0;  // General logging information.
    static constexpr int    GURU_WARN =     1;  // Warnings, non-fatal stuff.
    static constexpr int    GURU_ERROR =    2;  // Serious errors. Shit is going down.
    static constexpr int    GURU_CRITICAL = 3;  // Critical system failure.

            Guru();                                             // Opens the output log for messages.
            ~Guru();                                            // Closes the system log gracefully.
    void    check_stderr();                                     // Checks stderr for any updates, puts them in the log if any exist.
    void    console_ready(bool is_ready = true);                // Tells Guru that we're ready to render Guru error messages on-screen.
    void    halt(std::string error, int a = 0, int b = 0);      // Stops the game and displays an error messge.
    void    halt(const std::exception &e);                      // As above, but with an exception instead of a string.
    void    hook_signals();                                     // Tells Guru to hook system failure signals.
    void    intercept_signal(int sig);                          // Catches a segfault or other fatal signal.
    bool    is_dead() const;                                    // Checks if the system has halted.
    void    log(std::string msg, int type = GURU_INFO);         // Logs a message in the system log file.
    void    nonfatal(std::string error, int type);              // Reports a non-fatal error, which will be logged but won't halt execution unless it cascades.

private:
    int                 cascade_count_;     // Keeps track of rapidly-occurring, non-fatal error messages.
    bool                cascade_failure_;   // Is a cascade failure in progress?
    time_t              cascade_timer_;     // Timer to check the speed of non-halting Guru warnings, to prevent cascade locks.
    bool                console_ready_;     // Have we fully initialized the console yet?
    bool                dead_already_;      // Have we already died? Is this crash within the Guru subsystem?
    bool                lock_stderr_;       // Whether the stderr-checking code is allowed to run or not.
    std::stringstream   sferr_buffer_;      // Stringstream buffer used to catch sf::err() error messages.
    std::stringstream   stderr_buffer_;     // Pointer to a stringstream buffer used to catch stderr messages.
    std::streambuf*     stderr_old_;        // The old stderr buffer.
    std::ofstream       syslog_;            // The system log file.

    static constexpr int    ERROR_CASCADE_THRESHOLD =       25; // The amount cascade_count can reach within CASCADE_TIMEOUT seconds before it triggers abort.
    static constexpr int    ERROR_CASCADE_TIMEOUT =         30; // The number of seconds without an error to reset the cascade timer.
    static constexpr int    ERROR_CASCADE_WEIGHT_CRITICAL = 20; // The amount a critical type log entry will add to the cascade timer.
    static constexpr int    ERROR_CASCADE_WEIGHT_ERROR =    5;  // The amount an error type log entry will add to the cascade timer.
    static constexpr int    ERROR_CASCADE_WEIGHT_WARNING =  1;  // The amount a warning type log entry will add to the cascade timer.
};

}   // namespace gorp
