// core/terminal/terminal.cpp -- The Terminal class handles rendering a faux-terminal using SFML.

// SPDX-FileType: SOURCE
// SPDX-FileCopyrightText: Copyright 2025 Raine Simmons <gc@gravecat.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cmath>
#include <cstdlib>  // EXIT_SUCCESS
#include <string_view>

#ifdef GORP_TARGET_WINDOWS
#include <windows.h>
#endif

#include "3rdparty/sfxr/sfxr.hpp"
#include "cmake/version.hpp"
#include "core/core.hpp"
#include "core/guru.hpp"
#include "core/prefs.hpp"
#include "core/terminal/colour-maps.hpp"
#include "core/terminal/terminal.hpp"
#include "core/terminal/window.hpp"
#include "util/file/filereader.hpp"
#include "util/file/fileutils.hpp"
#include "util/file/yaml.hpp"
#include "util/stringutils.hpp"

#ifdef GORP_TARGET_LINUX
#include <X11/Xlib.h>   // Include this *after* SFML or the compiler goes crazy.
#endif

namespace gorp {

// Alagard bitmap font defintitions. In order, the numbers are: width, pos_top, pos_bottom, pad_left, pad_right.
const std::vector<Terminal::alagard_char> Terminal::alagard_vec = {
    { 8, 0, 248, 1, 0 }, { 8, 8, 256, 0, 1 }, { 8, 16, 264, 1, 1 }, { 9, 24, 272, 0, 1 }, { 9, 33, 281, 0, 0 }, { 9, 42, 290, 0, 0 },       // A-F
    { 8, 51, 299, 1, 1 }, { 9, 60, 308, 0, 0 }, { 4, 68, 316, 0, 0 }, { 3, 72, 320, 0, 1 }, { 9, 75, 323, 0, 0 }, { 7, 84, 332, 0, 0 },     // G-L
    { 12, 91, 339, 0, 0}, { 10, 103, 351, 0, 0}, { 8, 113, 361, 1, 1}, { 8, 128, 376, 0, 1}, { 9, 136, 384, 1, 0}, { 9, 145, 393, 0, 0},    // M-R
    { 8, 154, 402, 1, 1}, { 9, 162, 410, 0, 1}, { 10, 171, 419, 0, 0}, { 9, 181, 429, 0, 1}, { 11, 190, 438, 0, 1}, { 9, 201, 449, 0, 0},   // S-X
    { 9, 210, 458, 0, 1}, { 8, 219, 467, 0, 1}, { 7, 227, 475, 0, 1}, { 10, 234, 482, 1, 1}, { 2, 244, 492, 1, 1}   // Y-Z, ?, /, .
};

// Constructor, sets up default values but does not initialize the faux-terminal.
Terminal::Terminal() : current_frame_(nullptr), previous_frame_(nullptr), degauss_sound_(nullptr), sfxr_(nullptr), sprite_max_(0), window_pixels_({0, 0})
{
    core().log("Attempting to initialize SFML and create OpenGL context.");

    // Load the degauss sound from the game data.
    std::vector<char> degauss_data = fileutils::file_to_char_vec(core().datafile("ogg/crt-degauss.ogg"));
    if (!degauss_sound_buffer_.loadFromMemory(degauss_data.data(), degauss_data.size())) throw std::runtime_error("Could not load audio file: crt-degauss.ogg");
    degauss_sound_ = std::make_unique<sf::Sound>(degauss_sound_buffer_);
    degauss_data.clear();

    // Define the desired OpenGL context settings.
    sf::ContextSettings gl_settings;
    gl_settings.majorVersion = 3;  // OpenGL major version.
    gl_settings.minorVersion = 0;  // OpenGL minor version.
    gl_settings.attributeFlags = sf::ContextSettings::Default;

    sf::Vector2u window_size(800, 600);
    window_pixels_ = Vector2(window_size.x, window_size.y);

    main_window_.create(sf::VideoMode(window_size), ("GORP " + version::VERSION_STRING).c_str(), sf::State::Windowed, gl_settings);
    sf::ContextSettings actual_settings = main_window_.getSettings();
    core().log("OpenGL version: " + std::to_string(actual_settings.majorVersion) + "." + std::to_string(actual_settings.minorVersion) +
        " (requested " + std::to_string(gl_settings.majorVersion) + "." + std::to_string(gl_settings.minorVersion) + ").");
    if (actual_settings.majorVersion < gl_settings.majorVersion || (actual_settings.majorVersion == gl_settings.majorVersion &&
        actual_settings.minorVersion < gl_settings.minorVersion)) core().nonfatal("OpenGL version older than requested!", Core::CORE_ERROR);
    main_window_.setFramerateLimit(60);
    recreate_frames();

    // Get the screen resolution of the primary monitor.
#ifdef GORP_TARGET_WINDOWS
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);
#else
    Display* display = XOpenDisplay(nullptr);
    if (!display) throw std::runtime_error("Failed to open X display!");
    Screen* screen = DefaultScreenOfDisplay(display);
    int screen_width = screen->width;
    int screen_height = screen->height;
    XCloseDisplay(display);
#endif
    // Set the window on the central point on the screen.
    sf::Vector2i centered_position((screen_width - window_size.x) / 2, (screen_height - window_size.y) / 2);
    main_window_.setPosition(centered_position);

