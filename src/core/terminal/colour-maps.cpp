// core/terminal/colour-maps.cpp -- Static std::map lookups for converting colours in various forms to colours of other various forms.

// SPDX-FileType: SOURCE
// SPDX-FileCopyrightText: Copyright 2025 Raine Simmons <gc@gravecat.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/terminal/colour-maps.hpp"

namespace gorp {

// Lookup table for converting a char like 'R' into a Colour enum.
std::map<char, Colour> ColourMap::char_to_colour_map_ = {
    { 'W', Colour::WHITE },
    { 'w', Colour::GRAY },
    { 'K', Colour::GRAY_DARK },
    { 'k', Colour::BLACK },
    { '1', Colour::RED_LIGHT },
    { 'R', Colour::RED },
    { 'r', Colour::RED_DARK },
    { '2', Colour::ORANGE_LIGHT },
    { 'O', Colour::ORANGE },
    { 'o', Colour::ORANGE_DARK },
    { '3', Colour::YELLOW_LIGHT },
    { 'Y', Colour::YELLOW },
    { 'y', Colour::YELLOW_DARK },
    { '4', Colour::GREEN_LIGHT },
    { 'G', Colour::GREEN },
    { 'g', Colour::GREEN_DARK },
    { '5', Colour::CYAN_LIGHT },
    { 'C', Colour::CYAN },
    { 'c', Colour::CYAN_DARK },
    { '6', Colour::BLUE_LIGHT },
    { 'U', Colour::BLUE },
    { 'u', Colour::BLUE_DARK },
    { '7', Colour::PURPLE_LIGHT },
    { 'P', Colour::PURPLE },
    { 'p', Colour::PURPLE_DARK },
    { '8', Colour::BROWN_LIGHT },
    { 'B', Colour::BROWN },
    { 'b', Colour::BROWN_DARK }
};

// Lookup table for converting Colour enums to sf::Color classes.
std::map<Colour, sf::Color> ColourMap::colour_to_sf_map_ = {
    { Colour::WHITE, sf::Color(255, 255, 255) },
    { Colour::GRAY, sf::Color(128, 128, 128) },
    { Colour::GRAY_DARK, sf::Color(64, 64, 64) },
    { Colour::BLACK, sf::Color(2, 2, 2) },
    { Colour::RED_DARK, sf::Color(160, 15, 15) },
    { Colour::RED, sf::Color(220, 98, 80) },
    { Colour::RED_LIGHT, sf::Color(255, 144, 114) },
    { Colour::ORANGE_DARK, sf::Color(215, 73, 34) },
    { Colour::ORANGE, sf::Color(242, 140, 58) },
    { Colour::ORANGE_LIGHT, sf::Color(246, 195, 124) },
    { Colour::YELLOW_DARK, sf::Color(237, 164, 30) },
    { Colour::YELLOW, sf::Color(255, 215, 49) },
    { Colour::YELLOW_LIGHT, sf::Color(253, 255, 117) },
    { Colour::GREEN_DARK, sf::Color(42, 157, 100) },
    { Colour::GREEN, sf::Color(130, 206, 99) },
    { Colour::GREEN_LIGHT, sf::Color(221, 255, 163) },
    { Colour::CYAN_DARK, sf::Color(67, 150, 178) },
    { Colour::CYAN, sf::Color(93, 233, 218) },
    { Colour::CYAN_LIGHT, sf::Color(155, 252, 248) },
    { Colour::BLUE_DARK, sf::Color(38, 58, 174) },
    { Colour::BLUE, sf::Color(90, 139, 222) },
    { Colour::BLUE_LIGHT, sf::Color(126, 191, 255) },
    { Colour::PURPLE_DARK, sf::Color(78, 24, 124) },
    { Colour::PURPLE, sf::Color(66, 30, 166) },
    { Colour::PURPLE_LIGHT, sf::Color(206, 144, 255) },
    { Colour::BROWN_DARK, sf::Color(116, 63, 57) },
    { Colour::BROWN, sf::Color(184, 111, 80) },
    { Colour::BROWN_LIGHT, sf::Color(228, 166, 114) }
};

#ifdef GORP_STRING_COLOUR_MAP
// Lookup table for converting strings like 'GREEN_DARK' into a Colour enum.
std::map<std::string, Colour> ColourMap::string_to_colour_map_ = {
    { "WHITE", Colour::WHITE },
    { "GRAY", Colour::GRAY },
    { "GRAY_DARK", Colour::GRAY_DARK },
    { "BLACK", Colour::BLACK },
    { "RED_LIGHT", Colour::RED_LIGHT },
    { "RED", Colour::RED },
    { "RED_DARK", Colour::RED_DARK },
    { "ORANGE_LIGHT", Colour::ORANGE_LIGHT },
    { "ORANGE", Colour::ORANGE },
    { "ORANGE_DARK", Colour::ORANGE_DARK },
    { "YELLOW_LIGHT", Colour::YELLOW_LIGHT },
    { "YELLOW", Colour::YELLOW },
    { "YELLOW_DARK", Colour::YELLOW_DARK },
    { "GREEN_LIGHT", Colour::GREEN_LIGHT },
    { "GREEN", Colour::GREEN },
    { "GREEN_DARK", Colour::GREEN_DARK },
    { "CYAN_LIGHT", Colour::CYAN_LIGHT },
    { "CYAN", Colour::CYAN },
    { "CYAN_DARK", Colour::CYAN_DARK },
    { "BLUE_LIGHT", Colour::BLUE_LIGHT },
    { "BLUE", Colour::BLUE },
    { "BLUE_DARK", Colour::BLUE_DARK },
    { "PURPLE_LIGHT", Colour::PURPLE_LIGHT },
    { "PURPLE", Colour::PURPLE },
    { "PURPLE_DARK", Colour::PURPLE_DARK },
    { "BROWN_LIGHT", Colour::BROWN_LIGHT },
    { "BROWN", Colour::BROWN },
    { "BROWN_DARK", Colour::BROWN_DARK }
};
#endif

// Converts a char like 'R' into a Colour enum.
Colour ColourMap::char_to_colour(char ch)
{
    const auto result = char_to_colour_map_.find(ch);
    if (result == char_to_colour_map_.end()) throw std::runtime_error("Invalid colour code: " + std::string(1, ch));
    return result->second;
}

// Converts a Colour enum to a sf::Color class for use in SFML.
sf::Color ColourMap::colour_to_sf(Colour colour)
{
    const auto result = colour_to_sf_map_.find(colour);
    if (result == colour_to_sf_map_.end()) throw std::runtime_error("Invalid colour code: " + std::to_string(static_cast<int>(colour)));
    return result->second;
}

#ifdef GORP_STRING_COLOUR_MAP
// Converts a string like "ORANGE" into a Colour enum.
Colour ColourMap::string_to_colour(const std::string &str)
{
    const auto result = string_to_colour_map_.find(str);
    if (result == string_to_colour_map_.end()) throw std::runtime_error("Invalid colour string: " + str);
    return result->second;
}
#endif

}   // namespace gorp
