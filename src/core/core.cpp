// core/core.cpp -- Main program entry, initialization and cleanup routines, along with pointers to the key subsystems of the game.

// SPDX-FileType: SOURCE
// SPDX-FileCopyrightText: Copyright 2025 Raine Simmons <gc@gravecat.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE, std::getenv
#include <iostream>
#include <SFML/System.hpp>

#ifdef GORP_TARGET_WINDOWS
#include <windows.h>
#endif
#ifdef GORP_TARGET_LINUX
#include <csignal>
#endif

#include "3rdparty/sam/dictionary.hpp"
#include "3rdparty/sam/sam.hpp"
#include "cmake/source.hpp"
#include "core/core.hpp"
#include "core/game.hpp"
#include "core/guru.hpp"
#include "core/prefs.hpp"
#include "core/terminal/terminal.hpp"
#include "util/file/binpath.hpp"
#include "util/file/fileutils.hpp"
#include "util/file/yaml.hpp"
#include "util/stringutils.hpp"

namespace gorp {

// Constructor, sets up the Core object.
Core::Core() : game_ptr_(nullptr), guru_ptr_(nullptr), prefs_ptr_(nullptr), terminal_ptr_(nullptr) { }

// Cleans up all Core-managed objects.
void Core::cleanup()
{
    game_ptr_.reset(nullptr);
    terminal_ptr_.reset(nullptr);
    guru_ptr_.reset(nullptr);
    prefs_ptr_.reset(nullptr);
}

// Returns a reference to the singleton Core object.
Core& Core::core()
{
    static Core the_core;
    return the_core;
}

// Returns the full path to a specified game data file.
std::string Core::datafile(const std::string file)
{
    if (!gamedata_location.size()) throw GuruMeditation("Could not locate valid gamedata folder!");
    return BinPath::merge_paths(gamedata_location, file);
}

// Destroys the singleton Core object and ends execution.
void Core::destroy_core(int exit_code)
{
    if (exit_code == EXIT_SUCCESS) log("Normal core shutdown requested.");
    else if (exit_code == EXIT_FAILURE) log("Emergency core shutdown requested.", Core::CORE_CRITICAL);
    else log("Core shutdown with unknown error code: " + std::to_string(exit_code), Core::CORE_ERROR);
    sam::cleanup();
    cleanup();
    std::exit(exit_code);
}

// Attempts to locate the gamedata folder.
void Core::find_gamedata()
{
    const std::string game_path_data = BinPath::game_path("gamedata");
    const std::string game_path_data_gorp_yml = BinPath::merge_paths(game_path_data, "gorp.yml");
    const std::string source_path_data = BinPath::merge_paths(source::SOURCE_DIR, "gamedata");
    const std::string source_path_data_gorp_yml = BinPath::merge_paths(source_path_data, "gorp.yml");
    
    if (fileutils::file_exists(game_path_data_gorp_yml))
    {
        log("Game data folder location: " + game_path_data);
        gamedata_location = game_path_data;
    }
    else if (fileutils::file_exists(source_path_data_gorp_yml))
    {
        log("Game data folder location: " + source_path_data);
        gamedata_location = source_path_data;
    }
    else throw GuruMeditation("Could not locate valid gamedata folder!");

    YAML yaml_file(datafile("gorp.yml"));
    if (!yaml_file.is_map() || !yaml_file.key_exists("gorp_gamedata_version")) throw GuruMeditation("gorp.yml: Invalid file format!");
    constexpr int expected_version = 1;
    const int data_version = std::stoi(yaml_file.val("gorp_gamedata_version"));
    if (data_version != expected_version) guru_ptr_->halt("Unexpected gamedata version!", expected_version, data_version);
}

// Returns a reference to the Game manager object.
Game& Core::game() const
{
    if (!game_ptr_) throw std::runtime_error("Attempt to access null Game pointer!");
    return *game_ptr_;
}

// Used internally only to apply the most powerful possible method to kill the process, in event of emergency.
void Core::great_googly_moogly_its_all_gone_to_shit()
{
#ifdef GORP_TARGET_WINDOWS
    TerminateProcess(GetCurrentProcess(), 1);
#else   // GORP_TARGET_WINDOWS
#if GORP_TARGET_LINUX
    std::raise(SIGKILL);
#else   // GORP_TARGET_LINUX
    std::terminate();   // Not great, but that's our fallback.
#endif  // GORP_TARGET_LINUX
#endif  // GORP_TARGET_WINDOWS
}

// Returns a reference to the Guru Meditation error-handling/logging object.
Guru& Core::guru() const
{
    if (!guru_ptr_) throw std::runtime_error("Attempt to access null Guru pointer!");
    return *guru_ptr_;
}

// Checks if the Guru Meditation object currently exists.
bool Core::guru_exists() const { return (guru_ptr_ != nullptr); }

// Sets up the core game classes and data, and the terminal subsystem.
void Core::init_core(std::vector<std::string> parameters)
{
    guru_ptr_ = std::make_unique<Guru>();
    try
    {
        bool headless = false;
        for (auto param : parameters)
        {
            if (param == "-say") headless = true;
        }

        find_gamedata();
        if (!headless)
        {
            prefs_ptr_ = std::make_unique<Prefs>();
            terminal_ptr_ = std::make_unique<Terminal>();
            game_ptr_ = std::make_unique<Game>();
        }
        sam::SAMDict::load_strings();
    }
    catch(const GuruMeditation &e) { guru_ptr_->halt(e.what(), e.error_a(), e.error_b()); }
    catch(const std::exception& e) { guru_ptr_->halt(e); }
}

// Logs a message in the system log, or prints it to std::cout.
void Core::log(const std::string &str, int type)
{
    if (guru_ptr_) guru_ptr_->log(str, type);
    else
    {
        std::string txt_tag;
        switch(type)
        {
            case CORE_INFO: break;
            case CORE_WARN: txt_tag = "[WARN] "; break;
            case CORE_ERROR: txt_tag = "[ERROR] "; break;
            case CORE_CRITICAL: txt_tag = "[CRITICAL] "; break;
        }
        std::cout << txt_tag << str << std::endl;
    }
}

// Reports a non-fatal error, which will be logged but won't halt execution unless it cascades.
void Core::nonfatal(std::string error, int type)
{
    if (guru_ptr_) guru_ptr_->nonfatal(error, type);
    else this->log(error, type);
}

// Returns a reference to the Prefs object.
Prefs& Core::prefs() const
{
    if (!prefs_ptr_) throw std::runtime_error("Attempt to access null Prefs pointer!");
    return *prefs_ptr_;
}

// Returns a reference to the Terminal handler object.
Terminal& Core::terminal() const
{
    if (!terminal_ptr_) throw std::runtime_error("Attempt to access null Terminal pointer!");
    return *terminal_ptr_;
}

// A shortcut to using Core::core().
Core& core() { return Core::core(); }

}

