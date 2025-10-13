#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "audio_player.h"
#include <cstring>
#include <vector>

class AudioPlayerTest : public ::testing::Test {
protected:
    void SetUp() override {
        sink_name = "";
    }
    
    std::string sink_name;
    
    std::vector<char> createValidWav(int sample_rate = 44100, int channels = 2, int duration_samples = 1000) {
        std::vector<char> wav;
        
        wav.insert(wav.end(), {'R', 'I', 'F', 'F'});
        
        uint32_t file_size = 36 + duration_samples * channels * 2;
        wav.push_back(file_size & 0xFF);
        wav.push_back((file_size >> 8) & 0xFF);
        wav.push_back((file_size >> 16) & 0xFF);
        wav.push_back((file_size >> 24) & 0xFF);
        
        wav.insert(wav.end(), {'W', 'A', 'V', 'E'});
        
        wav.insert(wav.end(), {'f', 'm', 't', ' '});
        
        uint32_t fmt_size = 16;
        wav.push_back(fmt_size & 0xFF);
        wav.push_back((fmt_size >> 8) & 0xFF);
        wav.push_back((fmt_size >> 16) & 0xFF);
        wav.push_back((fmt_size >> 24) & 0xFF);
        
        uint16_t audio_format = 1;
        wav.push_back(audio_format & 0xFF);
        wav.push_back((audio_format >> 8) & 0xFF);
        
        uint16_t num_channels = channels;
        wav.push_back(num_channels & 0xFF);
        wav.push_back((num_channels >> 8) & 0xFF);
        
        uint32_t sample_rate_val = sample_rate;
        wav.push_back(sample_rate_val & 0xFF);
        wav.push_back((sample_rate_val >> 8) & 0xFF);
        wav.push_back((sample_rate_val >> 16) & 0xFF);
        wav.push_back((sample_rate_val >> 24) & 0xFF);
        
        uint32_t byte_rate = sample_rate * channels * 2;
        wav.push_back(byte_rate & 0xFF);
        wav.push_back((byte_rate >> 8) & 0xFF);
        wav.push_back((byte_rate >> 16) & 0xFF);
        wav.push_back((byte_rate >> 24) & 0xFF);
        
        uint16_t block_align = channels * 2;
        wav.push_back(block_align & 0xFF);
        wav.push_back((block_align >> 8) & 0xFF);
        
        uint16_t bits_per_sample = 16;
        wav.push_back(bits_per_sample & 0xFF);
        wav.push_back((bits_per_sample >> 8) & 0xFF);
        
        wav.insert(wav.end(), {'d', 'a', 't', 'a'});
        
        uint32_t data_size = duration_samples * channels * 2;
        wav.push_back(data_size & 0xFF);
        wav.push_back((data_size >> 8) & 0xFF);
        wav.push_back((data_size >> 16) & 0xFF);
        wav.push_back((data_size >> 24) & 0xFF);
        
        for (int i = 0; i < duration_samples * channels; ++i) {
            int16_t sample = (i % 100) * 100;
            wav.push_back(sample & 0xFF);
            wav.push_back((sample >> 8) & 0xFF);
        }
        
        return wav;
    }
};

TEST_F(AudioPlayerTest, ValidWavHeader) {
    AudioPlayer player(sink_name);
    
    auto wav = createValidWav();
    
    EXPECT_NO_THROW({
        player.play(wav);
    });
}

TEST_F(AudioPlayerTest, InvalidWavTooSmall) {
    AudioPlayer player(sink_name);
    
    std::vector<char> wav = {'R', 'I', 'F', 'F'};
    
    EXPECT_THROW(player.play(wav), std::runtime_error);
}

TEST_F(AudioPlayerTest, InvalidWavNotRIFF) {
    AudioPlayer player(sink_name);
    
    auto wav = createValidWav();
    wav[0] = 'X';
    
    EXPECT_THROW(player.play(wav), std::runtime_error);
}

TEST_F(AudioPlayerTest, InvalidWavNotWAVE) {
    AudioPlayer player(sink_name);
    
    auto wav = createValidWav();
    wav[8] = 'X';
    
    EXPECT_THROW(player.play(wav), std::runtime_error);
}

TEST_F(AudioPlayerTest, MonoAudio) {
    AudioPlayer player(sink_name);
    
    auto wav = createValidWav(44100, 1);
    
    EXPECT_NO_THROW({
        player.play(wav);
    });
}

TEST_F(AudioPlayerTest, StereoAudio) {
    AudioPlayer player(sink_name);
    
    auto wav = createValidWav(44100, 2);
    
    EXPECT_NO_THROW({
        player.play(wav);
    });
}

TEST_F(AudioPlayerTest, DifferentSampleRates) {
    AudioPlayer player(sink_name);
    
    std::vector<int> sample_rates = {8000, 16000, 22050, 44100, 48000};
    
    for (int rate : sample_rates) {
        auto wav = createValidWav(rate, 2, 100);
        EXPECT_NO_THROW({
            player.play(wav);
        }) << "Failed for sample rate: " << rate;
    }
}

TEST_F(AudioPlayerTest, ShortAudio) {
    AudioPlayer player(sink_name);
    
    auto wav = createValidWav(44100, 2, 10);
    
    EXPECT_NO_THROW({
        player.play(wav);
    });
}

TEST_F(AudioPlayerTest, LongAudio) {
    AudioPlayer player(sink_name);
    
    auto wav = createValidWav(44100, 2, 44100);
    
    EXPECT_NO_THROW({
        player.play(wav);
    });
}

TEST_F(AudioPlayerTest, EmptyWavData) {
    AudioPlayer player(sink_name);
    
    std::vector<char> empty;
    
    EXPECT_THROW(player.play(empty), std::runtime_error);
}

TEST_F(AudioPlayerTest, CustomSink) {
    AudioPlayer player("custom_sink");
    
    auto wav = createValidWav();
}

TEST_F(AudioPlayerTest, ParseWavHeaderCorrectly) {
    AudioPlayer player(sink_name);
    
    auto wav = createValidWav(48000, 1, 500);
    
    EXPECT_NO_THROW({
        player.play(wav);
    });
}

TEST_F(AudioPlayerTest, MultiplePlaybacksSequential) {
    AudioPlayer player(sink_name);
    
    for (int i = 0; i < 3; ++i) {
        auto wav = createValidWav(44100, 2, 100);
        EXPECT_NO_THROW({
            player.play(wav);
        });
    }
}