    sf::Image window_icon = load_png("ghost");
    main_window_.setIcon(window_icon);

    // Load the GLSL shader from the data files.
    std::vector<char> fragment_shader = fileutils::file_to_char_vec(core().datafile("shader/shader.glsl"));
    fragment_shader.push_back(0);   // C-style strings expect a null byte terminator.
    std::string_view fragment_sv(fragment_shader.data());
    if (!shader_.loadFromMemory(fragment_sv, sf::Shader::Type::Fragment)) throw std::runtime_error("Could not load GLSL shader!");

    // Load shader parameters, and set them as needed.
    YAML yaml_file(core().datafile("shader/uniforms.yml"));
    if (!yaml_file.is_map()) throw GuruMeditation("Invalid file format: shader/uniforms.yml");
    std::vector<std::string> children = yaml_file.keys();
    for (auto key : children)
        shader_.setUniform(key.c_str(), std::stof(yaml_file.val(key)));
    shader_.setUniform("tex", current_frame_->getTexture());
    shader_.setUniform("textureSize", sf::Vector2f(current_frame_->getSize()));

    core().log("SFML initialized successfully.");
    load_sprites();
    load_sfxr();
    core().log("Bitmap font and sound effects loaded successfully.");
    core().guru().console_ready(true);
}

// Destructor, ensures memory is freed in a predictable order.
Terminal::~Terminal()
{
    for (unsigned int i = 0; i < window_stack_.size(); i++)
        window_stack_.at(i).reset(nullptr);
    sfxr_.reset(nullptr);
    previous_frame_.reset(nullptr);
    current_frame_.reset(nullptr);
    degauss_sound_.reset(nullptr);
}

// Adds a new Window to the stack. This is called automatically from Window's constructor.
Window* Terminal::add_window(Vector2 new_size, Vector2 new_pos) {
    window_stack_.push_back(std::make_unique<Window>(new_size, new_pos));
    return window_stack_.back().get();
}

