#include "audio_player.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <cstring>
#include <thread>
#include <chrono>

AudioPlayer::AudioPlayer(const std::string& sink_name)
    : sink_name_(sink_name), is_playing_(false) {
    spdlog::info("Audio player initialized with sink: {}", 
                 sink_name_.empty() ? "default" : sink_name_);
}

AudioPlayer::~AudioPlayer() = default;

void AudioPlayer::parseWavHeader(const std::vector<char>& wav_data, WavHeader& header) {
    if (wav_data.size() < sizeof(WavHeader)) {
        throw std::runtime_error("Invalid WAV file: too small");
    }
    
    std::memcpy(&header, wav_data.data(), sizeof(WavHeader));
    
    if (std::strncmp(header.riff, "RIFF", 4) != 0 || 
        std::strncmp(header.wave, "WAVE", 4) != 0) {
        throw std::runtime_error("Invalid WAV file: missing RIFF/WAVE headers");
    }
}

double AudioPlayer::getWavDuration(const std::vector<char>& wav_data) {
    try {
        WavHeader header;
        parseWavHeader(wav_data, header);
        
        double duration = static_cast<double>(header.data_size) / 
                         static_cast<double>(header.byte_rate);
        return duration;
    } catch (const std::exception& e) {
        spdlog::error("Error getting WAV duration: {}", e.what());
        return 0.0;
    }
}

void AudioPlayer::play(const std::vector<char>& wav_data) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        WavHeader header;
        parseWavHeader(wav_data, header);
        
        const char* audio_data = wav_data.data() + sizeof(WavHeader);
        size_t audio_size = header.data_size;
        
        is_playing_ = true;
        playRaw(audio_data, audio_size, header.sample_rate, 
                header.num_channels, header.bits_per_sample);
        is_playing_ = false;
        
        spdlog::debug("Finished playing audio ({}s)", getWavDuration(wav_data));
    } catch (const std::exception& e) {
        is_playing_ = false;
        spdlog::error("Error playing audio: {}", e.what());
        throw;
    }
}

void AudioPlayer::playRaw(const char* data, size_t size, uint32_t sample_rate,
                          uint16_t channels, uint16_t bits_per_sample) {
    pa_sample_spec ss;
    ss.format = (bits_per_sample == 16) ? PA_SAMPLE_S16LE : PA_SAMPLE_U8;
    ss.rate = sample_rate;
    ss.channels = channels;

    int error;
    pa_simple* s = pa_simple_new(
        nullptr,
        "TTS Playback Service",
        PA_STREAM_PLAYBACK,
        sink_name_.empty() ? nullptr : sink_name_.c_str(),
        "TTS Audio",
        &ss,
        nullptr,
        nullptr,
        &error
    );

    if (!s) {
        throw std::runtime_error(std::string("PulseAudio error: ") + pa_strerror(error));
    }

    if (pa_simple_write(s, data, size, &error) < 0) {
        pa_simple_free(s);
        throw std::runtime_error(std::string("PulseAudio write error: ") + pa_strerror(error));
    }

    if (pa_simple_drain(s, &error) < 0) {
        pa_simple_free(s);
        throw std::runtime_error(std::string("PulseAudio drain error: ") + pa_strerror(error));
    }

    pa_simple_free(s);
}

bool AudioPlayer::isPlaying() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return is_playing_;
}
