
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#include "ClipPlayer.h"

#define AUDIO_FORMAT AUDIO_S16LSB
#define AUDIO_FREQUENCY 48000    /* Frequency of the file */
#define AUDIO_CHANNELS 2         /* 1 mono, 2 stereo, 4 quad, 6 (5.1) */
#define AUDIO_SAMPLES 4096       /* Specifies a unit of audio data to be used at a time. Must be a power of 2 */
#define AUDIO_MAX_SOUNDS 25      /* Max number of sounds that can be in the audio queue at anytime, stops too much mixing */
#define AUDIO_MUSIC_FADE_VALUE 2 /* The rate at which the volume fades when musics transition. The higher number indicates music fading faster */

/* Flags OR'd together, which specify how SDL should behave when a device cannot offer a specific feature
 * If flag is set, SDL will change the format in the actual audio file structure (as opposed to gDevice->want)
 *
 * Note: If you're having issues with Emscripten / EMCC play around with these flags
 *
 * 0                                    Allow no changes
 * SDL_AUDIO_ALLOW_FREQUENCY_CHANGE     Allow frequency changes (e.g. AUDIO_FREQUENCY is 48k, but allow files to play at 44.1k
 * SDL_AUDIO_ALLOW_FORMAT_CHANGE        Allow Format change (e.g. AUDIO_FORMAT may be S32LSB, but allow wave files of S16LSB to play)
 * SDL_AUDIO_ALLOW_CHANNELS_CHANGE      Allow any number of channels (e.g. AUDIO_CHANNELS being 2, allow actual 1)
 * SDL_AUDIO_ALLOW_ANY_CHANGE           Allow all changes above
 */
#define SDL_AUDIO_ALLOW_CHANGES SDL_AUDIO_ALLOW_ANY_CHANGE

namespace CLIP
{
    ClipPlayer *clip_player = nullptr;
    // We can't assign sound_handler now because SDL audio isn't
    // initialized yet, so we only create it when it's needed (by then
    // the sim will be up and audio will be initialized)
    ClipPlayer *get_clip_player()
    {
        if (!clip_player)
        {
            printf("Initializing clip system...\n");
            clip_player = new ClipPlayer();
        }
        return clip_player;
    }
} // namespace CLIP

ClipPlayer::ClipPlayer()
{
    printf("in clipplayer ctor\n");
    initAudio();
    sound = createAudio("kick.wav", false, SDL_MIX_MAXVOLUME / 20);
}

ClipPlayer::~ClipPlayer()
{
    endAudio();
    //free any saved audios
}
void ClipPlayer::playSound(const char *filename, int volume)
{
    playAudio(filename, NULL, 0, volume);
}

void ClipPlayer::playMusic(const char *filename, int volume)
{
    playAudio(filename, NULL, 1, volume);
}

void ClipPlayer::playSoundFromMemory(AudioClip *audio, int volume)
{
    audio = sound;
    playAudio(NULL, audio, 0, volume);
}

void ClipPlayer::playMusicFromMemory(AudioClip *audio, int volume)
{
    playAudio(NULL, audio, 1, volume);
}

