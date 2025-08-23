// core/guru.cpp -- Guru Meditation error-handling and reporting system.

// SPDX-FileType: SOURCE
// SPDX-FileCopyrightText: Copyright 2025 Raine Simmons <gc@gravecat.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <csignal>
#include <cstdlib>  // EXIT_FAILURE
#include <iostream>

#include "cmake/version.hpp"
#include "core/core.hpp"
#include "core/guru.hpp"
#include "core/terminal/terminal.hpp"
#include "core/terminal/window.hpp"
#include "SFML/System.hpp"
#include "util/file/binpath.hpp"
#include "util/file/fileutils.hpp"
#include "util/stringutils.hpp"

namespace gorp {

// Custom exception type, which records error codes that can be rendered in Guru::halt()
GuruMeditation::GuruMeditation(const std::string &what_arg, int code_a, int code_b) : std::runtime_error(what_arg), error_a_(code_a), error_b_(code_b) { }
int GuruMeditation::error_a() const { return error_a_; }
int GuruMeditation::error_b() const { return error_b_; }

// This has to be a non-class function because C.
void guru_intercept_signal(int sig) { core().guru().intercept_signal(sig); }

// Opens the output log for messages.
Guru::Guru() : cascade_count_(0), cascade_failure_(false), cascade_timer_(std::time(0)), console_ready_(false), dead_already_(false), lock_stderr_(false),
    stderr_old_(nullptr)
{
    const std::string userdata_path = BinPath::game_path("userdata");
    fileutils::make_dir(userdata_path);
    fileutils::delete_file(userdata_path + "/log.txt");
    syslog_.open(userdata_path + "/log.txt");
    if (!syslog_.is_open()) throw std::runtime_error("Cannot open " + userdata_path + "/log.txt");
    hook_signals();
    stderr_old_ = std::cerr.rdbuf(stderr_buffer_.rdbuf());
    sf::err().rdbuf(sferr_buffer_.rdbuf());
    this->log("Welcome to GORP " + version::VERSION_STRING + " build " + version::BUILD_TIMESTAMP);
    this->log("Guru error-handling system is online.");
}

// Closes the system log gracefully.
Guru::~Guru()
{
    check_stderr();
    this->log("Guru Meditation system shutting down.");

    if (stderr_old_)
    {
        std::cerr.rdbuf(stderr_old_);
        stderr_old_ = nullptr;
    }

    // Drop all signal hooks.
    signal(SIGABRT, SIG_IGN);
    signal(SIGSEGV, SIG_IGN);
    signal(SIGILL, SIG_IGN);
    signal(SIGFPE, SIG_IGN);
#ifdef INVICTUS_TARGET_LINUX
    signal(SIGBUS, SIG_IGN);
#endif

    this->log("The rest is silence.");
    syslog_.close();
}

// Checks stderr for any updates, puts them in the log if any exist.
void Guru::check_stderr()
{
    if (lock_stderr_) return;
    
    lock_stderr_ = true;
    const std::string err_buffer = stderr_buffer_.str(), sf_buffer = sferr_buffer_.str();
    if (err_buffer.size())
    {
        Guru::log(err_buffer);
        stderr_buffer_.str(std::string());
        stderr_buffer_.clear();
    }
    if (sf_buffer.size())
    {
        Guru::log(sf_buffer);
        sferr_buffer_.str(std::string());
        sferr_buffer_.clear();
    }
    lock_stderr_ = false;
}

// Tells Guru that we're ready to render Guru error messages on-screen.
void Guru::console_ready(bool is_ready) { console_ready_ = is_ready; }

// Guru meditation error.
void Guru::halt(std::string error, int a, int b)
{
    check_stderr();
    this->log("Critical error occurred, halting execution.", GURU_CRITICAL);
    this->log(error, GURU_CRITICAL);
    if (dead_already_)
    {
        log("Detected cleanup in process, attempting to die peacefully.", GURU_WARN);
        core().destroy_core(EXIT_FAILURE);
    }
    else dead_already_ = true;
    std::string meditation_str = (a || b ? "Guru Meditation " + stringutils::str_toupper(stringutils::itoh(a, 8)) + "." +
        stringutils::str_toupper(stringutils::itoh(b, 8)) : "");
    if (!meditation_str.empty()) this->log(meditation_str, GURU_CRITICAL);
    //if (!console_ready_)
    {
        std::cout << error << std::endl;
        std::cout << meditation_str << std::endl;
        core().destroy_core(EXIT_FAILURE);
    }

    // We should be fine to start a display loop here -- if anything goes horribly wrong, dead_already_ is set and we should go to exit(EXIT_FAILURE).
    bool resized = true, needs_redraw = true, border = true;
    
    Terminal& term = terminal();
    int window_size = 37;
    if (error.size() >= 37) window_size = error.size();
    const auto guru_window = term.add_window(Vector2(window_size + 2, (meditation_str.empty() ? 5 : 7)));
    const Vector2 window_mid = guru_window->get_middle();

    sf::Clock blink_timer;
    while (true)
    {
        if (needs_redraw)
        {
            guru_window->clear();
            if (border) guru_window->box(Colour::RED);
            guru_window->print("Software Failure, Halting Execution", {window_mid.x - 17, 1}, Colour::RED);
            guru_window->print(error, Vector2(window_mid.x - (error.size() / 2), 3), Colour::RED);
            if (!meditation_str.empty()) guru_window->print(meditation_str, Vector2(window_mid.x - (meditation_str.size() / 2), 5), Colour::RED);
            needs_redraw = false;
        }
        if (resized)
        {
            const Vector2 term_mid = term.get_middle();
            guru_window->move({term_mid.x - window_mid.x, term_mid.y - window_mid.y});
            resized = false;
        }
        if (term.get_key() == Key::RESIZE) resized = true;
        else if (blink_timer.getElapsedTime().asMilliseconds() > 500)
        {
            blink_timer.restart();
            border = !border;
            needs_redraw = true;
        }
    }
}

// As above, but with an exception instead of a string.
void Guru::halt(const std::exception &e) { halt(e.what()); }

// Tells Guru to hook system failure signals.
void Guru::hook_signals()
{
    this->log("Guru Meditation hooking signals...");
    if (signal(SIGABRT, guru_intercept_signal) == SIG_ERR) halt("Failed to hook abort signal.");
    if (signal(SIGSEGV, guru_intercept_signal) == SIG_ERR) halt("Failed to hook segfault signal.");
    if (signal(SIGILL, guru_intercept_signal) == SIG_ERR) halt("Failed to hook illegal instruction signal.");
    if (signal(SIGFPE, guru_intercept_signal) == SIG_ERR) halt("Failed to hook floating-point exception signal.");
#ifdef INVICTUS_TARGET_LINUX
    if (signal(SIGBUS, guru_intercept_signal) == SIG_ERR) halt("Failed to hook bus error signal.");
#endif
}

// Catches a segfault or other fatal signal.
void Guru::intercept_signal(int sig)
{
    std::string sig_type;
    int a = 0, b = 0;
    switch(sig)
    {
        case SIGABRT: sig_type = "Software requested abort."; a = SIGABRT; break;
        case SIGFPE: sig_type = "Floating-point exception."; a = SIGFPE; break;
        case SIGILL: sig_type = "Illegal instruction."; a = SIGILL; break;
        case SIGSEGV: sig_type = "Segmentation fault."; a = SIGSEGV; break;
#ifdef INVICTUS_TARGET_LINUX
        case SIGBUS: sig_type = "Bus error."; a = SIGBUS; break;
#endif
        default: sig_type = "Intercepted unknown signal."; b = 0xFF; break;
    }

    // Disable the signals for now, to stop a cascade.
    signal(SIGABRT, SIG_IGN);
    signal(SIGSEGV, SIG_IGN);
    signal(SIGILL, SIG_IGN);
    signal(SIGFPE, SIG_IGN);
#ifdef INVICTUS_TARGET_LINUX
    signal(SIGBUS, SIG_IGN);
#endif
    halt(sig_type, a, b);
}

// Checks if the system has halted.
bool Guru::is_dead() const { return dead_already_; }

// Logs a message in the system log file.
void Guru::log(std::string msg, int type)
{
    if (!syslog_.is_open()) return;
    if (!lock_stderr_) check_stderr();

    std::string txt_tag;
    switch(type)
    {
        case GURU_INFO: break;
        case GURU_WARN: txt_tag = "[WARN] "; break;
        case GURU_ERROR: txt_tag = "[ERROR] "; break;
        case GURU_CRITICAL: txt_tag = "[CRITICAL] "; break;
    }

    char* buffer = new char[32];
    const time_t now = time(nullptr);
    const tm *ptm = localtime(&now);
    std::strftime(&buffer[0], 32, "%H:%M:%S", ptm);
    std::string time_str = &buffer[0];
    msg = "[" + time_str + "] " + txt_tag + msg;
    syslog_ << msg << std::endl;
    delete[] buffer;
}

// Reports a non-fatal error, which will be logged but will not halt execution unless it cascades.
void Guru::nonfatal(std::string error, int type)
{
    if (cascade_failure_ || dead_already_) return;
    int cascade_weight = 0;
    std::string colour;
    switch(type)
    {
        case GURU_WARN: cascade_weight = ERROR_CASCADE_WEIGHT_WARNING; colour = "{Y}[WARNING] "; break;
        case GURU_ERROR: cascade_weight = ERROR_CASCADE_WEIGHT_ERROR; colour = "{R}[ERROR] "; break;
        case GURU_CRITICAL: cascade_weight = ERROR_CASCADE_WEIGHT_CRITICAL; colour = "{R}[CRITICAL] "; break;
        default: nonfatal("Nonfatal error reported with incorrect severity specified.", GURU_WARN); break;
    }

    this->log(error, type);

    if (cascade_weight)
    {
        time_t elapsed_seconds = std::time(0) - cascade_timer_;
        if (elapsed_seconds <= ERROR_CASCADE_TIMEOUT)
        {
            cascade_count_ += cascade_weight;
            if (cascade_count_ > ERROR_CASCADE_THRESHOLD)
            {
                cascade_failure_ = true;
                halt("Cascade failure detected!");
            }
        }
        else
        {
            cascade_timer_ = std::time(0);
            cascade_count_ = 0;
        }
    }
}

}   // namespace gorp
