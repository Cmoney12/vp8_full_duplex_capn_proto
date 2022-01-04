//
// Created by corey on 1/3/22.
//

#ifndef VP8_FULL_DUPLEX_CAPN_CHAT_CLIENT_H
#define VP8_FULL_DUPLEX_CAPN_CHAT_CLIENT_H


#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include "Serialization.h"
#include "Video/Decoder.h"
#include "Audio/Audio.h"

using boost::asio::ip::tcp;

typedef std::deque<std::shared_ptr<Serialization>> Serialization_queue;

class chat_client
{
public:
    chat_client(boost::asio::io_context& io_context,
                const tcp::resolver::results_type& endpoints, Decoder *decoder, Audio *audio)
            : io_context_(io_context),
              socket_(io_context)
    {
        audio_ = audio;
        decoder_ = decoder;
        do_connect(endpoints);
    }

    void write(const std::shared_ptr<Serialization>& msg)
    {
        boost::asio::post(io_context_,
                          [this, msg]()
                          {
                              bool write_in_progress = !write_msgs_.empty();
                              write_msgs_.push_back(msg);
                              if (!write_in_progress)
                              {
                                  do_write();
                              }
                          });
    }

    void close()
    {
        boost::asio::post(io_context_, [this]() { socket_.close(); });
    }

    void send_username(char* username) {
        boost::asio::async_write(socket_, boost::asio::buffer(username, std::strlen(username)),
                                 [this](boost::system::error_code ec, std::size_t) {
                                     if (ec) {
                                         socket_.close();
                                     }
                                 });
    }

private:
    void do_connect(const tcp::resolver::results_type& endpoints)
    {
        boost::asio::async_connect(socket_, endpoints,
                                   [this](boost::system::error_code ec, tcp::endpoint)
                                   {
                                       if (!ec)
                                       {
                                           do_read_header();
                                       }
                                   });
    }

    void do_read_header()
    {
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_->head(), Serialization::HEADER_LENGTH),
                                [this](boost::system::error_code ec, std::size_t /*length*/)
                                {
                                    if (!ec && read_msg_->decode_header())
                                    {
                                        do_read_body();
                                    }
                                    else
                                    {
                                        std::cout << ec << std::endl;
                                        socket_.close();
                                    }
                                });
    }


    void do_read_body()
    {
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_->body(), read_msg_->body_length()),
                                [this](boost::system::error_code ec, std::size_t /*length*/)
                                {
                                    if (!ec)
                                    {
                                        NetworkPacket packet_ = read_msg_->parse_contents(read_msg_->body(), read_msg_->body_length());
                                        if (packet_.type != nullptr) {
                                            if (std::strcmp(packet_.type, "Video") == 0) {
                                                decoder_->decode(packet_.data, packet_.size);
                                            } else if (std::strcmp(packet_.type, "Audio") == 0) {
                                                audio_->receive_message(packet_.data, packet_.size);
                                            }
                                        }
                                        do_read_header();
                                    }
                                    else
                                    {
                                        std::cout << ec << std::endl;
                                        socket_.close();
                                    }
                                });
    }

    void do_write()
    {
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(write_msgs_.front()->data(),
                                                     write_msgs_.front()->length()),
                                 [this](boost::system::error_code ec, std::size_t /*length*/)
                                 {
                                     if (!ec)
                                     {
                                         write_msgs_.pop_front();
                                         if (!write_msgs_.empty())
                                         {
                                             do_write();
                                         }
                                     }
                                     else
                                     {
                                         std::cout << ec << std::endl;
                                         socket_.close();
                                     }
                                 });
    }

private:
    //std::mutex mtx;
    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    std::shared_ptr<Serialization> read_msg_ = std::make_shared<Serialization>();
    //Serialization read_msg_;
    Serialization_queue write_msgs_;
    Decoder *decoder_;
    Audio *audio_;
};

#endif //VP8_FULL_DUPLEX_CAPN_CHAT_CLIENT_H