void ClipPlayer::initAudio(void)
{
    AudioClip *global;
    gDevice = (PrivateAudioDevice *)calloc(1, sizeof(PrivateAudioDevice));
    //gSoundCount = 0;

    if (gDevice == NULL)
    {
        fprintf(stderr, "[%s: %d]Fatal Error: Memory c-allocation error\n", __FILE__, __LINE__);
        return;
    }

    gDevice->audioEnabled = 0;

    if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO))
    {
        fprintf(stderr, "[%s: %d]Error: SDL_INIT_AUDIO not initialized\n", __FILE__, __LINE__);
        return;
    }

    SDL_memset(&(gDevice->want), 0, sizeof(gDevice->want));

    (gDevice->want).freq = AUDIO_FREQUENCY;
    (gDevice->want).format = AUDIO_FORMAT;
    (gDevice->want).channels = AUDIO_CHANNELS;
    (gDevice->want).samples = AUDIO_SAMPLES;
    (gDevice->want).callback = audioCallback;
    (gDevice->want).userdata = calloc(1, sizeof(AudioClip));

    global = (AudioClip *)(gDevice->want).userdata;

    if (global == NULL)
    {
        fprintf(stderr, "[%s: %d]Error: Memory allocation error\n", __FILE__, __LINE__);
        return;
    }

    global->buffer = NULL;
    global->next = NULL;

    /* want.userdata = newAudio; */
    if ((gDevice->device = SDL_OpenAudioDevice(NULL, 0, &(gDevice->want), NULL, SDL_AUDIO_ALLOW_CHANGES)) == 0)
    {
        fprintf(stderr, "[%s: %d]Warning: failed to open audio device: %s\n", __FILE__, __LINE__, SDL_GetError());
    }
    else
    {
        /* Set audio device enabled global flag */
        gDevice->audioEnabled = 1;

        /* Unpause active audio stream */
        unpauseAudio();
    }
}

void ClipPlayer::endAudio(void)
{
    if (gDevice->audioEnabled)
    {
        pauseAudio();

        freeAudio((AudioClip *)(gDevice->want).userdata);

        /* Close down audio */
        SDL_CloseAudioDevice(gDevice->device);
    }

    free(gDevice);
}

void ClipPlayer::pauseAudio(void)
{
    if (gDevice->audioEnabled)
    {
        SDL_PauseAudioDevice(gDevice->device, 1);
    }
}

void ClipPlayer::unpauseAudio(void)
{
    if (gDevice->audioEnabled)
    {
        SDL_PauseAudioDevice(gDevice->device, 0);
    }
}

void ClipPlayer::freeAudio(AudioClip *audio)
{
    AudioClip *temp;

    while (audio != NULL)
    {
        if (audio->free == 1)
        {
            SDL_FreeWAV(audio->bufferTrue);
        }

        temp = audio;
        audio = audio->next;

        free(temp);
    }
}

AudioClip *ClipPlayer::createAudio(const char *filename, bool loop, int volume)
{
    printf("attempting to create audio\n");
    AudioClip *newAudio = (AudioClip *)calloc(1, sizeof(AudioClip));

    if (newAudio == NULL)
    {
        fprintf(stderr, "[%s: %d]Error: Memory allocation error\n", __FILE__, __LINE__);
        return NULL;
    }

    if (filename == NULL)
    {
        fprintf(stderr, "[%s: %d]Warning: filename NULL: %s\n", __FILE__, __LINE__, filename);
        return NULL;
    }

    newAudio->next = NULL;
    newAudio->loop = loop;
    newAudio->fade = 0;
    newAudio->free = 1;
    newAudio->volume = volume;

    if (SDL_LoadWAV(filename, &(newAudio->audio), &(newAudio->bufferTrue), &(newAudio->lengthTrue)) == NULL)
    {
        fprintf(stderr, "[%s: %d]Warning: failed to open wave file: %s error: %s\n", __FILE__, __LINE__, filename, SDL_GetError());
        free(newAudio);
        return NULL;
    }

    newAudio->buffer = newAudio->bufferTrue;
    newAudio->length = newAudio->lengthTrue;
    (newAudio->audio).callback = NULL;
    (newAudio->audio).userdata = NULL;

    printf("Created audio\n");
    return newAudio;
}

