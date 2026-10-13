/*
 * Copyright (C) 2026 AERA Recovery Project contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Narrow plugin-to-TinyALSA PCM bridge for OnePlus 8 Pro recovery. Untrusted
 * plugins never receive access to ALSA, Binder, stock partitions, or this
 * process. Only the dedicated browser and media UIDs may connect and send
 * fixed-format audio over one abstract local socket.
 */
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <tinyalsa/asoundlib.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

namespace {
constexpr uid_t kBrowserUid = 99090;
constexpr uid_t kMediaUid = 99092;
constexpr uid_t kDoomUid = 99094;
constexpr uint32_t kMagic = 0x41525041U;
constexpr uint32_t kRate = 48000U;
constexpr uint32_t kChannels = 2U;
constexpr uint32_t kBits = 16U;
constexpr unsigned int kPeriodFrames = 1024U;
constexpr size_t kFrameBytes = kChannels * (kBits / 8U);
constexpr size_t kPeriodBytes = kPeriodFrames * kFrameBytes;
constexpr int kIdleTimeoutMs = 250;
constexpr int kDefaultVolumePercent = 30;
constexpr char kSocketName[] = "aera-browser-audio-v1";
constexpr char kBootstrap[] = "/system/bin/aera-audio-bootstrap";
volatile sig_atomic_t g_stop = 0;

void Stop(int) { g_stop = 1; }

bool ReadAll(int fd, void* data, size_t size) {
  auto* bytes = static_cast<uint8_t*>(data);
  size_t done = 0;
  while (done < size) {
    const ssize_t count = read(fd, bytes + done, size - done);
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) return false;
    done += static_cast<size_t>(count);
  }
  return true;
}

int ReadVolumePercent() {
  const int fd = open("/tmp/aera-audio-volume", O_RDONLY | O_CLOEXEC);
  if (fd < 0) return kDefaultVolumePercent;
  char value[8]{};
  const ssize_t count = read(fd, value, sizeof(value) - 1);
  close(fd);
  if (count <= 0) return kDefaultVolumePercent;
  return std::clamp(atoi(value), 0, 100);
}

