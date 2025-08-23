// Simple header to interface with sfxr's code.
// Copyright (c) 2025 Raine "Gravecat" Simmons. All rights reserved.

#pragma once

#include <string>
#include <vector>

#include "SFML/Audio.hpp"

namespace gorp {

struct SfxrSample
{
    int     version = 0, wave_type = 0;
    float   sound_vol = 0.5f, p_base_freq = 0.3f, p_freq_limit = 0.0f, p_freq_ramp = 0.0f, p_freq_dramp = 0.0f, p_duty = 0.0f, p_duty_ramp = 0.0f,
            p_vib_strength = 0.0f, p_vib_speed = 0.0f, p_vib_delay = 0.0f, p_env_attack = 0.0f, p_env_sustain = 0.3f, p_env_decay = 0.4f, p_env_punch = 0.0f,
            p_lpf_resonance = 0.0f, p_lpf_freq = 1.0f, p_lpf_ramp = 0.0f, p_hpf_freq = 0.0f, p_hpf_ramp = 0.0f, p_pha_offset = 0.0f, p_pha_ramp = 0.0f,
            p_repeat_speed = 0.0f, p_arp_speed = 0.0f, p_arp_mod = 0.0f;
    bool    filter_on = false;
};

class SfxrSoundStream : public sf::SoundStream
{
public:
            SfxrSoundStream();
    void    load_settings(SfxrSample new_sample);
    void    play_sample();
    void    reset();

private:
    void    ResetSample(bool restart);
    float   frnd(float range);
    bool    onGetData(Chunk& data) override;
    void    SynthSample(int length, float* buffer);
    void    onSeek(sf::Time timeOffset) override;

    SfxrSample  loaded_sample;

    float master_vol=0.05f;

    bool playing_sample;
    int phase;
    double fperiod;
    double fmaxperiod;
    double fslide;
    double fdslide;
    int period;
    float square_duty;
    float square_slide;
    int env_stage;
    int env_time;
    int env_length[3];
    float env_vol;
    float fphase;
    float fdphase;
    int iphase;
    float phaser_buffer[1024];
    int ipp;
    float noise_buffer[32];
    float fltp;
    float fltdp;
    float fltw;
    float fltw_d;
    float fltdmp;
    float fltphp;
    float flthp;
    float flthp_d;
    float vib_phase;
    float vib_speed;
    float vib_amp;
    int rep_time;
    int rep_limit;
    int arp_time;
    int arp_limit;
    double arp_mod;

    unsigned int channel_count;
    unsigned int sample_rate;

    std::vector<float> temp_samples; // Temporary buffer for float samples
};

} // namespace gorp