void Terminal::alagard_print(sf::RenderTexture &tex, const std::string &str, Vector2 pixel_pos, Colour colour)
{
    Prefs &pref = prefs();
    const std::string processed = alagard_process(str);
    int x_pos = 0;
    for (unsigned int i = 0; i < processed.size(); i++)
    {
        char ch = processed.at(i);
        if (ch == 29) { x_pos += 8; continue; } // space
        alagard_char al = alagard_vec.at(ch);
        if (i > 0) x_pos += al.pad_left;
        
        // Calculate the texture rectangles for the sprite sheet.
        const sf::IntRect texture_rect_top(sf::Vector2i(al.pos_top, ALAGARD_PIXEL_Y), sf::Vector2i(al.width, TILE_SIZE));
        const sf::IntRect texture_rect_bot(sf::Vector2i(al.pos_bottom, ALAGARD_PIXEL_Y), sf::Vector2i(al.width, TILE_SIZE));

        // Make sprites from the tilesheet texture.
        sf::Sprite sprite_top(sprite_sheet_, texture_rect_top);
        sf::Sprite sprite_bot(sprite_sheet_, texture_rect_bot);
        const sf::Vector2f scaling(pref.tile_scale(), pref.tile_scale());
        sprite_top.setScale(scaling);
        sprite_bot.setScale(scaling);

        // Set the sprites' colour.
        sf::Color sf_col = ColourMap::colour_to_sf(colour);
        if (pref.shader()) // Make the colour more vibrant if we're using a shader.
        {
            sf_col.r = std::min(255, static_cast<int>(sf_col.r * 1.2f));
            sf_col.g = std::min(255, static_cast<int>(sf_col.g * 1.2f));
            sf_col.b = std::min(255, static_cast<int>(sf_col.b * 1.2f));
        }
        sprite_top.setColor(sf_col);
        sprite_bot.setColor(sf_col);

        // Position and scale the sprites, then render them.
        sprite_top.setPosition(sf::Vector2f((pixel_pos.x + x_pos) * scaling.x, pixel_pos.y * scaling.y));
        sprite_bot.setPosition(sf::Vector2f((pixel_pos.x + x_pos) * scaling.x, (pixel_pos.y + TILE_SIZE) * scaling.y));

        // Draw the sprites.
        tex.draw(sprite_top);
        tex.draw(sprite_bot);

        // Move the render position for the next character.
        x_pos += al.width + al.pad_right;
    }
}

// Processes a string, replacing every letter with one that can be rendered in the Alagard font.
std::string Terminal::alagard_process(const std::string &str) const
{
    std::string result = str;
    for (unsigned int i = 0; i < result.size(); i++)
    {
        char ch = result[i];
        if (ch >= 'A' && ch <= 'Z') ch -= 65;
        else if (ch >= 'a' && ch <= 'z') ch -= 97;
        else if (ch == ' ') ch = 29;
        else if (ch == '?') ch = 26;
        else if (ch == '/') ch = 27;
        else if (ch == '.') ch = 28;
        else
        {
            core().nonfatal("Invalid character in Alagard string: " + std::string(1, ch), Core::CORE_WARN);
            ch = 26;
        }
        result.at(i) = ch;
    }
    return result;
}

// Returns the width IN PIXELS of a string rendered in the Alagard font.
int Terminal::alagard_strlen(const std::string str) const
{
    std::string processed = alagard_process(str);
    int len = 0;
    for (unsigned int i = 0; i < processed.size(); i++)
    {
        char ch = processed.at(i);
        if (ch < 0 || ch > 29) throw std::runtime_error("Invalid character in Alagard string: " + std::string(1, ch));
        if (ch == 29) len += 8;
        else
        {
            alagard_char al = alagard_vec.at(ch);
            len += al.width;
            if (i > 0) len += al.pad_left;
            if (i < processed.size() - 1) len += al.pad_right;
        }
    }
    return len;
}

