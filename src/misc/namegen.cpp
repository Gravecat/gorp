// misc/namegen.cpp -- Procedural name generation for NPCs.

// SPDX-FileType: SOURCE
// SPDX-FileCopyrightText: Copyright 2025 Raine Simmons <gc@gravecat.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "3rdparty/fantasyname/namegen.hpp"
#include "core/core.hpp"
#include "misc/namegen.hpp"
#include "util/file/fileutils.hpp"
#include "util/file/yaml.hpp"
#include "util/math/random.hpp"

namespace gorp {

// Picks a consonant from the table, for forming atoms.
std::string ProcNameGen::consonant()
{
    const int pos = random::get<unsigned int>(0, consonant_block.size() - 1);
    return consonant_block.substr(pos, 1);
}

// Loads the namelists from the data files.
void ProcNameGen::load_namelists()
{
    names_f = fileutils::file_to_vec(core().datafile("namegen/names-f.txt"));
    names_m = fileutils::file_to_vec(core().datafile("namegen/names-m.txt"));
    names_s_a = fileutils::file_to_vec(core().datafile("namegen/surname-a.txt"));
    names_s_b = fileutils::file_to_vec(core().datafile("namegen/surname-b.txt"));

    YAML yaml(core().datafile("namegen/namegen-strings.yml"));
    if (!yaml.is_map()) throw GuruMeditation("namegen-strings.yml: Invalid file format");
    if (!yaml.key_exists("consonant_block")) throw GuruMeditation("namegen-strings.yml: consonant_block missing");
    consonant_block = yaml.val("consonant_block");
    if (!yaml.key_exists("vowel_block")) throw GuruMeditation("namegen-strings.yml: vowel_block missing");
    consonant_block = yaml.val("vowel_block");
    if (!yaml.key_exists("v4_template")) throw GuruMeditation("namegen-strings.yml: v4_template missing");
    consonant_block = yaml.val("v4_template");
    pv3_c = yaml.get_seq("pv3_c");
    pv3_d = yaml.get_seq("pv3_d");
    pv3_e = yaml.get_seq("pv3_e");
    pv3_f = yaml.get_seq("pv3_f");
    pv3_i = yaml.get_seq("pv3_i");
    pv3_k = yaml.get_seq("pv3_k");
    pv3_v = yaml.get_seq("pv3_v");
    pv3_x = yaml.get_seq("pv3_x");
}

// Returns a random feminine name.
std::string ProcNameGen::name_f() { return names_f[random::get<unsigned int>(0, names_f.size() - 1)]; }

// Returns a random masculine name.
std::string ProcNameGen::name_m() { return names_m[random::get<unsigned int>(0, names_m.size() - 1)]; }

// Generates a random name (v1 code, Elite-style).
std::string ProcNameGen::namegen_v1()
{
    std::string name, atom;

    // Construct a max-length name to begin with.
    for (int i = 0; i < 4; i++)
    {
        // The type of atom chosen depends on the seed.
        const int choice = random::get(1, 10);

        // Assign the appropriate type of atom.
        if (choice >= 1 && choice <= 3) atom = vowel() + consonant();
        else if (choice >= 4 && choice <= 7) atom = consonant() + vowel();
        else if (choice == 8 || choice == 9) atom = vowel() + vowel();
        else if (choice == 10) atom = consonant() + consonant();

        // Add the atom to the name-in-progress.
        name += atom;
    }

    // Trim the name's length down to a specified number.
    const int length = random::get(4, 8);
    name = name.substr(0, length);

    // Make the first letter of the name capitalized.
    name[0] -= 32;
    return name;
}

// Generates a name with the v4 generator (front-end to Skeeto's fantasy name generator).
std::string ProcNameGen::namegen_v4(const std::string& pattern, unsigned int max_len, unsigned int min_len)
{
    std::string result;
    do
    {
        NameGen::Generator ng(pattern);
        result = ng.toString();
    } while (result.size() < min_len || result.size() > max_len);
    return result;
}

// Generates a random NPC name, using a combination of the other systems.
std::string ProcNameGen::npc_name(Gender gender, bool with_surname)
{
    const std::string surname_str = (with_surname ? " " + surname() : "");

    // 1 in 10 chance of using a pre-existing name list.
    if (random::get<bool>(0.1f) && (gender == Gender::MALE || gender == Gender::FEMALE))
    {
        switch(gender)
        {
            case Gender::FEMALE: return name_f() + surname_str;
            case Gender::MALE: return name_m() + surname_str;
            default: break;
        }
    }

    // Otherwise, we'll use one of the name generators.
    auto sounds_feminine = [](std::string to_check)
    {
        int vowel_count = 0;
        bool ends_in_vowel = false;
        to_check[0] = std::tolower(to_check[0]);
        for (unsigned int i = 0; i < to_check.size(); i++)
        {
            const char ch = to_check[i];
            const bool last_char = (i == to_check.size() - 1);
            if (ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u' || ch == 'y') // I know Y isn't a vowel, shut up.
            {
                vowel_count++;
                if (last_char) ends_in_vowel = true;
            }
        }
        if (vowel_count >= 3 || (vowel_count >= 2 && ends_in_vowel)) return true;
        else return false;
    };

    std::string chosen_name;
    unsigned int attempts = 0;
    while (true)
    {
        if (gender == Gender::FEMALE) chosen_name = namegen_v4(v4_template, 9, 3);
        else
        {
            // The v1 and v3 name generators tend to be a bit clunky-sounding, so they're better used for masculine/neutral names
            if (random::get<bool>(0.2f)) chosen_name = namegen_v1();
            else if (random::get<bool>(0.2f)) chosen_name = random_word(true);
            else chosen_name = namegen_v4(v4_template, 8, 4);
        }

        const bool name_sounds_feminine = sounds_feminine(chosen_name);
        if (gender == Gender::FEMALE)
        {
            if (name_sounds_feminine) return chosen_name + surname_str;
        }
        else if (gender == Gender::MALE)
        {
            if (!name_sounds_feminine) return chosen_name + surname_str;
        }
        else return chosen_name + surname_str;  // Gender-neutral NPCs are fine either way.

        if (++attempts > 100)   // If we can't get anything suitable after 100 tries, just give up.
        {
            if (gender == Gender::FEMALE) return name_f() + surname_str;
            else return name_m() + surname_str;
        }
    }
}

// Ends of words.
std::string ProcNameGen::pv3_t()
{
    if (random::get<bool>()) return pv3_v[random::get<int>(0, pv3_v.size() - 1)] + pv3_f[random::get<int>(0, pv3_f.size() - 1)];
    else return pv3_v[random::get<int>(0, pv3_v.size() - 1)] + pv3_e[random::get<int>(0, pv3_e.size() - 1)] + "e";
}

// Generates a random word.
std::string ProcNameGen::random_word(bool cap)
{
    std::string gen_name;
    switch(random::get(1, 8))
    {
        case 1: case 2: gen_name = pv3_c[random::get<int>(0, pv3_c.size() - 1)] + pv3_t(); break;
        case 3: gen_name = pv3_c[random::get<int>(0, pv3_c.size() - 1)] + pv3_x[random::get<int>(0, pv3_x.size() - 1)]; break;
        case 4: gen_name = pv3_c[random::get<int>(0, pv3_c.size() - 1)] + pv3_d[random::get<int>(0, pv3_d.size() - 1)] +
            pv3_f[random::get<int>(0, pv3_f.size() - 1)]; break;
        case 5: gen_name = pv3_c[random::get<int>(0, pv3_c.size() - 1)] + pv3_v[random::get<int>(0, pv3_v.size() - 1)] +
            pv3_f[random::get<int>(0, pv3_f.size() - 1)] + pv3_t(); break;
        case 6: gen_name = pv3_i[random::get<int>(0, pv3_i.size() - 1)] + pv3_t(); break;
        case 7: gen_name = pv3_i[random::get<int>(0, pv3_i.size() - 1)] + pv3_c[random::get<int>(0, pv3_c.size() - 1)] + pv3_t(); break;
        case 8: gen_name = pv3_k[random::get<int>(0, pv3_k.size() - 1)] + pv3_v[random::get<int>(0, pv3_v.size() - 1)] +
            pv3_k[random::get<int>(0, pv3_k.size() - 1)] + pv3_v[random::get<int>(0, pv3_v.size() - 1)]; break;
    }
    if (cap) gen_name[0] = std::toupper(gen_name[0]);
    return gen_name;
}

// Generates a random surname.
std::string ProcNameGen::surname()
{
    std::string part_a = names_s_a[random::get<int>(0, names_s_a.size() - 1)], part_b;
    do
    {
        part_b = names_s_b[random::get<int>(0, names_s_b.size() - 1)];
    } while (part_a == part_b || part_a[part_a.size() - 1] == part_b[0]);
    part_a[0] = std::toupper(part_a[0]);
    if (random::get<bool>(0.333f))
    {
        part_b[0] = std::toupper(part_b[0]);
        return part_a + "-" + part_b;
    }
    else return part_a + part_b;
}

// Picks a vowel from the table, for forming atoms.
std::string ProcNameGen::vowel()
{
    const int pos = random::get<int>(0, vowel_block.size() - 1);
    return vowel_block.substr(pos, 1);
}

}   // namespace gorp