bool StartBackend() {
  const pid_t child = fork();
  if (child < 0) return false;
  if (!child) {
    execl(kBootstrap, "aera-audio-bootstrap", nullptr);
    _exit(127);
  }
  int status = 0;
  while (waitpid(child, &status, 0) < 0) {
    if (errno != EINTR) return false;
  }
  return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool SetMixerValue(mixer* card, const char* name, int value) {
  mixer_ctl* control = mixer_get_ctl_by_name(card, name);
  if (control == nullptr) {
    fprintf(stderr, "AERA audio: mixer control '%s' is missing\n", name);
    return false;
  }
  const unsigned int count = mixer_ctl_get_num_values(control);
  for (unsigned int index = 0; index < count; ++index) {
    if (mixer_ctl_set_value(control, index, value) != 0) {
      fprintf(stderr, "AERA audio: could not set mixer control '%s'\n", name);
      return false;
    }
  }
  return true;
}

bool SetMixerEnum(mixer* card, const char* name, const char* value) {
  mixer_ctl* control = mixer_get_ctl_by_name(card, name);
  if (control == nullptr) {
    fprintf(stderr, "AERA audio: mixer control '%s' is missing\n", name);
    return false;
  }
  if (mixer_ctl_set_enum_by_string(control, value) != 0) {
    fprintf(stderr, "AERA audio: could not set mixer control '%s' to '%s'\n",
            name, value);
    return false;
  }
  return true;
}

bool PrepareMixer() {
  mixer* card = mixer_open(0);
  if (card == nullptr) {
    fprintf(stderr, "AERA audio: could not open sound card 0 mixer\n");
    return false;
  }
  const bool ready =
      SetMixerEnum(card, "TERT_MI2S_RX Channels", "Two") &&
      SetMixerEnum(card, "TERT_MI2S_RX Format", "S24_LE") &&
      SetMixerValue(card, "TERT_MI2S_RX Audio Mixer MultiMedia1", 1) &&
      SetMixerValue(card, "TFA_CHIP_SELECTOR", 3) &&
      SetMixerEnum(card, "stereo Profile", "speaker");
  mixer_close(card);
  return ready;
}

pcm* OpenPcm() {
  pcm_config config{};
  config.channels = kChannels;
  config.rate = kRate;
  config.period_size = kPeriodFrames;
  config.period_count = 4;
  config.format = PCM_FORMAT_S16_LE;
  config.start_threshold = config.period_size * 2;
  config.stop_threshold = config.period_size * config.period_count;
  config.silence_threshold = config.stop_threshold;
  pcm* output = pcm_open(0, 0, PCM_OUT | PCM_NORESTART, &config);
  if (output == nullptr || !pcm_is_ready(output)) {
    fprintf(stderr, "AERA audio: PCM output is unavailable: %s\n",
            output == nullptr ? "open failed" : pcm_get_error(output));
    if (output != nullptr) pcm_close(output);
    return nullptr;
  }
  return output;
}

bool WritePcm(pcm*& output, const uint8_t* data, size_t size) {
  size_t offset = 0;
  bool restarted = false;
  while (offset < size) {
    const unsigned int frames = static_cast<unsigned int>(
        (size - offset) / kFrameBytes);
    const int written = pcm_writei(output, data + offset, frames);
    if (written <= 0) {
      const int error = errno;
      if (!restarted && (error == EPIPE || error == ESTRPIPE)) {
        // Re-prepare alone leaves this Qualcomm DSP stream stuck after an XRUN.
        fprintf(stderr, "AERA audio: reopening PCM after underrun\n");
        pcm_close(output);
        output = OpenPcm();
        if (output == nullptr) return false;
        restarted = true;
        continue;
      }
      fprintf(stderr, "AERA audio: PCM write failed: %s\n",
              pcm_get_error(output));
      return false;
    }
    offset += static_cast<size_t>(written) * kFrameBytes;
  }
  return true;
}

void ServeClient(int client) {
  ucred credentials{};
  socklen_t credentials_size = sizeof(credentials);
  if (getsockopt(client, SOL_SOCKET, SO_PEERCRED, &credentials,
                 &credentials_size) != 0 ||
      credentials_size != sizeof(credentials) ||
      (credentials.uid != kBrowserUid && credentials.uid != kMediaUid &&
       credentials.uid != kDoomUid)) {
    fprintf(stderr, "AERA audio: rejected local UID %u\n", credentials.uid);
    return;
  }
  std::array<uint32_t, 4> hello{};
  if (!ReadAll(client, hello.data(), sizeof(hello)) || hello[0] != kMagic ||
      hello[1] != kRate || hello[2] != kChannels || hello[3] != kBits) {
    fprintf(stderr, "AERA audio: rejected invalid PCM handshake\n");
    return;
  }

  pcm* output = nullptr;
  // Socket reads are not audio boundaries. Submit complete DSP periods only.
  std::array<uint8_t, kPeriodBytes> buffer{};
  bool good = true;
  size_t filled = 0;
  unsigned volume_counter = 0;
  int volume = ReadVolumePercent();
  while (good && !g_stop) {
    pollfd input{client, POLLIN, 0};
    const int ready = poll(&input, 1, kIdleTimeoutMs);
    if (ready < 0 && errno == EINTR) continue;
    if (ready < 0) break;
    if (ready == 0) {
      if (output != nullptr) {
        pcm_close(output);
        output = nullptr;
      }
      continue;
    }
    if (!(input.revents & POLLIN)) break;
    const ssize_t count = read(client, buffer.data() + filled,
                               buffer.size() - filled);
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) break;
    filled += static_cast<size_t>(count);
    if (filled != buffer.size()) continue;
    if (output == nullptr) {
      output = OpenPcm();
      if (output == nullptr) break;
    }
    if ((volume_counter++ % 12U) == 0) volume = ReadVolumePercent();
    for (size_t offset = 0; offset < filled; offset += 2) {
      int16_t sample = static_cast<int16_t>(
          static_cast<uint16_t>(buffer[offset]) |
          (static_cast<uint16_t>(buffer[offset + 1]) << 8));
      sample = static_cast<int16_t>((static_cast<int32_t>(sample) * volume) /
                                    100);
      buffer[offset] = static_cast<uint8_t>(sample);
      buffer[offset + 1] = static_cast<uint8_t>(
          static_cast<uint16_t>(sample) >> 8);
    }
    good = WritePcm(output, buffer.data(), filled);
    filled = 0;
  }
  if (output != nullptr) pcm_close(output);
}
}  // namespace

int main(int argc, char** argv) {
  if (getuid() != 0 || argc != 2) {
    fprintf(stderr,
            "Usage (root recovery only): aera-audio-bridge --prepare|--browser-audio\n");
    return 64;
  }
  if (strcmp(argv[1], "--prepare") == 0) return PrepareMixer() ? 0 : 69;
  if (strcmp(argv[1], "--browser-audio") != 0) return 64;
  signal(SIGPIPE, SIG_IGN);
  signal(SIGTERM, Stop);
  signal(SIGINT, Stop);
  prctl(PR_SET_PDEATHSIG, SIGTERM, 0, 0, 0);

  const int server = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (server < 0) return 70;
  sockaddr_un address{};
  address.sun_family = AF_UNIX;
  memcpy(address.sun_path + 1, kSocketName, sizeof(kSocketName) - 1);
  const socklen_t address_size = static_cast<socklen_t>(
      offsetof(sockaddr_un, sun_path) + 1 + sizeof(kSocketName) - 1);
  if (bind(server, reinterpret_cast<const sockaddr*>(&address), address_size) != 0 ||
      listen(server, 1) != 0) {
    close(server);
    return 70;
  }
  if (!StartBackend()) {
    fprintf(stderr, "AERA audio: Kona TinyALSA backend failed to start\n");
    close(server);
    return 69;
  }
  fprintf(stderr, "AERA audio: browser PCM bridge ready\n");
  while (!g_stop) {
    const int client = accept4(server, nullptr, nullptr, SOCK_CLOEXEC);
    if (client < 0) {
      if (errno == EINTR) continue;
      break;
    }
    ServeClient(client);
    close(client);
  }
  close(server);
  return 0;
}
