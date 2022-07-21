#pragma once
#include <alsa/asoundlib.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include "audio_mixer_manager_alsa_linux.h"

typedef AlsaSymbolTable LiteavAlsaSymbolTables;
LiteavAlsaSymbolTables* GetAlsaSymbolTable();

class AudioDeviceAlsaLinux {
 public:
  AudioDeviceAlsaLinux();
  ~AudioDeviceAlsaLinux();
  int32_t GetDevicesInfo(const int32_t function,
                                             const bool playback,
                                             const int32_t enumDeviceNo,
                                             char* enumDeviceName,
                                             const int32_t ednLen);
};