// Refreshes the terminal after rendering.
void Terminal::flip()
{
    // Clear the main render surface.
    current_frame_->clear(sf::Color(2, 2, 2));

    // Update the shader's timer.
    static sf::Clock clock;
    float time = clock.getElapsedTime().asSeconds();
    shader_.setUniform("time", time);

    // Render any stacked windows.
    for (unsigned int i = 0; i < window_stack_.size(); i++)
    {
        Window* win = window_stack_.at(i).get();
        if (!win)   // This shouldn't happen, but it can't hurt to be safe.
        {
            window_stack_.erase(window_stack_.begin() + i);
            i--;
            continue;
        }
        win->render_texture().display();

        sf::Sprite win_sprite(win->render_texture().getTexture());
        sf::Vector2f render_pos((win->pos().x + render_offset().x) * TILE_SIZE * prefs().tile_scale(),
            (win->pos().y + render_offset().y) * TILE_SIZE * prefs().tile_scale());
        win_sprite.setPosition(render_pos);
        current_frame_->draw(win_sprite);
    }

    // Finish drawing the current frame.
    current_frame_->display();

    // Blend the previous frame with reduced opacity
    if (prefs().shader())
    {
        sf::Sprite previous_sprite(previous_frame_->getTexture());
        previous_sprite.setColor(sf::Color(255, 255, 255, 200));    // Adjust alpha for ghosting.
        current_frame_->draw(previous_sprite);  // Blend previous frame onto current frame.
        current_frame_->display();

        // Copy the current frame to the previous frame.
        previous_frame_->clear(sf::Color(4, 4, 4));
        previous_frame_->draw(sf::Sprite(current_frame_->getTexture()));
        previous_frame_->display();
    }
    else
    {
        previous_frame_->clear(sf::Color(4, 4, 4));
        current_frame_->display();
    }

    // Render the final frame with the shader
    sf::RenderStates states;
    states.shader = &shader_;
    sf::Sprite sprite(current_frame_->getTexture());
    main_window_.clear(sf::Color(4, 4, 4));
    if (prefs().shader()) main_window_.draw(sprite, states);
    else main_window_.draw(sprite);
    main_window_.display();
}

// Gets keyboard input from the user.
int Terminal::get_key()
{
    if (!main_window_.isOpen()) core().destroy_core(EXIT_SUCCESS);
    Prefs &pref = prefs();

    auto adjust_tile_scale = [this, &pref](int adj) {
        int new_scale = pref.tile_scale() + adj;
        if (new_scale < 1 || new_scale > 10) play_sound("fail");
        else
        {
            pref.set_tile_scale(new_scale);
            recreate_frames();
        }
    };

    while (const std::optional event = main_window_.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            main_window_.close();
            core().destroy_core(EXIT_SUCCESS);
        }
        else if (const auto* resized = event->getIf<sf::Event::Resized>())
        {
            window_pixels_ = Vector2(resized->size.x, resized->size.y);
            sf::Vector2f zero_zero(0, 0), screen_vec(resized->size.x, resized->size.y);
            sf::FloatRect visible_area(zero_zero, screen_vec);
            main_window_.setView(sf::View(visible_area));
            shader_.setUniform("scanlineCount", static_cast<float>(resized->size.y) / 3.0f);
            uint32_t total_pixels = window_pixels_.x * window_pixels_.y;

            if (pref.auto_rescale())
            {
                if (pref.tile_scale() > 2 && total_pixels < 960000) pref.set_tile_scale(2);
                else if (pref.tile_scale() < 3 && total_pixels >= 960000) pref.set_tile_scale(3);
            }
            recreate_frames();
            return Key::RESIZE;
        }
        if (const auto* text_entered = event->getIf<sf::Event::TextEntered>())
        {
            if (text_entered->unicode == '+') { adjust_tile_scale(1); return Key::RESIZE; }
            else if (text_entered->unicode == '-') { adjust_tile_scale(-1); return Key::RESIZE; }
            else if (text_entered->unicode >= ' ' && text_entered->unicode < 128) return text_entered->unicode;
        }
        if (const auto* key_pressed = event->getIf<sf::Event::KeyPressed>())
        {
            switch (key_pressed->scancode)
            {
                case sf::Keyboard::Scancode::F1: pref.set_shader(!pref.shader()); return Key::RESIZE;
                case sf::Keyboard::Scancode::Backspace: return Key::BACKSPACE;
                case sf::Keyboard::Scancode::Tab: return Key::TAB;
                case sf::Keyboard::Scancode::Enter: return Key::ENTER;
                case sf::Keyboard::Scancode::Up: return Key::ARROW_UP;
                case sf::Keyboard::Scancode::Down: return Key::ARROW_DOWN;
                case sf::Keyboard::Scancode::Left: return Key::ARROW_LEFT;
                case sf::Keyboard::Scancode::Right: return Key::ARROW_RIGHT;
                case sf::Keyboard::Scancode::Delete: return Key::DELETE_KEY;
                case sf::Keyboard::Scancode::Insert: return Key::INSERT;
                case sf::Keyboard::Scancode::Home: return Key::HOME;
                case sf::Keyboard::Scancode::End: return Key::END;
                case sf::Keyboard::Scancode::PageUp: return Key::PAGE_UP;
                case sf::Keyboard::Scancode::PageDown: return Key::PAGE_DOWN;
                case sf::Keyboard::Scancode::F2: return Key::F2;
                case sf::Keyboard::Scancode::F3: return Key::F3;
                case sf::Keyboard::Scancode::F4: return Key::F4;
                case sf::Keyboard::Scancode::F5: return Key::F5;
                case sf::Keyboard::Scancode::F6: return Key::F6;
                case sf::Keyboard::Scancode::F7: return Key::F7;
                case sf::Keyboard::Scancode::F8: return Key::F8;
                case sf::Keyboard::Scancode::F9: return Key::F9;
                case sf::Keyboard::Scancode::F10: return Key::F10;
                case sf::Keyboard::Scancode::F11: return Key::F11;
                case sf::Keyboard::Scancode::F12: return Key::F12;
                case sf::Keyboard::Scancode::Numpad0: return Key::KP0;
                case sf::Keyboard::Scancode::Numpad1: return Key::KP1;
                case sf::Keyboard::Scancode::Numpad2: return Key::KP2;
                case sf::Keyboard::Scancode::Numpad3: return Key::KP3;
                case sf::Keyboard::Scancode::Numpad4: return Key::KP4;
                case sf::Keyboard::Scancode::Numpad5: return Key::KP5;
                case sf::Keyboard::Scancode::Numpad6: return Key::KP6;
                case sf::Keyboard::Scancode::Numpad7: return Key::KP7;
                case sf::Keyboard::Scancode::Numpad8: return Key::KP8;
                case sf::Keyboard::Scancode::Numpad9: return Key::KP9;
                case sf::Keyboard::Scancode::Escape: return Key::ESCAPE;
                default: break;
            }
        }
    }
    flip(); // If nothing else is happening right now, we can take the opportunity to update the screen.
    return 0;
}

