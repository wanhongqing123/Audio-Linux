/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "audio_mixer_manager_alsa_linux.h"

// Accesses ALSA functions through our late-binding symbol table instead of
// directly. This way we don't have to link to libasound, which means our binary
// will work on systems that don't have it.


typedef AlsaSymbolTable LiteavAlsaSymbolTables;
#define AdmMaxDeviceNameSize 128
LiteavAlsaSymbolTables* GetAlsaSymbolTable() {
    static LiteavAlsaSymbolTables* p = new LiteavAlsaSymbolTables();
    return p;
}


#define LATE(sym)                                                            \
  LATESYM_GET(AlsaSymbolTable, GetAlsaSymbolTable(), \
              sym)

AudioMixerManagerLinuxALSA::AudioMixerManagerLinuxALSA()
    : outputMixerHandle_(nullptr),
      inputMixerHandle_(nullptr),
      outputMixerElement_(nullptr),
      inputMixerElement_(nullptr) {

  memset(outputMixerStr_, 0, AdmMaxDeviceNameSize);
  memset(inputMixerStr_, 0, AdmMaxDeviceNameSize);
}

AudioMixerManagerLinuxALSA::~AudioMixerManagerLinuxALSA() {
  Close();
}

// ============================================================================
//                                    PUBLIC METHODS
// ============================================================================

int32_t AudioMixerManagerLinuxALSA::Close() {

  std::lock_guard<std::mutex> lock(mutex_);

  CloseSpeakerLocked();
  CloseMicrophoneLocked();

  return 0;
}

int32_t AudioMixerManagerLinuxALSA::CloseSpeaker() {
  std::lock_guard<std::mutex> lock(mutex_);
  return CloseSpeakerLocked();
}

int32_t AudioMixerManagerLinuxALSA::CloseSpeakerLocked() {

  int errVal = 0;

  if (outputMixerHandle_ != nullptr) {
    LATE(snd_mixer_free)(outputMixerHandle_);
    if (errVal < 0) {
      LATE(snd_strerror)(errVal);
    }
    errVal = LATE(snd_mixer_detach)(outputMixerHandle_, outputMixerStr_);
    if (errVal < 0) {
      LATE(snd_strerror)(errVal);
    }
    errVal = LATE(snd_mixer_close)(outputMixerHandle_);
    if (errVal < 0) {

    }
    outputMixerHandle_ = nullptr;
    outputMixerElement_= nullptr;
  }
  memset(outputMixerStr_, 0, AdmMaxDeviceNameSize);

  return 0;
}

int32_t AudioMixerManagerLinuxALSA::CloseMicrophone() {
  std::lock_guard<std::mutex> lock(mutex_);
  return CloseMicrophoneLocked();
}

int32_t AudioMixerManagerLinuxALSA::CloseMicrophoneLocked() {
  int errVal = 0;

  if (inputMixerHandle_ != nullptr) {

    LATE(snd_mixer_free)(inputMixerHandle_);
    if (errVal < 0) {
      LATE(snd_strerror)(errVal);
    }

    errVal = LATE(snd_mixer_detach)(inputMixerHandle_, inputMixerStr_);
    if (errVal < 0) {
       LATE(snd_strerror)(errVal);
    }

    errVal = LATE(snd_mixer_close)(inputMixerHandle_);
    if (errVal < 0) {
    }

    inputMixerHandle_ = nullptr;
    inputMixerElement_ = nullptr;
  }
  memset(inputMixerStr_, 0, 128);

  return 0;
}

