// util/file/yaml.cpp -- Helper functions to make using RapidYAML easier.

// SPDX-FileType: SOURCE
// SPDX-FileCopyrightText: Copyright 2025 Raine Simmons <gc@gravecat.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "util/file/fileutils.hpp"
#include "util/file/yaml.hpp"

namespace gorp {

// Blank constructor.
YAML::YAML() : ref_(nullptr) { }

// Calls load_file() when constructing.
YAML::YAML(const std::string& filename, bool allow_backslash) { load_file(filename, allow_backslash); }

// Creates a new YAML object from a parent tree.
YAML::YAML(const ryml::Tree tree, ryml::ConstNodeRef new_ref) : ref_(new_ref), tree_(tree) { }

// Retrieves a value from a sequence, as a string.
std::string YAML::get(int index) const
{
    if (!is_seq()) throw std::runtime_error("Not a sequence!");
    if (index < 0 || index >= size()) throw std::runtime_error("Invalid sequence index!");
    return std::string(noderef()[index].val().str).substr(0, noderef()[index].val().len);
}

// Retrieves a child of this tree.
YAML YAML::get_child(const std::string& key) const
{
    YAML new_yaml(tree_, ref_[ryml::to_csubstr(key)]);
    return new_yaml;
}

// Retrieves all values of a sequence.
std::vector<std::string> YAML::get_seq(const std::string &key) const
{
    if (!key_exists(key)) throw GuruMeditation("Missing YAML key: " + key);
    YAML yaml = get_child(key);
    if (!yaml.is_seq()) throw GuruMeditation("Invalid YAML key (not a sequence): " + key);
    std::vector<std::string> vec(yaml.size());
    for (int i = 0; i < yaml.size(); i++)
        vec.at(i) = yaml.get(i);
    return vec;
}

// Checks if the noderef points to a valid map.
bool YAML::is_map() const { return noderef().is_map(); }

// Checks if the noderef points to a valid sequence.
bool YAML::is_seq() const { return noderef().is_seq(); }

// Checks if a given key exists.
bool YAML::key_exists(const std::string& key) const
{
    if (!is_map()) throw std::runtime_error("Not a map!");
    return noderef().has_child(ryml::to_csubstr(key));
}

// Retrieves the key values of a map.
std::vector<std::string> YAML::keys() const
{
    if (!is_map()) throw std::runtime_error("Not a map!");
    ryml::ConstNodeRef::children_view children = noderef().children();
    std::vector<std::string> vec_out;
    for (auto child : children)
        vec_out.push_back(std::string(child.key().str).substr(0, child.key().len));
    return vec_out;
}

// Retrieves the key/value pairs of a map.
std::map<std::string, std::string> YAML::keys_vals() const
{
    if (!is_map()) throw std::runtime_error("Not a map!");
    ryml::ConstNodeRef::children_view children = noderef().children();
    std::map<std::string, std::string> map_out;
    for (auto child : children)
    {
        std::string key_str = (std::string(child.key().str).substr(0, child.key().len));
        if (!child.has_val()) throw std::runtime_error("No values!");
        std::string val_str = (std::string(child.val().str).substr(0, child.val().len));
        map_out.insert(std::pair<std::string, std::string>(key_str, val_str));
    }
    return map_out;
}

// Loads a YAML file into memory and parse it.
void YAML::load_file(const std::string& filename, bool allow_backslash)
{
    if (!fileutils::file_exists(filename)) throw std::runtime_error("Invalid file: " + filename);
    std::string file_string = fileutils::file_to_string(filename.c_str());
    // If we don't care about using backslash for... whatever rapidYAML does with them, just turn them into double-backslashes so they're treated as a
    // string literal of \ instead of... I don't know, it's probably used for writing hex or octal or some shit.
    if (!allow_backslash)
    {
        for (unsigned int i = 0; i < file_string.size(); i++)
        {
            if (file_string[i] == '\\')
            {
                file_string = file_string.substr(0, i) + '\\' + file_string.substr(i);
                i++;
            }
        }
    }
    tree_ = ryml::parse_in_arena(ryml::to_csubstr(file_string));
    ref_ = tree_.rootref();
}

// Returns the noderef for the loaded tree.
ryml::ConstNodeRef YAML::noderef() const { return ref_; }

// Checks the number of children on the noderef.
int YAML::size() const { return noderef().num_children(); }

// Returns the value of a key, as a string.
std::string YAML::val(const std::string& key) const
{
    auto cskey = ryml::to_csubstr(key);
    return std::string(noderef()[cskey].val().str).substr(0, noderef()[cskey].val().len);
}

}   // namespace westgate