void ClipPlayer::playAudio(const char *filename, AudioClip *audio, bool loop, int volume)
{
    AudioClip *newAudio;

    /* Check if audio is enabled */
    if (!gDevice->audioEnabled)
    {
        return;
    }

    /* If sound, check if under max number of sounds allowed, else don't play */
    if (loop == 0)
    {
        /*
        if (gSoundCount >= AUDIO_MAX_SOUNDS)
        {
            return;
        }
        else
        {
            //gSoundCount++;
        }
        */
    }

    /* Load from filename or from Memory */
    if (filename != NULL)
    {
        /* Create new music sound with loop */
        newAudio = createAudio(filename, loop, volume);
    }
    else if (audio != NULL)
    {
        newAudio = (AudioClip *)malloc(sizeof(AudioClip));

        if (newAudio == NULL)
        {
            fprintf(stderr, "[%s: %d]Fatal Error: Memory allocation error\n", __FILE__, __LINE__);
            return;
        }

        memcpy(newAudio, audio, sizeof(AudioClip));

        newAudio->volume = volume;
        newAudio->loop = loop;
        newAudio->free = 0;
    }
    else
    {
        fprintf(stderr, "[%s: %d]Warning: filename and Audio parameters NULL\n", __FILE__, __LINE__);
        return;
    }

    /* Lock callback function */
    SDL_LockAudioDevice(gDevice->device);

    if (loop == 1)
    {
        addMusic((AudioClip *)(gDevice->want).userdata, newAudio);
    }
    else
    {
        addAudio((AudioClip *)(gDevice->want).userdata, newAudio);
    }

    SDL_UnlockAudioDevice(gDevice->device);
}

void ClipPlayer::addMusic(AudioClip *root, AudioClip *newAudio)
{
    uint8_t musicFound = 0;
    AudioClip *rootNext = root->next;

    /* Find any existing musics, 0, 1 or 2 and fade them out */
    while (rootNext != NULL)
    {
        /* Phase out any current music */
        if (rootNext->loop == 1 && rootNext->fade == 0)
        {
            if (musicFound)
            {
                rootNext->length = 0;
                rootNext->volume = 0;
            }

            rootNext->fade = 1;
        }
        /* Set flag to remove any queued up music in favour of new music */
        else if (rootNext->loop == 1 && rootNext->fade == 1)
        {
            musicFound = 1;
        }

        rootNext = rootNext->next;
    }

    addAudio(root, newAudio);
}

void ClipPlayer::audioCallback(void *userdata, uint8_t *stream, int len)
{
    AudioClip *audioClip = (AudioClip *)userdata;
    AudioClip *previous = audioClip;
    int tempLength;
    uint8_t music = 0;

    /* Silence the main buffer */
    SDL_memset(stream, 0, len);

    /* First one is place holder */
    audioClip = audioClip->next;

    while (audioClip != NULL)
    {
        if (audioClip->length > 0)
        {
            if (audioClip->fade == 1 && audioClip->loop == 1)
            {
                music = 1;

                if (audioClip->volume > 0)
                {
                    if (audioClip->volume - AUDIO_MUSIC_FADE_VALUE < 0)
                    {
                        audioClip->volume = 0;
                    }
                    else
                    {
                        audioClip->volume -= AUDIO_MUSIC_FADE_VALUE;
                    }
                }
                else
                {
                    audioClip->length = 0;
                }
            }

            if (music && audioClip->loop == 1 && audioClip->fade == 0)
            {
                tempLength = 0;
            }
            else
            {
                tempLength = ((uint32_t)len > audioClip->length) ? audioClip->length : (uint32_t)len;
            }

            SDL_MixAudioFormat(stream, audioClip->buffer, AUDIO_FORMAT, tempLength, audioClip->volume);

            audioClip->buffer += tempLength;
            audioClip->length -= tempLength;

            previous = audioClip;
            audioClip = audioClip->next;
        }
        else if (audioClip->loop == 1 && audioClip->fade == 0)
        {
            audioClip->buffer = audioClip->bufferTrue;
            audioClip->length = audioClip->lengthTrue;
        }
        else
        {
            previous->next = audioClip->next;

            if (audioClip->loop == 0)
            {
                //gSoundCount--;
            }

            audioClip->next = NULL;
            freeAudio(audioClip);

            audioClip = previous->next;
        }
    }
}

void ClipPlayer::addAudio(AudioClip *root, AudioClip *newAudio)
{
    if (root == NULL)
    {
        return;
    }

    while (root->next != NULL)
    {
        root = root->next;
    }

    root->next = newAudio;
}
