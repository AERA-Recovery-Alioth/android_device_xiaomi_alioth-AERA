// SPDX-License-Identifier: Apache-2.0
// Exercise the real bridge transport with hardware calls replaced by a PCM recorder.
#define main AudioBridgeMain
#include "aera_audio_bridge.cpp"
#undef main
#include <cassert>
#include <fstream>
#include <map>
#include <sys/stat.h>
#include <chrono>
#include <thread>
#include <vector>
static uid_t peer_uid = kMediaUid;
static unsigned opens = 0, closes = 0;
static std::vector<int16_t> samples;
extern "C" int __wrap_getsockopt(int, int, int, void* value, socklen_t* size) {
  auto* credentials = static_cast<ucred*>(value);
  *credentials = {}; credentials->uid = peer_uid; *size = sizeof(ucred);
  return 0;
}
extern "C" pcm* __wrap_pcm_open(unsigned card, unsigned device, unsigned flags, const pcm_config* config) {
  assert(card == 0 && device == 0 && (flags & PCM_NORESTART));
  assert(config->channels == 2 && config->rate == 48000 && config->format == PCM_FORMAT_S16_LE);
  ++opens; return reinterpret_cast<pcm*>(0x1000);
}
extern "C" int __wrap_pcm_get_poll_fd(pcm*) { return -1; }
extern "C" int __wrap_pcm_is_ready(const pcm*) { return 1; }
extern "C" int __wrap_pcm_close(pcm*) { ++closes; return 0; }
extern "C" const char* __wrap_pcm_get_error(const pcm*) { return "test"; }
extern "C" int __wrap_pcm_writei(pcm*, const void* data, unsigned frames) {
  assert(frames == kPeriodFrames);
  auto* begin = static_cast<const int16_t*>(data);
  samples.insert(samples.end(), begin, begin + frames * kChannels);
  return static_cast<int>(frames);
}
struct TestControl { std::string name; };
static std::map<std::string, TestControl> controls;
static std::map<std::string, std::string> values;
static std::string absent, failing;
extern "C" mixer* __wrap_mixer_open(unsigned card) { assert(card == 0); return reinterpret_cast<mixer*>(0x1000); }
extern "C" void __wrap_mixer_close(mixer*) {}
extern "C" mixer_ctl* __wrap_mixer_get_ctl_by_name(mixer*, const char* name) {
  if (absent == name) return nullptr;
  auto& ctl = controls[name]; ctl.name = name;
  return reinterpret_cast<mixer_ctl*>(&ctl);
}
extern "C" unsigned __wrap_mixer_ctl_get_num_values(const mixer_ctl*) { return 1; }
static int Store(mixer_ctl* opaque, const std::string& value) {
  const auto& name = reinterpret_cast<TestControl*>(opaque)->name;
  if (failing == name) { failing.clear(); return -1; }
  values[name] = value; return 0;
}
extern "C" int __wrap_mixer_ctl_set_value(mixer_ctl* ctl, unsigned, int value) { return Store(ctl, std::to_string(value)); }
extern "C" int __wrap_mixer_ctl_set_enum_by_string(mixer_ctl* ctl, const char* value) { return Store(ctl, value); }
static void CheckMixerFailure() {
  const bool existing_lock = access("/tmp/aera-audio-mixer.lock", F_OK) == 0;
  absent = "RCV AMP PCM Gain";
  assert(!PrepareMixer() && values.empty());
  absent.clear(); failing = "TERT_TDM_RX_0 Format";
  assert(!PrepareMixer());
  assert(values["TERT_TDM_RX_0 Audio Mixer MultiMedia1"] == "0");
  assert(values["PCM Source"] == "None" && values["RCV PCM Source"] == "None");
  assert(values["DSP1 Preload Switch"] == "0" && values["RCV DSP1 Preload Switch"] == "0");
  assert(PrepareMixer());
  assert(values["TERT_TDM_RX_0 Audio Mixer MultiMedia1"] == "1");
  assert(values["PCM Source"] == "DSP" && values["RCV PCM Source"] == "DSP");
  assert(values["DSP1 Firmware"] == "Protection" && values["RCV DSP1 Firmware"] == "Protection");
  if (!existing_lock) unlink("/tmp/aera-audio-mixer.lock");
}
static void Send(int fd, const void* data, size_t size) {
  auto* bytes = static_cast<const char*>(data);
  while (size) { ssize_t n = write(fd, bytes, size); assert(n > 0); bytes += n; size -= n; }
}
int main() {
  signal(SIGPIPE, SIG_IGN);
  CheckMixerFailure();
  for (int scenario = 0; scenario < 4; ++scenario) {
    int pair[2]; assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    samples.clear(); opens = closes = 0; peer_uid = scenario == 1 ? 1000 : kMediaUid;
    auto start = std::chrono::steady_clock::now();
    std::thread server([&] { ServeClient(pair[1]); close(pair[1]); });
    if (scenario != 3) {
      std::array<uint32_t, 4> hello{kMagic, kRate, kChannels, kBits};
      if (scenario == 2) hello[1] = 44100;
      Send(pair[0], hello.data(), sizeof(hello));
      if (scenario == 0) {
        std::vector<int16_t> payload(kPeriodFrames * kChannels * 2, 10000);
        const char* bytes = reinterpret_cast<const char*>(payload.data());
        Send(pair[0], bytes, 13); // split in the middle of a stereo frame
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        for (size_t off = 13; off < payload.size() * 2;) {
          size_t chunk = std::min<size_t>(37, payload.size() * 2 - off);
          Send(pair[0], bytes + off, chunk); off += chunk;
        }
      }
      shutdown(pair[0], SHUT_WR);
    }
    server.join(); close(pair[0]);
    if (scenario == 0) {
      assert(opens == 1 && closes == 1);
      assert(samples.size() == kPeriodFrames * kChannels * 2);
      for (int16_t sample : samples) assert(sample == 3000);
    } else { assert(opens == 0 && samples.empty()); }
    if (scenario == 3) assert(std::chrono::steady_clock::now() - start < std::chrono::seconds(3));
  }
  assert(prctl(PR_SET_CHILD_SUBREAPER, 1, 0, 0, 0) == 0);
  char directory[] = "/tmp/alioth-audio-test-XXXXXX";
  assert(mkdtemp(directory));
  std::string script = std::string(directory) + "/bootstrap";
  std::string marker = std::string(directory) + "/child";
  { std::ofstream f(script); f << "#!/bin/sh\nsleep 30 &\necho $! > " << marker << "\nwait\n"; }
  assert(chmod(script.c_str(), 0700) == 0);
  assert(!StartBackend(script.c_str(), std::chrono::milliseconds(250)));
  pid_t descendant = 0;
  { std::ifstream f(marker); f >> descendant; }
  assert(descendant > 0);
  int child_status = 0;
  assert(waitpid(descendant, &child_status, 0) == descendant);
  assert(WIFSIGNALED(child_status) && WTERMSIG(child_status) == SIGKILL);
  unlink(script.c_str()); unlink(marker.c_str()); rmdir(directory);
  puts("Audio bridge: fragmentation, idle continuation, volume, UID rejection, format rejection handshake timeout and bootstrap descendant cleanup passed");
}