int32_t AudioMixerManagerLinuxALSA::OpenSpeaker(char* deviceName) {
  std::lock_guard<std::mutex> lock(mutex_);

  int errVal = 0;
  if (outputMixerHandle_ != nullptr) {
    LATE(snd_mixer_free)(outputMixerHandle_);
    if (errVal < 0) {
      LATE(snd_strerror)(errVal);
    }
    errVal = LATE(snd_mixer_detach)(outputMixerHandle_, outputMixerStr_);
    if (errVal < 0) {
      LATE(snd_strerror)(errVal);
    }
    errVal = LATE(snd_mixer_close)(outputMixerHandle_);
    if (errVal < 0) {
    }
  }
  outputMixerHandle_ = nullptr;
  outputMixerElement_ = nullptr;

  errVal = LATE(snd_mixer_open)(&outputMixerHandle_, 0);
  if (errVal < 0) {
    return -1;
  }

  char controlName[AdmMaxDeviceNameSize] = {0};
  GetControlName(controlName, deviceName);

  errVal = LATE(snd_mixer_attach)(outputMixerHandle_, controlName);
  if (errVal < 0) {
    LATE(snd_strerror)(errVal);
    outputMixerHandle_ = nullptr;
    return -1;
  }
  strcpy(outputMixerStr_, controlName);

  errVal = LATE(snd_mixer_selem_register)(outputMixerHandle_, nullptr, nullptr);
  if (errVal < 0) {
    LATE(snd_strerror)(errVal);
    outputMixerHandle_ = nullptr;
    return -1;
  }

  // Load and find the proper mixer element
  if (LoadSpeakerMixerElement() < 0) {
    return -1;
  }

  if (outputMixerHandle_ != nullptr) {
  }

  return 0;
}

int32_t AudioMixerManagerLinuxALSA::OpenMicrophone(char* deviceName) {

  std::lock_guard<std::mutex> lock(mutex_);

  int errVal = 0;

  if (inputMixerHandle_ != nullptr) {
    LATE(snd_mixer_free)(inputMixerHandle_);
    if (errVal < 0) {
      LATE(snd_strerror)(errVal);
    }

    errVal = LATE(snd_mixer_detach)(inputMixerHandle_, inputMixerStr_);
    if (errVal < 0) {
      LATE(snd_strerror)(errVal);
    }

    errVal = LATE(snd_mixer_close)(inputMixerHandle_);
    if (errVal < 0) {

    }
  }
  inputMixerHandle_ = nullptr;
  inputMixerElement_ = nullptr;

  errVal = LATE(snd_mixer_open)(&inputMixerHandle_, 0);
  if (errVal < 0) {
    return -1;
  }

  char controlName[AdmMaxDeviceNameSize] = {0};
  GetControlName(controlName, deviceName);


  errVal = LATE(snd_mixer_attach)(inputMixerHandle_, controlName);
  if (errVal < 0) {
    LATE(snd_strerror)(errVal);
    inputMixerHandle_ = nullptr;
    return -1;
  }
  strcpy(inputMixerStr_, controlName);

  errVal = LATE(snd_mixer_selem_register)(inputMixerHandle_, nullptr, nullptr);
  if (errVal < 0) {
    LATE(snd_strerror)(errVal);
    inputMixerHandle_ = nullptr;
    return -1;
  }
  if (LoadMicMixerElement() < 0) {
    return -1;
  }

  if (inputMixerHandle_ != nullptr) {
  }

  return 0;
}

bool AudioMixerManagerLinuxALSA::SpeakerIsInitialized() const {
  return (outputMixerHandle_ != nullptr);
}

bool AudioMixerManagerLinuxALSA::MicrophoneIsInitialized() const {
  return (inputMixerHandle_ != nullptr);
}

int32_t AudioMixerManagerLinuxALSA::SetSpeakerVolume(uint32_t volume) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (outputMixerElement_ == nullptr) {
    return -1;
  }

  int errVal = LATE(snd_mixer_selem_set_playback_volume_all)(
      outputMixerElement_, volume);
  if (errVal < 0) {
    LATE(snd_strerror)(errVal);
    return -1;
  }

  return (0);
}

int32_t AudioMixerManagerLinuxALSA::SpeakerVolume(uint32_t& volume) const {
  if (outputMixerElement_ == nullptr) {
    return -1;
  }

  long int vol(0);

  int errVal = LATE(snd_mixer_selem_get_playback_volume)(
      outputMixerElement_, (snd_mixer_selem_channel_id_t)0, &vol);
  if (errVal < 0) {
    LATE(snd_strerror)(errVal);
    return -1;
  }

  volume = static_cast<uint32_t>(vol);

  return 0;
}