// Gets the central column and row of the screen.
Vector2 Terminal::get_middle() const { return size() / 2; }

// Loads a PNG from the data files.
sf::Image Terminal::load_png(const std::string &filename)
{
    std::vector<char> blob = fileutils::file_to_char_vec(core().datafile("png/" + filename + ".png"));
    sf::Image image;
    if (!image.loadFromMemory(blob.data(), blob.size())) throw std::runtime_error("Failed to load image: " + filename);
    return image;
}

// Loads the sfxr sound samples.
void Terminal::load_sfxr()
{
    sfxr_ = std::make_unique<SfxrSoundStream>();
    const std::string sfxr_dir = core().datafile("sfxr");
    std::vector<std::string> sfxr_files = fileutils::files_in_dir(sfxr_dir);

    for (unsigned int i = 0; i < sfxr_files.size(); i++)
    {
        const std::string filename = sfxr_files.at(i);
        FileReader fr(core().datafile("sfxr/" + filename));

        const int version = fr.read_data<int>();
        if (version < 100 || version > 102) throw GuruMeditation(filename + ": Invalid sfxr file version!", version, i);

        SfxrSample samp;
        samp.wave_type = fr.read_data<int>();
        if (version == 102) samp.sound_vol = fr.read_data<float>();
        samp.p_base_freq = fr.read_data<float>();
        samp.p_freq_limit = fr.read_data<float>();
        samp.p_freq_ramp = fr.read_data<float>();
        if (version >= 101) samp.p_freq_dramp = fr.read_data<float>();
        samp.p_duty = fr.read_data<float>();
        samp.p_duty_ramp = fr.read_data<float>();
        samp.p_vib_strength = fr.read_data<float>();
        samp.p_vib_speed = fr.read_data<float>();
        samp.p_vib_delay = fr.read_data<float>();
        samp.p_env_attack = fr.read_data<float>();
        samp.p_env_sustain = fr.read_data<float>();
        samp.p_env_decay = fr.read_data<float>();
        samp.p_env_punch = fr.read_data<float>();
        samp.filter_on = fr.read_data<bool>();
        samp.p_lpf_resonance = fr.read_data<float>();
        samp.p_lpf_freq = fr.read_data<float>();
        samp.p_lpf_ramp = fr.read_data<float>();
        samp.p_hpf_freq = fr.read_data<float>();
        samp.p_hpf_ramp = fr.read_data<float>();
        samp.p_pha_offset = fr.read_data<float>();
        samp.p_pha_ramp = fr.read_data<float>();
        samp.p_repeat_speed = fr.read_data<float>();
        if (version >= 101)
        {
            samp.p_arp_speed = fr.read_data<float>();
            samp.p_arp_mod = fr.read_data<float>();
        }
        sfxr_samples_.insert({filename, samp});
    }
}

