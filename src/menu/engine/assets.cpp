#include "engine/assets.h"

#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>

#include "engine/dfm.h"  // Lower

namespace davi {

std::vector<uint8_t> ReadFile(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return {};
  return std::vector<uint8_t>(std::istreambuf_iterator<char>(f), {});
}

std::string FindNoCase(const std::string& dir, const std::string& rel) {
  std::string cur = dir;
  size_t i = 0;
  while (i <= rel.size()) {
    size_t j = rel.find_first_of("/\\", i);
    std::string part = rel.substr(i, j == std::string::npos ? std::string::npos : j - i);
    i = j == std::string::npos ? rel.size() + 1 : j + 1;
    if (part.empty()) continue;
    std::string next = cur + "/" + part;
    struct stat st;
    if (stat(next.c_str(), &st) != 0) {
      next.clear();
      if (DIR* d = opendir(cur.c_str())) {
        std::string lw = Lower(part);
        while (dirent* e = readdir(d))
          if (Lower(e->d_name) == lw) {
            next = cur + "/" + e->d_name;
            break;
          }
        closedir(d);
      }
      if (next.empty()) return "";
    }
    cur = next;
  }
  struct stat st;
  return stat(cur.c_str(), &st) == 0 ? cur : "";
}

SDL_Surface* LoadBmp(const std::vector<uint8_t>& bytes) {
  if (bytes.empty()) return nullptr;
  SDL_IOStream* io = SDL_IOFromConstMem(bytes.data(), bytes.size());
  SDL_Surface* s = SDL_LoadBMP_IO(io, true);
  if (!s) return nullptr;
  SDL_Surface* c = SDL_ConvertSurface(s, SDL_PIXELFORMAT_XRGB8888);
  SDL_DestroySurface(s);
  return c;
}

SDL_Surface* LoadTga(const std::vector<uint8_t>& b) {
  if (b.size() < 18) return nullptr;
  int idlen = b[0], cmap = b[1], type = b[2];
  int w = b[12] | b[13] << 8, h = b[14] | b[15] << 8, bpp = b[16], desc = b[17];
  if (cmap || (type != 2 && type != 10) || (bpp != 24 && bpp != 32) || w <= 0 || h <= 0) return nullptr;
  int px = bpp / 8;
  std::vector<uint8_t> raw(size_t(w) * h * px);
  size_t p = 18 + idlen, o = 0;
  if (type == 2) {
    if (p + raw.size() > b.size()) return nullptr;
    std::memcpy(raw.data(), b.data() + p, raw.size());
  } else {
    while (o < raw.size() && p < b.size()) {
      int hdr = b[p++], n = (hdr & 0x7f) + 1;
      if (hdr & 0x80) {
        if (p + px > b.size()) break;
        for (int i = 0; i < n && o < raw.size(); i++, o += px) std::memcpy(&raw[o], &b[p], px);
        p += px;
      } else {
        size_t len = std::min(size_t(n) * px, raw.size() - o);
        if (p + len > b.size()) break;
        std::memcpy(&raw[o], &b[p], len);
        o += len;
        p += size_t(n) * px;
      }
    }
  }
  SDL_Surface* s = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_ARGB8888);
  if (!s) return nullptr;
  bool top = desc & 0x20;
  for (int y = 0; y < h; y++) {
    const uint8_t* src = &raw[size_t(top ? y : h - 1 - y) * w * px];
    uint32_t* dst = (uint32_t*)((uint8_t*)s->pixels + y * s->pitch);
    for (int x = 0; x < w; x++, src += px)
      dst[x] = uint32_t(px == 4 ? src[3] : 255) << 24 | src[2] << 16 | src[1] << 8 | src[0];
  }
  SDL_SetSurfaceBlendMode(s, px == 4 ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
  return s;
}

SDL_Surface* LoadPicture(const std::vector<uint8_t>& b) {
  if (b.size() >= 2 && b[0] == 'B' && b[1] == 'M') return LoadBmp(b);
  return LoadTga(b);
}

Sound LoadWav(const std::vector<uint8_t>& bytes) {
  Sound snd;
  if (bytes.empty()) return snd;
  SDL_IOStream* io = SDL_IOFromConstMem(bytes.data(), bytes.size());
  Uint8* buf = nullptr;
  Uint32 len = 0;
  if (SDL_LoadWAV_IO(io, true, &snd.spec, &buf, &len)) {
    snd.pcm.assign(buf, buf + len);
    SDL_free(buf);
  }
  return snd;
}

bool Assets::AddArchive(const std::string& ind_path) {
  std::vector<uint8_t> ind = ReadFile(ind_path);
  std::string img = ind_path.substr(0, ind_path.size() - 3) + "IMG";
  std::string found = FindNoCase(img.substr(0, img.find_last_of('/')), img.substr(img.find_last_of('/') + 1));
  if (ind.size() < 2 || found.empty()) return false;
  struct stat st;
  if (stat(found.c_str(), &st) != 0) return false;
  size_t n = ind[0] | ind[1] << 8;
  if (ind.size() < 2 + 24 * n) return false;
  std::vector<std::pair<uint32_t, std::string>> v;
  for (size_t i = 0; i < n; i++) {
    const uint8_t* e = &ind[2 + 24 * i];
    std::string name((const char*)e, strnlen((const char*)e, 20));
    uint32_t off = e[20] | e[21] << 8 | e[22] << 16 | uint32_t(e[23]) << 24;
    v.push_back({off, Lower(name)});
  }
  // Item size = distance to the next item (raw items, no header).
  std::vector<uint32_t> offs;
  for (auto& [o, _] : v) offs.push_back(o);
  offs.push_back(uint32_t(st.st_size));
  std::sort(offs.begin(), offs.end());
  size_t a = archive_imgs_.size();
  archive_imgs_.push_back(found);
  for (auto& [o, name] : v) {
    if (name.empty() || name[0] == '*') continue;  // "*empty resitem*"
    uint32_t next = *std::upper_bound(offs.begin(), offs.end(), o);
    if (!entries_.count(name)) entries_[name] = {a, o, next - o};
  }
  return true;
}

bool Assets::Has(const std::string& name) const {
  return !Find(name).empty() || entries_.count(Lower(name.substr(name.find_last_of("/\\") + 1)));
}

std::vector<uint8_t> Assets::Read(const std::string& name) const {
  std::string p = Find(name);
  if (!p.empty()) return ReadFile(p);
  auto it = entries_.find(Lower(name.substr(name.find_last_of("/\\") + 1)));
  if (it == entries_.end()) return {};
  std::ifstream f(archive_imgs_[it->second.archive], std::ios::binary);
  std::vector<uint8_t> out(it->second.size);
  f.seekg(it->second.offset);
  f.read((char*)out.data(), out.size());
  if (!f) return {};
  return out;
}

std::string Assets::Find(const std::string& name) const {
  for (auto& d : dirs_) {
    std::string p = FindNoCase(d, name);
    if (!p.empty()) return p;
  }
  return "";
}

SDL_Surface* Assets::Bitmap(const std::string& name) {
  std::string key = Lower(name);
  auto it = bmps_.find(key);
  if (it != bmps_.end()) return it->second;
  SDL_Surface* s = nullptr;
  if (name.find('.') == std::string::npos) {
    for (const char* ext : {".bmp", ".tga"})
      if (!s) s = LoadPicture(Read(name + ext));
  } else {
    s = LoadPicture(Read(name));
  }
  if (!s) SDL_Log("menu: picture not found: %s", name.c_str());
  bmps_[key] = s;
  return s;
}

const Sound* Assets::Wav(const std::string& name) {
  std::string n = name;
  if (n.find('.') == std::string::npos) n += ".wav";
  std::string key = Lower(n);
  auto it = wavs_.find(key);
  if (it != wavs_.end()) return it->second.get();
  auto s = std::make_unique<Sound>();
  *s = LoadWav(Read(n));
  if (!s->Ok()) {
    SDL_Log("menu: sound not found: %s", n.c_str());
    s.reset();
  }
  return (wavs_[key] = std::move(s)).get();
}

bool Audio::Init() {
  if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) return false;
  dev_ = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
  return dev_ != 0;
}

