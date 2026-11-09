// Files, bitmaps and sounds for the menu, looked up by name in a list of
// directories. The mod directory comes first, so a file there with the same
// name (any case) replaces the game's original.
#pragma once

#include <SDL3/SDL.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace davi {

std::vector<uint8_t> ReadFile(const std::string& path);
// Case-insensitive lookup of `rel` ("MenuData/menu.ini", '/' or '\\') below dir.
std::string FindNoCase(const std::string& dir, const std::string& rel);

// A BMP (file bytes) as an XRGB8888 surface, or nullptr.
SDL_Surface* LoadBmp(const std::vector<uint8_t>& bytes);
// A Targa (types 2 and 10, 24 or 32 bit) as an ARGB8888 surface with
// blending on (32 bit: the TGA's own alpha), or nullptr.
SDL_Surface* LoadTga(const std::vector<uint8_t>& bytes);
// BMP or TGA, by the file's first bytes.
SDL_Surface* LoadPicture(const std::vector<uint8_t>& bytes);

struct Sound {
  SDL_AudioSpec spec{};
  std::vector<uint8_t> pcm;
  bool Ok() const { return !pcm.empty(); }
};
Sound LoadWav(const std::vector<uint8_t>& bytes);

class Assets {
 public:
  // Directories searched in order of priority (first = wins).
  void AddDir(const std::string& dir) { dirs_.push_back(dir); }
  // A RED-engine resource archive (<name>.IND + <name>.IMG, e.g. MenuRcs):
  // searched after all directories, so a loose file (mod) wins.
  bool AddArchive(const std::string& ind_path);
  std::string Find(const std::string& name) const;  // "" if not found
  // File bytes from a directory or an archive (empty if not found).
  std::vector<uint8_t> Read(const std::string& name) const;
  bool Has(const std::string& name) const;
  // "MAINSCRE" or "mainscre.bmp"; without extension .bmp, then .tga.
  // Cached, owned by Assets.
  SDL_Surface* Bitmap(const std::string& name);
  const Sound* Wav(const std::string& name);
  const std::vector<std::string>& Dirs() const { return dirs_; }

 private:
  struct Entry {
    size_t archive;
    uint32_t offset, size;
  };
  std::vector<std::string> dirs_;
  std::vector<std::string> archive_imgs_;
  std::map<std::string, Entry> entries_;  // lower-case name
  std::map<std::string, SDL_Surface*> bmps_;
  std::map<std::string, std::unique_ptr<Sound>> wavs_;
};

// Simple mixer on SDL3 audio streams: fire-and-forget sounds, one voice
// channel (a new voice line stops the previous one) and looping music.
class Audio {
 public:
  bool Init();
  void Shutdown();
  void Play(const Sound* s, float gain = 1.0f);
  void PlayVoice(const Sound* s, float gain = 1.0f);
  bool VoicePlaying() const;
  void StopVoice();
  void PlayMusic(const Sound* s, float gain);
  void SetMusicGain(float gain);
  void StopMusic();
  void Pause(bool paused);  // while the race runs
  // A raw stream on the device (owned by the caller), e.g. for video sound.
  SDL_AudioStream* OpenStream(const SDL_AudioSpec& spec, float gain);
  void Update();            // refills the music loop, frees finished sounds

 private:
  SDL_AudioDeviceID dev_ = 0;
  std::vector<SDL_AudioStream*> sfx_;
  SDL_AudioStream* voice_ = nullptr;
  SDL_AudioStream* music_ = nullptr;
  const Sound* music_snd_ = nullptr;
};

}  // namespace davi
