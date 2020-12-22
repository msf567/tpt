#include "music/audio.h"
#include "music/music.h"
#include "music/synth/violin.h"

#include <cmath>
#include <SDL.h>
#include <SDL_audio.h>
#include <SDL2/SDL.h>
#include <iostream>
#include <algorithm> // VS is retarded and can't find this shit

float interpolate(float a, float b, float i)
{
    return (a * i + b * (1 - i)) / 2;
}

namespace NOTE
{
    SoundHandler *sound_handler = nullptr;
    // We can't assign sound_handler now because SDL audio isn't
    // initialized yet, so we only create it when it's needed (by then
    // the sim will be up and audio will be initialized)
    SoundHandler *get_sound()
    {
        if (!sound_handler)
        {
            printf("Initializing sound system...\n");
            sound_handler = new SoundHandler();
        }
        return sound_handler;
    }
} // namespace NOTE

SoundHandler::SoundHandler()
    : m_sampleFreq(44100)
{
    SDL_zero(wantSpec);
    wantSpec.freq = m_sampleFreq;
    wantSpec.format = AUDIO_U8;
    wantSpec.channels = 1;
    wantSpec.samples = 512;
    wantSpec.callback = SDLAudioCallback;
    wantSpec.userdata = this;

    m_device = SDL_OpenAudioDevice(NULL, 0, &wantSpec, &haveSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (m_device == 0)
        m_device = SDL_OpenAudioDevice(NULL, 0, &wantSpec, &haveSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (m_device == 0)
        std::cout << "Failed to open audio: " << SDL_GetError() << std::endl;
    SDL_PauseAudioDevice(m_device, 0);
}

SoundHandler::~SoundHandler()
{
    this->stop();
    SDL_CloseAudioDevice(m_device);
}

void SoundHandler::play()
{
    if (!NOTE::enabled)
        return;
    SDL_PauseAudioDevice(m_device, 0);
}

void SoundHandler::stop()
{
    this->callbacks = 0;
    SDL_PauseAudioDevice(m_device, 1);
}

void SoundHandler::add_sound(float freq, int length, InstrumentType instrument)
{
    if (!NOTE::enabled)
        return;
    if (length <= NOTE::MIN_LENGTH)
        length = NOTE::MIN_LENGTH;
    if (length >= NOTE::MAX_LENGTH)
        length = NOTE::MAX_LENGTH;

    this->play();
    for (size_t i = 0; i < frequencies.size(); i++)
        if (frequencies[i]->freq == freq && frequencies[i]->type == instrument)
        {
            frequencies[i]->end = this->callbacks + length;
            return;
        }
    frequencies.push_back(new Note(this->callbacks, this->callbacks + length, freq, instrument));
}

void SoundHandler::SDLAudioCallback(void *data, Uint8 *buffer, int length)
{
    SoundHandler *sound = reinterpret_cast<SoundHandler *>(data);
    std::fill(&buffer[0], &buffer[length], 0);

    for (int i = sound->frequencies.size() - 1; i >= 0; i--)
    {
        if (sound->frequencies[i]->end <= sound->callbacks)
        {
            delete sound->frequencies[i];
            sound->frequencies.erase(sound->frequencies.begin() + i);
        }
    }
    if (sound->frequencies.size() == 0)
    {
        sound->stop();
        return;
    }

    for (int i = 0; i < length; ++i)
    {
        float tone = 0.0f;
        for (size_t j = 0; j < std::min(NOTE::MAX_NOTES_AT_SAME_TIME, sound->frequencies.size()); j++)
        {
            Note *note = sound->frequencies[j];
            float freq = note->freq;
            float samples_per_sine = sound->m_sampleFreq / freq;

            switch (note->type)
            {
            case VIOLIN:
            {
                float scaled = note->count * freq / 440.0f;
                tone += interpolate(
                    violin_buffer[(int)scaled % violin_size],
                    violin_buffer[((int)scaled + 1) % violin_size],
                    scaled - (int)scaled);
                break;
            }
            case SAW:
                tone += note->count * 2 / samples_per_sine - floor(note->count * 2 / samples_per_sine) * 127.4;
                break;
            case SQUARE:
                tone += note->count % (int)samples_per_sine < samples_per_sine / 2 ? 127.4 : 0.0;
                break;
            case TRIANGLE:
            {
                float saw = note->count / samples_per_sine - floor(note->count / samples_per_sine);
                // Double cast is needed because VS and g++ don't agree on type for fabs
                tone += std::min((double)fabs(saw - 0.5) * 2, 1.0) * 127.4;
                break;
            }
            case SINE:
            default:
                tone += (std::sin(note->count / samples_per_sine * M_PI * 2) + 1) * 127.4 / 2;
                break;
            }
            note->count++;
        }
        if (tone > 235)
            tone = 235;
        buffer[i] = tone;
    }
    sound->callbacks++;
}
