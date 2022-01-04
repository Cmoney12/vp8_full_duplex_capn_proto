//
// Created by corey on 1/3/22.
//

#ifndef VP8_FULL_DUPLEX_CAPN_AUDIO_H
#define VP8_FULL_DUPLEX_CAPN_AUDIO_H

#include <opus/opus.h>
#include <portaudio.h>
#include <portaudio.h>
#include <iostream>

enum Constants {
    PORT_DEFAULT = 56780,
    PORT_MAX     = 56789,
    CHANNELS = 1,               //1 channel (mono) audio 2 channel stereo was 2
    SAMPLE_RATE = 48000,        //48kHz, the number of 16-bit samples per second
    PACKET_MS = 20,             //How long a single packet of samples is (20ms recommended by Opus)
    PACKET_SAMPLES = 960,       //Samples per packet (48kHz * 0.020s = 960 samples)
    ENCODED_MAX_BYTES = 240,    //Max size of a single packet's data once compressed (capacity of opus_encode buffer)
    BUFFERED_PACKETS_MIN = 2,   //How many packets to build up before we start playing audio
    BUFFERED_PACKETS_MAX = 5,   //When too many packets have built up and we start skipping them to speed up playback
    DISCONNNECT_TIMEOUT = 5000, //How long to wait for valid AUDIO packets before we time out and disconnect
    RING_PACKET_INTERVAL = 500, //How often to repeat RING packet
    UPNP_TIMEOUT_MS = 8000,      //Timeout to use when doing UPnP discovery
    NUM_SECONDS = 5,
    inChannels = 0,
    outChannels = 1
};

struct AudioPacket {
    int size; //total size of the network packet
    uint8_t data_[ENCODED_MAX_BYTES];
};


class Audio {
public:
    Audio() {
        PaError paError = Pa_Initialize();
        if (paError != paNoError)
            throw std::runtime_error(std::string("Pa_ReadStream error: ") + Pa_GetErrorText(paError));
    }

    ~Audio() {
        if (decoder)
            opus_decoder_destroy(decoder);
        if (encoder)
            opus_encoder_destroy(encoder);
        stop();
    }

    void audio_init() {
        readParams.device = Pa_GetDefaultInputDevice();
        readParams.channelCount = 1;                    /* stereo input */
        readParams.sampleFormat = paInt16;
        readParams.suggestedLatency = Pa_GetDeviceInfo( readParams.device )->defaultLowInputLatency;
        readParams.hostApiSpecificStreamInfo = nullptr;
        writeParams.device = Pa_GetDefaultOutputDevice();
        writeParams.channelCount = 1;
        writeParams.sampleFormat = paInt16;
        writeParams.suggestedLatency = Pa_GetDeviceInfo( writeParams.device )->defaultHighOutputLatency;
        writeParams.hostApiSpecificStreamInfo = nullptr;

        int opusErr;
        encoder = opus_encoder_create(SAMPLE_RATE, CHANNELS, OPUS_APPLICATION_VOIP, &opusErr);
        if (opusErr != OPUS_OK)
            throw std::runtime_error(std::string("opus_encoder_create error: ") + opus_strerror(opusErr));

        decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &opusErr);
        if (opusErr != OPUS_OK)
            throw std::runtime_error(std::string("opus_decoder_create error: ") + opus_strerror(opusErr));

        //open read stream
        PaError paError = Pa_OpenStream(&read_stream, &readParams, nullptr,
                                        SAMPLE_RATE, PACKET_SAMPLES, paClipOff, nullptr, nullptr);
        if (paError != paNoError)
            throw std::runtime_error(std::string("Pa_ReadStream error: ") + Pa_GetErrorText(paError));

        //open write stream
        paError = Pa_OpenStream(&write_stream, nullptr,&writeParams,
                                SAMPLE_RATE, PACKET_SAMPLES, paClipOff, nullptr,nullptr);

        if (paError != paNoError)
            throw std::runtime_error(std::string("Pa_WriteStream error: ") + Pa_GetErrorText(paError));

        PaError paErr = Pa_StartStream(read_stream);
        if (paErr)
            throw std::runtime_error(std::string("Start read stream failed ") + Pa_GetErrorText(paErr));

        if (Pa_StartStream(write_stream) != paNoError)
            throw std::runtime_error(std::string("Start write stream failed ") + Pa_GetErrorText(paErr));
    }

    AudioPacket read_message() {

        AudioPacket packet{};
        opus_int16 microphone[PACKET_SAMPLES]{};
        PaError paError = Pa_ReadStream(read_stream, microphone, PACKET_SAMPLES);
        if (paError != paNoError) {
            if (paError != paInputOverflowed)
                throw std::runtime_error(std::string("Pa_ReadStream error: ") + Pa_GetErrorText(paError));
        }

        opus_int32 encoded = opus_encode(encoder, microphone, PACKET_SAMPLES, packet.data_, sizeof(packet.data_));
        if (encoded < 0)
            throw std::runtime_error(std::string("opus_encode error: ") + opus_strerror(encoded));

        packet.size = encoded;
        // size is never greater than 150
        return packet;
    }

    void receive_message(const std::uint8_t *data, opus_int size) {
        opus_int16 decoded[PACKET_SAMPLES]{};
        opus_int32 decoded_ret = opus_decode(decoder, data,size, decoded, PACKET_SAMPLES, 0);
        if (decoded_ret == OPUS_INVALID_PACKET)
        {
            // Try again by treating the packet as lost corrupt packet
            decoded_ret = opus_decode(decoder, nullptr, 0, decoded, PACKET_SAMPLES, 0);
        }
        write_message(decoded, PACKET_SAMPLES);
    }

    void write_message(void* buffer, ulong samples) {
        PaError PaErr = Pa_WriteStream(write_stream, buffer, PACKET_SAMPLES);
        if (PaErr != paNoError)
        {
            /**if (PaErr == paOutputUnderflowed)
                //std::cout << "Pa_WriteStream output underflowed" << std::endl;
            else
                throw std::runtime_error(std::string("Pa_WriteStream failed: ") + Pa_GetErrorText(PaErr));**/
            if (PaErr != paOutputUnderflowed)
                throw std::runtime_error(std::string("Pa_WriteStream failed:") + Pa_GetErrorText(PaErr));
        }
    }


    void stop() {
        if (read_stream) {
            PaError err = Pa_StopStream(read_stream);
            if (err)
                throw std::runtime_error(std::string( " Error: ") + static_cast<std::string>(Pa_GetErrorText(err)));
        }

        if (write_stream) {
            PaError err = Pa_StopStream(write_stream);
            if (err)
                throw std::runtime_error(std::string( " Error: ") + static_cast<std::string>(Pa_GetErrorText(err)));
        }
        Pa_Terminate();
    }

private:
    PaStreamParameters readParams{};
    PaStreamParameters writeParams{};
    PaStream *read_stream = nullptr;
    PaStream *write_stream = nullptr;
    OpusEncoder *encoder{};
    OpusDecoder *decoder{};
};

#endif //VP8_FULL_DUPLEX_CAPN_AUDIO_H
