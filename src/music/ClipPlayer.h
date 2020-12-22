#ifndef SIMPLE_AUDIO_
#define SIMPLE_AUDIO_

#include <SDL2/SDL.h>
typedef struct sound
{
  uint32_t length;
  uint32_t lengthTrue;
  uint8_t *bufferTrue;
  uint8_t *buffer;
  uint8_t loop;
  uint8_t fade;
  uint8_t free;
  uint8_t volume;

  SDL_AudioSpec audio;

  struct sound *next;
} AudioClip;

typedef struct privateAudioDevice
{
  SDL_AudioDeviceID device;
  SDL_AudioSpec want;
  uint8_t audioEnabled;
} PrivateAudioDevice;

class ClipPlayer;
namespace CLIP
{
  extern ClipPlayer *clip_player;
  ClipPlayer *get_clip_player();
} // namespace CLIP

class ClipPlayer
{
public:
  ClipPlayer();
  ~ClipPlayer();
  AudioClip *sound;
  AudioClip *createAudio(const char *filename, bool loop, int volume);
  static void freeAudio(AudioClip *audio);
  void playSound(const char *filename, int volume);
  void playMusic(const char *filename, int volume);
  void playSoundFromMemory(AudioClip *audio, int volume);
  void playMusicFromMemory(AudioClip *audio, int volume);
  void endAudio(void);
  void initAudio(void);
  void pauseAudio(void);
  void unpauseAudio(void);
  void addMusic(AudioClip *root, AudioClip *newAudio);
  void playAudio(const char *filename, AudioClip *audio, bool loop, int volume);
  void addAudio(AudioClip *root, AudioClip *newAudio);
  static void audioCallback(void *userdata, uint8_t *stream, int len);

private:
  PrivateAudioDevice *gDevice;
  //static int gSoundCount;
};

#endif
