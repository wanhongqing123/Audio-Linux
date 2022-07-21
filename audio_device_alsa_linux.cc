#include "audio_device_alsa_linux.h"

LiteavAlsaSymbolTables* GetAlsaSymbolTable() {
    static LiteavAlsaSymbolTables* p = new LiteavAlsaSymbolTables();
    return p;
}

static const unsigned int ALSA_PLAYOUT_FREQ = 48000;
static const unsigned int ALSA_PLAYOUT_CH = 2;
static const unsigned int ALSA_PLAYOUT_LATENCY = 40 * 1000;  // in us
static const unsigned int ALSA_CAPTURE_FREQ = 48000;
static const unsigned int ALSA_CAPTURE_CH = 2;
static const unsigned int ALSA_CAPTURE_LATENCY = 40 * 1000;  // in us
static const unsigned int ALSA_CAPTURE_WAIT_TIMEOUT = 5;     // in ms

#define FUNC_GET_NUM_OF_DEVICE 0
#define FUNC_GET_DEVICE_NAME 1
#define FUNC_GET_DEVICE_NAME_FOR_AN_ENUM 2

AudioDeviceAlsaLinux::AudioDeviceAlsaLinux() = default;
AudioDeviceAlsaLinux::~AudioDeviceAlsaLinux() = default;
int32_t AudioDeviceAlsaLinux::GetDevicesInfo(const int32_t function,
                                             const bool playback,
                                             const int32_t enumDeviceNo,
                                             char* enumDeviceName,
                                             const int32_t ednLen) {

// Device enumeration based on libjingle implementation
  // by Tristan Schmelcher at Google Inc.

  const char* type = playback ? "Output" : "Input";
  // dmix and dsnoop are only for playback and capture, respectively, but ALSA
  // stupidly includes them in both lists.
  const char* ignorePrefix = playback ? "dsnoop:" : "dmix:";
  // (ALSA lists many more "devices" of questionable interest, but we show them
  // just in case the weird devices may actually be desirable for some
  // users/systems.)
  int err;
  int enumCount(0);
  bool keepSearching(true);

  // From Chromium issue 95797
  // Loop through the sound cards to get Alsa device hints.
  // Don't use snd_device_name_hint(-1,..) since there is a access violation
  // inside this ALSA API with libasound.so.2.0.0.
  int card = -1;
  while (!(LATE(snd_card_next)(&card)) && (card >= 0) && keepSearching) {
    void** hints;
    err = LATE(snd_device_name_hint)(card, "pcm", &hints);
    if (err != 0) {
      LATE(snd_strerror)(err);
      return -1;
    }

    enumCount++;  // default is 0
    if ((function == FUNC_GET_DEVICE_NAME ||
         function == FUNC_GET_DEVICE_NAME_FOR_AN_ENUM) &&
        enumDeviceNo == 0) {
      strcpy(enumDeviceName, "default");

      err = LATE(snd_device_name_free_hint)(hints);
      if (err != 0) {
        LATE(snd_strerror)(err);
      }

      return 0;
    }

    for (void** list = hints; *list != NULL; ++list) {
      char* actualType = LATE(snd_device_name_get_hint)(*list, "IOID");
      if (actualType) {  // NULL means it's both.
        bool wrongType = (strcmp(actualType, type) != 0);
        free(actualType);
        if (wrongType) {
          // Wrong type of device (i.e., input vs. output).
          continue;
        }
      }

      char* name = LATE(snd_device_name_get_hint)(*list, "NAME");
      if (!name) {
        continue;
      }

      // Now check if we actually want to show this device.
      if (strcmp(name, "default") != 0 && strcmp(name, "null") != 0 &&
          strcmp(name, "pulse") != 0 &&
          strncmp(name, ignorePrefix, strlen(ignorePrefix)) != 0) {
        // Yes, we do.
        char* desc = LATE(snd_device_name_get_hint)(*list, "DESC");
        if (!desc) {
          // Virtual devices don't necessarily have descriptions.
          // Use their names instead.
          desc = name;
        }

        if (FUNC_GET_NUM_OF_DEVICE == function) {
         
        }
        if ((FUNC_GET_DEVICE_NAME == function) && (enumDeviceNo == enumCount)) {
          // We have found the enum device, copy the name to buffer.
          strncpy(enumDeviceName, desc, ednLen);
          enumDeviceName[ednLen - 1] = '\0';
          keepSearching = false;
          // Replace '\n' with '-'.
          char* pret = strchr(enumDeviceName, '\n' /*0xa*/);  // LF
          if (pret)
            *pret = '-';
        }
        if ((FUNC_GET_DEVICE_NAME_FOR_AN_ENUM == function) &&
            (enumDeviceNo == enumCount)) {
          // We have found the enum device, copy the name to buffer.
          strncpy(enumDeviceName, name, ednLen);
          enumDeviceName[ednLen - 1] = '\0';
          keepSearching = false;
        }

        if (keepSearching)
          ++enumCount;

        if (desc != name)
          free(desc);
      }

      free(name);

      if (!keepSearching)
        break;
    }

    err = LATE(snd_device_name_free_hint)(hints);
    if (err != 0) {
      LATE(snd_strerror)(err);
      // Continue and return true anyway, since we did get the whole list.
    }
  }

  if (FUNC_GET_NUM_OF_DEVICE == function) {
    if (enumCount == 1)  // only default?
      enumCount = 0;
    return enumCount;  // Normal return point for function 0
  }

  if (keepSearching) {
    return -1;
  }

  return 0;
}