// Main program entry point. Must be OUTSIDE the gorp namespace.
int main(int argc, char** argv)
{
    using namespace gorp;

    // Create the main Core object.
    std::vector<std::string> parameters(argv + 1, argv + argc);
    try { core().init_core(parameters); }
    catch (std::exception &e)
    {
        std::cout << "[FATAL] " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // Check command-line parameters.
    if (parameters.size() && parameters.at(0) == "-say")
    {
        parameters.erase(parameters.begin());
        std::string sam_say = stringutils::join_words(parameters);
        sam::sam_say(sam_say);
        sf::sleep(sf::milliseconds((100 * sam_say.size()) + 200));
        core().destroy_core(EXIT_SUCCESS);
    }

    // Start the ball rolling. Everything from this point will be handled by the game manager.
    try { game().begin(); }
    catch(const GuruMeditation &e)
    {
        if (gorp::core().guru_exists()) gorp::core().guru().halt(e.what(), e.error_a(), e.error_b());
        else
        {
            std::cout << e.what() << " " << e.error_a() << ":" << e.error_b() << std::endl;
            core().destroy_core(EXIT_FAILURE);
        }
    }
    catch(const std::exception &e)
    {
        if (gorp::core().guru_exists()) gorp::core().guru().halt(e);
        else
        {
            std::cout << e.what() << std::endl;
            core().destroy_core(EXIT_FAILURE);
        }
    }

    // Trigger cleanup code.
    core().destroy_core(EXIT_SUCCESS);
    return EXIT_SUCCESS;    // Technically not needed, as destroy_core() calls exit(), but this'll keep the compiler happy.
}
