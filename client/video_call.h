//
// Created by corey on 1/3/22.
//

#ifndef VP8_FULL_DUPLEX_CAPN_VIDEO_CALL_H
#define VP8_FULL_DUPLEX_CAPN_VIDEO_CALL_H

#include <boost/asio.hpp>
#include "Audio/Audio.h"
#include "Video/Encoder.h"
#include "Video/Decoder.h"
#include "chat_client.h"
#include <opencv4/opencv2/opencv.hpp>

using namespace boost::asio;
using boost::asio::ip::tcp;

class VideoCall {
public:
    explicit VideoCall(char *username):username_(username) {
        ip::tcp::resolver resolver(io_context);
        ip::tcp::resolver::query query("127.0.0.1","1234");
        auto endpoint = resolver.resolve(query);
        audio = new Audio;
        encoder = new Encoder(width, height, fps);
        decoder = new Decoder(width, height);
        client = new chat_client(io_context, endpoint, decoder, audio);
        network_thread = new std::thread([this](){io_context.run();});
    }

    ~VideoCall() {
        client->close();
        network_thread->join();
        delete network_thread;
        delete audio;
        delete encoder;
        delete decoder;
        delete client;
    }

    void send_username() {
        int len = std::strlen(username_);
        char username_line[len + 2];
        std::strcpy(username_line, username_);
        std::strcat(username_line, "\n");
        std::cout << "username len " << std::strlen(username_line) << std::endl;
        client->send_username(username_line);
        std::cout << username_line;
    }

    void start_audio() {
        audio->audio_init();
        char type[] = "Audio";
        audio_send_active = true;
        while (audio_send_active) {
            try {
                AudioPacket packet = audio->read_message();
                std::shared_ptr<Serialization> message = std::make_shared<Serialization>();
                message->create_packet(username_, username_, type, packet.data_, packet.size);
                client->write(message);
            } catch (std::error_code& ec) {
                std::cerr << ec << std::endl;
            }
        }
    }

    void stop_audio() {
        audio_send_active = false;
    }

    void start_video() {
        video_send_active = true;
        char type[] = "Video";
        cv::VideoCapture cap(0);
        cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
        cap.set(cv::CAP_PROP_FPS, fps);
        cap.open(0);
        while(video_send_active) {
            cap >> image;

            std::shared_ptr<VideoPacket> packet = encoder->encode(image);

            if (packet != nullptr && packet->size > 0) {
                std::shared_ptr<Serialization> message = std::make_shared<Serialization>();
                message->create_packet(username_, username_, type, packet->data.get(), packet->size);
                client->write(message);
            }
        }
    }

    void display_video() {
        while(video_send_active) {
            decoder->display();
        }
    }

private:
    char* username_;
    Audio *audio;
    Decoder *decoder;
    Encoder *encoder;
    boost::asio::io_context io_context;
    chat_client *client;
    cv::Mat image;
    int width = 1280;
    int height = 720;
    int fps = 30;
    int rows = (int) (height * 1.5);
    int cols = width;
    std::thread *network_thread;
    bool audio_send_active = false;
    bool video_send_active = false;
};

#endif //VP8_FULL_DUPLEX_CAPN_VIDEO_CALL_H