// Load the sprites from the static data.
void Terminal::load_sprites()
{
    core().log("Loading pixel font...");
    sf::Image new_image = load_png("font");

    // Determine the size of the image.
    sf::Vector2u image_size = new_image.getSize();
    sprite_sheet_size_ = { static_cast<int>(image_size.x), static_cast<int>(image_size.y) };

    // Apply a transparency mask to any black parts of the image.
    for (unsigned int x = 0; x < image_size.x; x++)
        for (unsigned int y = 0; y < image_size.y; y++)
            if (new_image.getPixel({x, y}) == sf::Color(0,0,0))
                new_image.setPixel({x, y}, sf::Color(0, 0, 0, 0));

    if (!sprite_sheet_.loadFromImage(new_image)) throw std::runtime_error("Failed to load texture: font.png");
    sprite_max_ = (sprite_sheet_.getSize().x / TILE_SIZE) * (sprite_sheet_.getSize().y / TILE_SIZE);
}

// Plays an sfxr sound effect.
void Terminal::play_sound(const std::string &sound)
{
    const auto result = sfxr_samples_.find(sound + ".sfs");
    if (result == sfxr_samples_.end()) throw std::runtime_error("Missing sound file: " + sound);

    sfxr_->stop();
    sfxr_->load_settings(result->second);
    sfxr_->play_sample();
}

// Enables or disables the frame-limiting on rendering.
void Terminal::set_frame_limit(bool enable) { main_window_.setFramerateLimit(enable ? 60 : 0); }

// Internal rendering code.
void Terminal::print(sf::RenderTexture &tex, std::string str, Vector2 pos, Colour colour, Font font)
{
    while (str.size())
    {
        std::string first_word;
        size_t tag_pos = str.substr(1).find_first_of('{');  // We skip ahead one char, to ignore the opening brace. We want to find the *next* opening brace.
        if (tag_pos != std::string::npos)
        {
            first_word = str.substr(0, tag_pos + 1);
            str = str.substr(tag_pos + 1);
        }
        else
        {
            first_word = str;
            str = "";
        }

        while (first_word.size() >= 3 && first_word[0] == '{' && first_word[2] == '}')
        {
            const std::string tag = first_word.substr(0, 3);
            first_word = first_word.substr(3);
            colour = ColourMap::char_to_colour(tag[1]);
        }

        for (char ch : first_word)
            put(tex, ch, {pos.x++, pos.y}, colour, font);
    }
}

