// core/terminal/colour-maps.hpp -- Static std::map lookups for converting colours in various forms to colours of other various forms.

// SPDX-FileType: SOURCE
// SPDX-FileCopyrightText: Copyright 2025 Raine Simmons <gc@gravecat.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <map>

#include "core/global.hpp"
#include "SFML/Graphics/Color.hpp"

namespace gorp {

class ColourMap
{
public:
    static Colour       char_to_colour(char ch);        // Converts a char like 'R' into a Colour enum.
    static sf::Color    colour_to_sf(Colour colour);    // Converts a Colour enum to a sf::Color class for use in SFML.
#ifdef GORP_STRING_COLOUR_MAP
    static Colour       string_to_colour(const std::string &str);   // Converts a string like "ORANGE" into a Colour enum.
#endif

private:
    static std::map<char, Colour>           char_to_colour_map_;    // Lookup table for converting a char like 'R' into a Colour enum.
    static std::map<Colour, sf::Color>      colour_to_sf_map_;      // Lookup table for converting Colour enums to sf::Color classes.
#ifdef GORP_STRING_COLOUR_MAP
    static std::map<std::string, Colour>    string_to_colour_map_;  // Lookup table for converting strings like 'GREEN_DARK' into a Colour enum.
#endif
};

}   // namespace gorp