int32_t AudioMixerManagerLinuxALSA::MaxSpeakerVolume(
    uint32_t& maxVolume) const {
  if (outputMixerElement_ == nullptr) {
    return -1;
  }

  long int minVol(0);
  long int maxVol(0);

  int errVal = LATE(snd_mixer_selem_get_playback_volume_range)(
      outputMixerElement_, &minVol, &maxVol);

  if (maxVol <= minVol) {
    LATE(snd_strerror)(errVal);
  }

  maxVolume = static_cast<uint32_t>(maxVol);

  return 0;
}

int32_t AudioMixerManagerLinuxALSA::MinSpeakerVolume(
    uint32_t& minVolume) const {
  if (outputMixerElement_ == nullptr) {
    return -1;
  }

  long int minVol(0);
  long int maxVol(0);

  int errVal = LATE(snd_mixer_selem_get_playback_volume_range)(
      outputMixerElement_, &minVol, &maxVol);

  if (maxVol <= minVol) {
    LATE(snd_strerror)(errVal);
  }

  minVolume = static_cast<uint32_t>(minVol);

  return 0;
}

// TL: Have done testnig with these but they don't seem reliable and
// they were therefore not added
/*
 // ----------------------------------------------------------------------------
 //    SetMaxSpeakerVolume
 // ----------------------------------------------------------------------------

 int32_t AudioMixerManagerLinuxALSA::SetMaxSpeakerVolume(
     uint32_t maxVolume)
 {

 if (_outputMixerElement == NULL)
 {
 RTC_LOG(LS_WARNING) << "no avaliable output mixer element exists";
 return -1;
 }

 long int minVol(0);
 long int maxVol(0);

 int errVal = snd_mixer_selem_get_playback_volume_range(
 _outputMixerElement, &minVol, &maxVol);
 if ((maxVol <= minVol) || (errVal != 0))
 {
 RTC_LOG(LS_WARNING) << "Error getting playback volume range: "
                 << snd_strerror(errVal);
 }

 maxVol = maxVolume;
 errVal = snd_mixer_selem_set_playback_volume_range(
 _outputMixerElement, minVol, maxVol);
 RTC_LOG(LS_VERBOSE) << "Playout hardware volume range, min: " << minVol
                 << ", max: " << maxVol;
 if (errVal != 0)
 {
 RTC_LOG(LS_ERROR) << "Error setting playback volume range: "
               << snd_strerror(errVal);
 return -1;
 }

 return 0;
 }

 // ----------------------------------------------------------------------------
 //    SetMinSpeakerVolume
 // ----------------------------------------------------------------------------

 int32_t AudioMixerManagerLinuxALSA::SetMinSpeakerVolume(
     uint32_t minVolume)
 {

 if (_outputMixerElement == NULL)
 {
 RTC_LOG(LS_WARNING) << "no avaliable output mixer element exists";
 return -1;
 }

 long int minVol(0);
 long int maxVol(0);

 int errVal = snd_mixer_selem_get_playback_volume_range(
 _outputMixerElement, &minVol, &maxVol);
 if ((maxVol <= minVol) || (errVal != 0))
 {
 RTC_LOG(LS_WARNING) << "Error getting playback volume range: "
                 << snd_strerror(errVal);
 }

 minVol = minVolume;
 errVal = snd_mixer_selem_set_playback_volume_range(
 _outputMixerElement, minVol, maxVol);
 RTC_LOG(LS_VERBOSE) << "Playout hardware volume range, min: " << minVol
                 << ", max: " << maxVol;
 if (errVal != 0)
 {
 RTC_LOG(LS_ERROR) << "Error setting playback volume range: "
               << snd_strerror(errVal);
 return -1;
 }

 return 0;
 }
 */

int32_t AudioMixerManagerLinuxALSA::SpeakerVolumeIsAvailable(bool& available) {
  if (outputMixerElement_ == nullptr) {
    return -1;
  }

  available = LATE(snd_mixer_selem_has_playback_volume)(outputMixerElement_);

  return 0;
}

int32_t AudioMixerManagerLinuxALSA::SpeakerMuteIsAvailable(bool& available) {
  if (outputMixerElement_ == nullptr) {
    return -1;
  }

  available = LATE(snd_mixer_selem_has_playback_switch)(outputMixerElement_);

  return 0;
}

