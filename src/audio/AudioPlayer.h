#pragma once

#include <stdint.h>
#include <memory>
#include <mutex>

struct mal_device;
struct sts_mixer_t;

class AudioPlayer
{
public:
    AudioPlayer();
    ~AudioPlayer();

    void playSound(const int id, const int civilization, const float pan = 0.f, const float volume = 1.f);

    static AudioPlayer &instance();

private:
    void playSample(const std::shared_ptr<uint8_t[]> &data, const float pan = 0.f, const float volume = 1.f);

    static uint32_t malCallback(mal_device *device, uint32_t frameCount, void *buffer);

    std::unique_ptr<sts_mixer_t> m_mixer;
    std::unique_ptr<mal_device> m_device;
    std::mutex m_mutex;
};

