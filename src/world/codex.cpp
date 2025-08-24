// world/codex.cpp -- The Codex class handles loading and storing all the static game data (tiles, mobiles, items, etc.) and producing copies as requested.

// SPDX-FileType: SOURCE
// SPDX-FileCopyrightText: Copyright 2025 Raine Simmons <gc@gravecat.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/core.hpp"
#include "core/game.hpp"
#include "core/terminal/terminal.hpp"
#include "misc/namegen.hpp"
#include "util/stringutils.hpp"
#include "world/codex.hpp"

namespace gorp {

// Loads the static data from the gorp.k10 datafile.
Codex::Codex() : namegen_ptr_(std::make_unique<ProcNameGen>())
{
    core().log("Loading static data into the codex...");
    namegen_ptr_->load_namelists();
}

// Destructor, explicitly frees memory used.
Codex::~Codex() { namegen_ptr_.reset(nullptr); }

// Returns a reference to the procedural name generator object.
ProcNameGen& Codex::namegen() const
{
    if (!namegen_ptr_) throw std::runtime_error("Attempt to access null ProcNameGen object!");
    return *namegen_ptr_;
}

// Shortcut instead of using game()->codex()
Codex& codex() { return game().codex(); }

}   // namespace gorp