int32_t AudioMixerManagerLinuxALSA::SetSpeakerMute(bool enable) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (outputMixerElement_ == nullptr) {
    return -1;
  }

  // Ensure that the selected speaker destination has a valid mute control.
  bool available(false);
  SpeakerMuteIsAvailable(available);
  if (!available) {
    return -1;
  }

  // Note value = 0 (off) means muted
  int errVal = LATE(snd_mixer_selem_set_playback_switch_all)(
      outputMixerElement_, !enable);
  if (errVal < 0) {
    LATE(snd_strerror)(errVal);
    return -1;
  }

  return (0);
}

int32_t AudioMixerManagerLinuxALSA::SpeakerMute(bool& enabled) const {
  if (outputMixerElement_ == nullptr) {
    return -1;
  }

  // Ensure that the selected speaker destination has a valid mute control.
  bool available =
      LATE(snd_mixer_selem_has_playback_switch)(outputMixerElement_);
  if (!available) {
    return -1;
  }

  int value(false);

  // Retrieve one boolean control value for a specified mute-control
  //
  int errVal = LATE(snd_mixer_selem_get_playback_switch)(
      outputMixerElement_, (snd_mixer_selem_channel_id_t)0, &value);
  if (errVal < 0) {
    LATE(snd_strerror)(errVal);
    return -1;
  }

  // Note value = 0 (off) means muted
  enabled = (bool)!value;

  return 0;
}

int32_t AudioMixerManagerLinuxALSA::MicrophoneMuteIsAvailable(bool& available) {
  if (inputMixerElement_ == nullptr) {
    return -1;
  }

  available = LATE(snd_mixer_selem_has_capture_switch)(inputMixerElement_);
  return 0;
}

int32_t AudioMixerManagerLinuxALSA::SetMicrophoneMute(bool enable) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (inputMixerElement_ == nullptr) {
    return -1;
  }

  // Ensure that the selected microphone destination has a valid mute control.
  bool available(false);
  MicrophoneMuteIsAvailable(available);
  if (!available) {
    return -1;
  }

  // Note value = 0 (off) means muted
  int errVal =
      LATE(snd_mixer_selem_set_capture_switch_all)(inputMixerElement_, !enable);
  if (errVal < 0) {
    LATE(snd_strerror)(errVal);
    return -1;
  }

  return (0);
}

int32_t AudioMixerManagerLinuxALSA::MicrophoneMute(bool& enabled) const {
  if (inputMixerElement_ == nullptr) {
    return -1;
  }

  // Ensure that the selected microphone destination has a valid mute control.
  bool available = LATE(snd_mixer_selem_has_capture_switch)(inputMixerElement_);
  if (!available) {
    return -1;
  }

  int value(false);

  // Retrieve one boolean control value for a specified mute-control
  //
  int errVal = LATE(snd_mixer_selem_get_capture_switch)(
      inputMixerElement_, (snd_mixer_selem_channel_id_t)0, &value);
  if (errVal < 0) {
    LATE(snd_strerror)(errVal);
    return -1;
  }

  // Note value = 0 (off) means muted
  enabled = (bool)!value;

  return 0;
}

int32_t AudioMixerManagerLinuxALSA::MicrophoneVolumeIsAvailable(
    bool& available) {
  if (inputMixerElement_ == nullptr) {
    return -1;
  }

  available = LATE(snd_mixer_selem_has_capture_volume)(inputMixerElement_);

  return 0;
}

int32_t AudioMixerManagerLinuxALSA::SetMicrophoneVolume(uint32_t volume) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (inputMixerElement_ == nullptr) {
    return -1;
  }

  int errVal =
      LATE(snd_mixer_selem_set_capture_volume_all)(inputMixerElement_, volume);
  if (errVal < 0) {
   LATE(snd_strerror)(errVal);
    return -1;
  }

  return (0);
}