// Internal rendering code.
void Terminal::put(sf::RenderTexture &tex, int ch, Vector2 pos, Colour colour, Font font)
{
    Prefs &pref = prefs();
    bool half_font = false;
    switch(font)
    {
        case Font::NORMAL: break;
        case Font::TRIHOOK: ch += 256; break;
        case Font::TRIHOOK_HALF: ch += 768; half_font = true; break;
    }

    // Check for valid sprite position.
    if (static_cast<uint32_t>(ch) >= sprite_max_) throw GuruMeditation("Invalid sprite tile!", ch);

    // Calculate the coordinates on the sprite sheet.
    int tile_x, tile_y;
    if (half_font)
    {
        tile_x = (ch % (sprite_sheet_size_.x / (TILE_SIZE / 2))) * (TILE_SIZE / 2);
        tile_y = (ch / (sprite_sheet_size_.x / (TILE_SIZE / 2))) * TILE_SIZE;
    }
    else
    {
        tile_x = (ch % (sprite_sheet_size_.x / TILE_SIZE)) * TILE_SIZE;
        tile_y = (ch / (sprite_sheet_size_.x / TILE_SIZE)) * TILE_SIZE;
    }

    // Calculate the texture rectangle for the sprite sheet.
    sf::IntRect texture_rect({tile_x, tile_y}, {TILE_SIZE / (half_font ? 2 : 1), TILE_SIZE});

    // Make a sprite from this texture.
    sf::Sprite sprite(sprite_sheet_, texture_rect);
    sprite.setScale(sf::Vector2f(pref.tile_scale(), pref.tile_scale()));

    // Set the sprite's colour.
    if (colour != Colour::NONE)
    {
        sf::Color sf_col = ColourMap::colour_to_sf(colour);
        if (pref.shader()) // Make the colour more vibrant if we're using a shader.
        {
            sf_col.r = std::min(255, static_cast<int>(sf_col.r * 1.2f));
            sf_col.g = std::min(255, static_cast<int>(sf_col.g * 1.2f));
            sf_col.b = std::min(255, static_cast<int>(sf_col.b * 1.2f));
        }
        sprite.setColor(sf_col);
    }

    // Position and scale the sprite, then render it.
    sprite.setPosition(sf::Vector2f((pos.x * TILE_SIZE * pref.tile_scale()) / (half_font ? 2 : 1), pos.y * TILE_SIZE * pref.tile_scale()));

    tex.draw(sprite);
}

// Recreates the frame textures, after the window has resized.
void Terminal::recreate_frames()
{
    sf::Vector2u window_size(window_pixels_.x, window_pixels_.y);

    current_frame_ = std::make_unique<sf::RenderTexture>(window_size);
    current_frame_->clear(sf::Color(255, 255, 255));
    current_frame_->display();

    previous_frame_ = std::make_unique<sf::RenderTexture>(window_size);
    previous_frame_->clear(sf::Color(255, 255, 255));
    previous_frame_->display();

    shader_.setUniform("tex", current_frame_->getTexture());
    shader_.setUniform("textureSize", sf::Vector2f(current_frame_->getSize()));

    static bool first_degauss = true;
    degauss_sound_->play();
    if (!first_degauss) degauss_sound_->setPlayingOffset(sf::milliseconds(550));
    first_degauss = false;
}

// Removes a Window from the stack. This is called automatically from Window's destructor.
void Terminal::remove_window(Window* win)
{
    for (unsigned int i = 0; i < window_stack_.size(); i++)
    {
        if (window_stack_.at(i).get() == win)
        {
            window_stack_.erase(window_stack_.begin() + i);
            return;
        }
    }
    core().nonfatal("Attempt to remove nonexistent window from stack.", Core::CORE_ERROR);
}

// Applies an offset to rendering, to avoid useless (obscured by the bevel) tiles.
Vector2 Terminal::render_offset() const
{
    Prefs &pref = prefs();
    if (!pref.shader()) return {0, 0};

    int main_x = main_window_.getSize().x;
    int main_y = main_window_.getSize().y;

    Vector2 result({std::max(1, static_cast<int>(main_x / 1.2f) / (150 * pref.tile_scale())), std::max(1, main_y / (150 * pref.tile_scale()))});

    return result;
}

// Determines the size of the screen, in character width and height, taking tiles obscured by the shader into account.
Vector2 Terminal::size() const
{
    Vector2 result(main_window_.getSize().x / prefs().tile_scale() / TILE_SIZE, main_window_.getSize().y / prefs().tile_scale() / TILE_SIZE);
    result = result - (render_offset() * 2);
    if (result.x < 1) result.x = 1;
    if (result.y < 1) result.y = 1;
    return result;
}

// Gets the raw size of the screen in pixels, without any adjustments.
Vector2 Terminal::size_pixels() const { return Vector2(main_window_.getSize().x, main_window_.getSize().y); }

// Easier access than calling core()->terminal()
Terminal& terminal() { return core().terminal(); }

}   // namespace gorp
