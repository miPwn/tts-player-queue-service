#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <pulse/simple.h>
#include <pulse/error.h>

class AudioPlayer {
public:
    AudioPlayer(const std::string& sink_name = "");
    ~AudioPlayer();

    void play(const std::vector<char>& wav_data);
    double getWavDuration(const std::vector<char>& wav_data);
    bool isPlaying() const;

private:
    std::string sink_name_;
    mutable std::mutex mutex_;
    bool is_playing_;
    
    struct WavHeader {
        char riff[4];
        uint32_t file_size;
        char wave[4];
        char fmt[4];
        uint32_t fmt_size;
        uint16_t audio_format;
        uint16_t num_channels;
        uint32_t sample_rate;
        uint32_t byte_rate;
        uint16_t block_align;
        uint16_t bits_per_sample;
        char data[4];
        uint32_t data_size;
    } __attribute__((packed));

    void parseWavHeader(const std::vector<char>& wav_data, WavHeader& header);
    void playRaw(const char* data, size_t size, uint32_t sample_rate, 
                 uint16_t channels, uint16_t bits_per_sample);
};