// TL: Have done testnig with these but they don't seem reliable and
// they were therefore not added
/*
 // ----------------------------------------------------------------------------
 //    SetMaxMicrophoneVolume
 // ----------------------------------------------------------------------------

 int32_t AudioMixerManagerLinuxALSA::SetMaxMicrophoneVolume(
     uint32_t maxVolume)
 {

 if (_inputMixerElement == NULL)
 {
 RTC_LOG(LS_WARNING) << "no avaliable output mixer element exists";
 return -1;
 }

 long int minVol(0);
 long int maxVol(0);

 int errVal = snd_mixer_selem_get_capture_volume_range(_inputMixerElement,
  &minVol, &maxVol);
 if ((maxVol <= minVol) || (errVal != 0))
 {
 RTC_LOG(LS_WARNING) << "Error getting capture volume range: "
                 << snd_strerror(errVal);
 }

 maxVol = (long int)maxVolume;
 printf("min %d max %d", minVol, maxVol);
 errVal = snd_mixer_selem_set_capture_volume_range(_inputMixerElement, minVol,
 maxVol); RTC_LOG(LS_VERBOSE) << "Capture hardware volume range, min: " <<
 minVol
                 << ", max: " << maxVol;
 if (errVal != 0)
 {
 RTC_LOG(LS_ERROR) << "Error setting capture volume range: "
               << snd_strerror(errVal);
 return -1;
 }

 return 0;
 }

 // ----------------------------------------------------------------------------
 //    SetMinMicrophoneVolume
 // ----------------------------------------------------------------------------

 int32_t AudioMixerManagerLinuxALSA::SetMinMicrophoneVolume(
 uint32_t minVolume)
 {

 if (_inputMixerElement == NULL)
 {
 RTC_LOG(LS_WARNING) << "no avaliable output mixer element exists";
 return -1;
 }

 long int minVol(0);
 long int maxVol(0);

 int errVal = snd_mixer_selem_get_capture_volume_range(
 _inputMixerElement, &minVol, &maxVol);
 if (maxVol <= minVol)
 {
 //maxVol = 255;
 RTC_LOG(LS_WARNING) << "Error getting capture volume range: "
                 << snd_strerror(errVal);
 }

 printf("min %d max %d", minVol, maxVol);
 minVol = (long int)minVolume;
 errVal = snd_mixer_selem_set_capture_volume_range(
 _inputMixerElement, minVol, maxVol);
 RTC_LOG(LS_VERBOSE) << "Capture hardware volume range, min: " << minVol
                 << ", max: " << maxVol;
 if (errVal != 0)
 {
 RTC_LOG(LS_ERROR) << "Error setting capture volume range: "
               << snd_strerror(errVal);
 return -1;
 }

 return 0;
 }
 */

int32_t AudioMixerManagerLinuxALSA::MicrophoneVolume(uint32_t& volume) const {
  if (inputMixerElement_ == nullptr) {
    return -1;
  }

  long int vol(0);

  int errVal = LATE(snd_mixer_selem_get_capture_volume)(
      inputMixerElement_, (snd_mixer_selem_channel_id_t)0, &vol);
  if (errVal < 0) {
    LATE(snd_strerror)(errVal);
    return -1;
  }

  volume = static_cast<uint32_t>(vol);

  return 0;
}

int32_t AudioMixerManagerLinuxALSA::MaxMicrophoneVolume(
    uint32_t& maxVolume) const {
  if (inputMixerElement_ == nullptr) {
     return -1;
  }

  long int minVol(0);
  long int maxVol(0);

  // check if we have mic volume at all
  if (!LATE(snd_mixer_selem_has_capture_volume)(inputMixerElement_)) {
    return -1;
  }

  int errVal = LATE(snd_mixer_selem_get_capture_volume_range)(
      inputMixerElement_, &minVol, &maxVol);

  if (maxVol <= minVol) {
    LATE(snd_strerror)(errVal);
  }

  maxVolume = static_cast<uint32_t>(maxVol);

  return 0;
}

int32_t AudioMixerManagerLinuxALSA::MinMicrophoneVolume(
    uint32_t& minVolume) const {
  if (inputMixerElement_ == nullptr) {
    return -1;
  }

  long int minVol(0);
  long int maxVol(0);

  int errVal = LATE(snd_mixer_selem_get_capture_volume_range)(
      inputMixerElement_, &minVol, &maxVol);

  if (maxVol <= minVol) {
    LATE(snd_strerror)(errVal);
  }

  minVolume = static_cast<uint32_t>(minVol);

  return 0;
}

