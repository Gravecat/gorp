      
/*
   Copyright (c) 2007 Tomas Pettersson <drpetter@gmail.com>

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.

*/

// Heavily stripped down and somewhat modified by Raine "Gravecat" Simmons, 2024.
// Even more modified and re-written for SFML, 2025.

#include <cstdint> // Required for int16_t
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <vector>

#include "3rdparty/sfxr/sfxr.hpp"
#include "core/core.hpp"
#include "core/guru.hpp"
#include "util/file/fileutils.hpp"
#include "util/math/random.hpp"

#ifndef PI
#define PI 3.14159265f
#endif

namespace gorp {

SfxrSoundStream::SfxrSoundStream() : playing_sample(false), channel_count(1), sample_rate(44100)
{ initialize(channel_count, sample_rate, {}); } // Initialize the base class

void SfxrSoundStream::load_settings(SfxrSample new_sample) { loaded_sample = new_sample; }

void SfxrSoundStream::play_sample()
{
    ResetSample(false);
    playing_sample = true;
    play();
}

void SfxrSoundStream::ResetSample(bool restart)
{
    if(!restart)
        phase=0;
    fperiod=100.0/(loaded_sample.p_base_freq*loaded_sample.p_base_freq+0.001);
    period=(int)fperiod;
    fmaxperiod=100.0/(loaded_sample.p_freq_limit*loaded_sample.p_freq_limit+0.001);
    fslide=1.0-pow((double)loaded_sample.p_freq_ramp, 3.0)*0.01;
    fdslide=-pow((double)loaded_sample.p_freq_dramp, 3.0)*0.000001;
    square_duty=0.5f-loaded_sample.p_duty*0.5f;
    square_slide=-loaded_sample.p_duty_ramp*0.00005f;
    if(loaded_sample.p_arp_mod>=0.0f)
        arp_mod=1.0-pow((double)loaded_sample.p_arp_mod, 2.0)*0.9;
    else
        arp_mod=1.0+pow((double)loaded_sample.p_arp_mod, 2.0)*10.0;
    arp_time=0;
    arp_limit=(int)(pow(1.0f-loaded_sample.p_arp_speed, 2.0f)*20000+32);
    if(loaded_sample.p_arp_speed==1.0f)
        arp_limit=0;
    if(!restart)
    {
        // reset filter
        fltp=0.0f;
        fltdp=0.0f;
        fltw=pow(loaded_sample.p_lpf_freq, 3.0f)*0.1f;
        fltw_d=1.0f+loaded_sample.p_lpf_ramp*0.0001f;
        fltdmp=5.0f/(1.0f+pow(loaded_sample.p_lpf_resonance, 2.0f)*20.0f)*(0.01f+fltw);
        if(fltdmp>0.8f) fltdmp=0.8f;
        fltphp=0.0f;
        flthp=pow(loaded_sample.p_hpf_freq, 2.0f)*0.1f;
        flthp_d=1.0+loaded_sample.p_hpf_ramp*0.0003f;
        // reset vibrato
        vib_phase=0.0f;
        vib_speed=pow(loaded_sample.p_vib_speed, 2.0f)*0.01f;
        vib_amp=loaded_sample.p_vib_strength*0.5f;
        // reset envelope
        env_vol=0.0f;
        env_stage=0;
        env_time=0;
        env_length[0]=(int)(loaded_sample.p_env_attack*loaded_sample.p_env_attack*100000.0f);
        env_length[1]=(int)(loaded_sample.p_env_sustain*loaded_sample.p_env_sustain*100000.0f);
        env_length[2]=(int)(loaded_sample.p_env_decay*loaded_sample.p_env_decay*100000.0f);

        fphase=pow(loaded_sample.p_pha_offset, 2.0f)*1020.0f;
        if(loaded_sample.p_pha_offset<0.0f) fphase=-fphase;
        fdphase=pow(loaded_sample.p_pha_ramp, 2.0f)*1.0f;
        if(loaded_sample.p_pha_ramp<0.0f) fdphase=-fdphase;
        iphase=abs((int)fphase);
        ipp=0;
        for(int i=0;i<1024;i++)
            phaser_buffer[i]=0.0f;

        for(int i=0;i<32;i++)
            noise_buffer[i]=frnd(2.0f)-1.0f;

        rep_time=0;
        rep_limit=(int)(pow(1.0f-loaded_sample.p_repeat_speed, 2.0f)*20000+32);
        if(loaded_sample.p_repeat_speed==0.0f)
            rep_limit=0;
    }
}

float SfxrSoundStream::frnd(float range) { return random::get<float>(range); }

bool SfxrSoundStream::onGetData(Chunk& data)
{
    const std::size_t samples_to_produce = 512; // Number of samples to generate in this chunk

    // Allocate memory for the samples
    temp_samples.resize(samples_to_produce);

    // Generate the samples
    SynthSample(samples_to_produce, temp_samples.data());

    std::vector<int16_t> samples(samples_to_produce);

    // Convert float samples to int16_t
    for (std::size_t i = 0; i < samples_to_produce; ++i)
    {
        float f = temp_samples[i];
        if (f < -1.0) f = -1.0;
        if (f > 1.0) f = 1.0;
        samples[i] = static_cast<int16_t>(f * 32767);
    }

    // Prepare the chunk to return
    data.samples = samples.data();
    data.sampleCount = samples_to_produce;

    return playing_sample; // Return true to continue streaming, false to stop
}

void SfxrSoundStream::SynthSample(int length, float* buffer)
{
    for(int i=0;i<length;i++)
    {
        if(!playing_sample)
        {
            buffer[i] = 0.0f;
            continue;
        }


        rep_time++;
        if(rep_limit!=0 && rep_time>=rep_limit)
        {
            rep_time=0;
            ResetSample(true);
        }

        // frequency envelopes/arpeggios
        arp_time++;
        if(arp_limit!=0 && arp_time>=arp_limit)
        {
            arp_limit=0;
            fperiod*=arp_mod;
        }
        fslide+=fdslide;
        fperiod*=fslide;
        if(fperiod>fmaxperiod)
        {
            fperiod=fmaxperiod;
            if(loaded_sample.p_freq_limit>0.0f)
                playing_sample=false;
        }
        float rfperiod=fperiod;
        if(vib_amp>0.0f)
        {
            vib_phase+=vib_speed;
            rfperiod=fperiod*(1.0+sin(vib_phase)*vib_amp);
        }
        period=(int)rfperiod;
        if(period<8) period=8;
        square_duty+=square_slide;
        if(square_duty<0.0f) square_duty=0.0f;
        if(square_duty>0.5f) square_duty=0.5f;
        // volume envelope
        env_time++;
        if(env_time>env_length[env_stage])
        {
            env_time=0;
            env_stage++;
            if(env_stage==3)
                playing_sample=false;
        }
        if(env_stage==0)
            env_vol=(float)env_time/env_length[0];
        if(env_stage==1)
            env_vol=1.0f+pow(1.0f-(float)env_time/env_length[1], 1.0f)*2.0f*loaded_sample.p_env_punch;
        if(env_stage==2)
            env_vol=1.0f-(float)env_time/env_length[2];

        // phaser step
        fphase+=fdphase;
        iphase=abs((int)fphase);
        if(iphase>1023) iphase=1023;

        if(flthp_d!=0.0f)
        {
            flthp*=flthp_d;
            if(flthp<0.00001f) flthp=0.00001f;
            if(flthp>0.1f) flthp=0.1f;
        }

        float ssample=0.0f;
        for(int si=0;si<8;si++) // 8x supersampling
        {
            float sample=0.0f;
            phase++;
            if(phase>=period)
            {
//				phase=0;
                phase%=period;
                if(loaded_sample.wave_type==3)
                    for(int i=0;i<32;i++)
                        noise_buffer[i]=frnd(2.0f)-1.0f;
            }
            // base waveform
            float fp=(float)phase/period;
            switch(loaded_sample.wave_type)
            {
            case 0: // square
                if(fp<square_duty)
                    sample=0.5f;
                else
                    sample=-0.5f;
                break;
            case 1: // sawtooth
                sample=1.0f-fp*2;
                break;
            case 2: // sine
                sample=(float)sin(fp*2*PI);
                break;
            case 3: // noise
                sample=noise_buffer[phase*32/period];
                break;
            }
            // lp filter
            float pp=fltp;
            fltw*=fltw_d;
            if(fltw<0.0f) fltw=0.0f;
            if(fltw>0.1f) fltw=0.1f;
            if(loaded_sample.p_lpf_freq!=1.0f)
            {
                fltdp+=(sample-fltp)*fltw;
                fltdp-=fltdp*fltdmp;
            }
            else
            {
                fltp=sample;
                fltdp=0.0f;
            }
            fltp+=fltdp;
            // hp filter
            fltphp+=fltp-pp;
            fltphp-=fltphp*flthp;
            sample=fltphp;
            // phaser
            phaser_buffer[ipp&1023]=sample;
            sample+=phaser_buffer[(ipp-iphase+1024)&1023];
            ipp=(ipp+1)&1023;
            // final accumulation and envelope application
            ssample+=sample*env_vol;
        }
        ssample=ssample/8*master_vol;

        ssample*=2.0f*loaded_sample.sound_vol;

        if(buffer!=NULL)
        {
            if(ssample>1.0f) ssample=1.0f;
            if(ssample<-1.0f) ssample=-1.0f;
            buffer[i]=ssample;
        }
    }
}

void SfxrSoundStream::onSeek(sf::Time timeOffset)
{
    // Seeking is not supported in this simple implementation.
    // This could be implemented by tracking the number of samples generated and
    // resetting the synthesis state to the appropriate point in time.
    (void)timeOffset;
    ResetSample(false);
}

void SfxrSoundStream::reset()
{
    loaded_sample.wave_type=0;

    loaded_sample.p_base_freq=0.3f;
    loaded_sample.p_freq_limit=0.0f;
    loaded_sample.p_freq_ramp=0.0f;
    loaded_sample.p_freq_dramp=0.0f;
    loaded_sample.p_duty=0.0f;
    loaded_sample.p_duty_ramp=0.0f;

    loaded_sample.p_vib_strength=0.0f;
    loaded_sample.p_vib_speed=0.0f;
    loaded_sample.p_vib_delay=0.0f;

    loaded_sample.p_env_attack=0.0f;
    loaded_sample.p_env_sustain=0.3f;
    loaded_sample.p_env_decay=0.4f;
    loaded_sample.p_env_punch=0.0f;

    loaded_sample.filter_on=false;
    loaded_sample.p_lpf_resonance=0.0f;
    loaded_sample.p_lpf_freq=1.0f;
    loaded_sample.p_lpf_ramp=0.0f;
    loaded_sample.p_hpf_freq=0.0f;
    loaded_sample.p_hpf_ramp=0.0f;

    loaded_sample.p_pha_offset=0.0f;
    loaded_sample.p_pha_ramp=0.0f;

    loaded_sample.p_repeat_speed=0.0f;

    loaded_sample.p_arp_speed=0.0f;
    loaded_sample.p_arp_mod=0.0f;
}

} // namespace gorp