void Audio::Shutdown() {
  for (auto* s : sfx_) SDL_DestroyAudioStream(s);
  sfx_.clear();
  StopVoice();
  StopMusic();
  if (dev_) SDL_CloseAudioDevice(dev_);
  dev_ = 0;
}

static SDL_AudioStream* Start(SDL_AudioDeviceID dev, const Sound* s, float gain) {
  if (!dev || !s || !s->Ok()) return nullptr;
  SDL_AudioStream* st = SDL_CreateAudioStream(&s->spec, nullptr);
  if (!st) return nullptr;
  SDL_SetAudioStreamGain(st, gain);
  SDL_PutAudioStreamData(st, s->pcm.data(), int(s->pcm.size()));
  SDL_FlushAudioStream(st);
  SDL_BindAudioStream(dev, st);
  return st;
}

void Audio::Play(const Sound* s, float gain) {
  if (SDL_AudioStream* st = Start(dev_, s, gain)) sfx_.push_back(st);
}

void Audio::PlayVoice(const Sound* s, float gain) {
  StopVoice();
  voice_ = Start(dev_, s, gain);
}

bool Audio::VoicePlaying() const { return voice_ && SDL_GetAudioStreamQueued(voice_) > 0; }

void Audio::StopVoice() {
  if (voice_) SDL_DestroyAudioStream(voice_);
  voice_ = nullptr;
}

void Audio::PlayMusic(const Sound* s, float gain) {
  if (music_snd_ == s && music_) {
    SetMusicGain(gain);
    return;
  }
  StopMusic();
  music_snd_ = s;
  music_ = Start(dev_, s, gain);
}

void Audio::SetMusicGain(float gain) {
  if (music_) SDL_SetAudioStreamGain(music_, gain);
}

void Audio::StopMusic() {
  if (music_) SDL_DestroyAudioStream(music_);
  music_ = nullptr;
  music_snd_ = nullptr;
}

SDL_AudioStream* Audio::OpenStream(const SDL_AudioSpec& spec, float gain) {
  if (!dev_) return nullptr;
  SDL_AudioStream* st = SDL_CreateAudioStream(&spec, nullptr);
  if (!st) return nullptr;
  SDL_SetAudioStreamGain(st, gain);
  SDL_BindAudioStream(dev_, st);
  return st;
}

void Audio::Pause(bool paused) {
  if (!dev_) return;
  if (paused) SDL_PauseAudioDevice(dev_);
  else SDL_ResumeAudioDevice(dev_);
}

void Audio::Update() {
  if (music_ && music_snd_ && SDL_GetAudioStreamQueued(music_) < int(music_snd_->pcm.size() / 4)) {
    SDL_PutAudioStreamData(music_, music_snd_->pcm.data(), int(music_snd_->pcm.size()));
    SDL_FlushAudioStream(music_);
  }
  for (size_t i = 0; i < sfx_.size();) {
    if (SDL_GetAudioStreamQueued(sfx_[i]) == 0) {
      SDL_DestroyAudioStream(sfx_[i]);
      sfx_.erase(sfx_.begin() + i);
    } else {
      i++;
    }
  }
}

}  // namespace davi