// ============================================================================
//                                 Private Methods
// ============================================================================

int32_t AudioMixerManagerLinuxALSA::LoadMicMixerElement() const {
  int errVal = LATE(snd_mixer_load)(inputMixerHandle_);
  if (errVal < 0) {
    LATE(snd_strerror)(errVal);
    inputMixerHandle_ = nullptr;
    return -1;
  }

  snd_mixer_elem_t* elem = nullptr;
  snd_mixer_elem_t* micElem = nullptr;
  unsigned mixerIdx = 0;
  const char* selemName = nullptr;

  // Find and store handles to the right mixer elements
  for (elem = LATE(snd_mixer_first_elem)(inputMixerHandle_); elem;
       elem = LATE(snd_mixer_elem_next)(elem), mixerIdx++) {
    if (LATE(snd_mixer_selem_is_active)(elem)) {
      selemName = LATE(snd_mixer_selem_get_name)(elem);
      if (strcmp(selemName, "Capture") == 0)  // "Capture", "Mic"
      {
        inputMixerElement_ = elem;
      } else if (strcmp(selemName, "Mic") == 0) {
        micElem = elem;
      }
    }

    if (inputMixerElement_) {
      // Use the first Capture element that is found
      // The second one may not work
      break;
    }
  }

  if (inputMixerElement_ == nullptr) {
    // We didn't find a Capture handle, use Mic.
    if (micElem != nullptr) {
      inputMixerElement_ = micElem;
    } else {
      inputMixerElement_ = nullptr;
      return -1;
    }
  }

  return 0;
}

int32_t AudioMixerManagerLinuxALSA::LoadSpeakerMixerElement() const {
  int errVal = LATE(snd_mixer_load)(outputMixerHandle_);
  if (errVal < 0) {
    LATE(snd_strerror)(errVal);
    outputMixerHandle_ = nullptr;
    return -1;
  }

  snd_mixer_elem_t* elem = nullptr;
  snd_mixer_elem_t* masterElem = nullptr;
  snd_mixer_elem_t* speakerElem = nullptr;
  unsigned mixerIdx = 0;
  const char* selemName = nullptr;

  // Find and store handles to the right mixer elements
  for (elem = LATE(snd_mixer_first_elem)(outputMixerHandle_); elem;
       elem = LATE(snd_mixer_elem_next)(elem), mixerIdx++) {
    if (LATE(snd_mixer_selem_is_active)(elem)) {
      selemName = LATE(snd_mixer_selem_get_name)(elem);
      // "Master", "PCM", "Wave", "Master Mono", "PC Speaker", "PCM", "Wave"
      if (strcmp(selemName, "PCM") == 0) {
        outputMixerElement_ = elem;
      } else if (strcmp(selemName, "Master") == 0) {
        masterElem = elem;
      } else if (strcmp(selemName, "Speaker") == 0) {
        speakerElem = elem;
      }
    }

    if (outputMixerElement_) {
      // We have found the element we want
      break;
    }
  }

  // If we didn't find a PCM Handle, use Master or Speaker
  if (outputMixerElement_ == nullptr) {
    if (masterElem != nullptr) {
      outputMixerElement_ = masterElem;
    } else if (speakerElem != nullptr) {
      outputMixerElement_ = speakerElem;
    } else {
      outputMixerElement_ = nullptr;
      return -1;
    }
  }

  return 0;
}

void AudioMixerManagerLinuxALSA::GetControlName(char* controlName,
                                                char* deviceName) const {
  // Example
  // deviceName: "front:CARD=Intel,DEV=0"
  // controlName: "hw:CARD=Intel"
  char* pos1 = strchr(deviceName, ':');
  char* pos2 = strchr(deviceName, ',');
  if (!pos2) {
    // Can also be default:CARD=Intel
    pos2 = &deviceName[strlen(deviceName)];
  }
  if (pos1 && pos2) {
    strcpy(controlName, "hw");
    int nChar = (int)(pos2 - pos1);
    strncpy(&controlName[2], pos1, nChar);
    controlName[2 + nChar] = '\0';
  } else {
    strcpy(controlName, deviceName);
  }
}
