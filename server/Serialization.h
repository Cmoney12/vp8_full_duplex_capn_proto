//
// Created by corey on 1/1/22.
//

#ifndef VP8_SERVER_SERIALIZATION_H
#define VP8_SERVER_SERIALIZATION_H

#include <cstring>
#include <cinttypes>
#include <memory>
#include <capnp/common.h>
#include "../Proto/Packet.capnp.h"
#include <capnp/message.h>
#include <capnp/serialize-packed.h>


struct NetworkPacket {
    const char* sender_username;
    const char* receiver_username;
    const char* type;
    std::size_t size;
    const std::uint8_t *data;
};

class Serialization {
public:

    enum { HEADER_LENGTH = 4 };

    Serialization():body_length_(0) {}

    void create_packet() {
        data_ = std::make_unique<std::uint8_t[]>(body_length_ + HEADER_LENGTH);
        encode_header();
    }

    std::uint8_t *data() {
        return data_.get();
    }

    std::size_t body_length() const {
        return body_length_;
    }

    std::size_t length() const {
        return body_length_ + HEADER_LENGTH;
    }

    std::uint8_t* head() {
        return header;
    }

    std::uint8_t* body() {
        return data_.get() + HEADER_LENGTH;
    }

    void set_size(const std::size_t& size) {
        body_length_ = size;
        data_ = std::make_unique<std::uint8_t[]>(size + HEADER_LENGTH);
    }

    static std::string parse_contents(std::uint8_t* data, const std::size_t size) {

        std::cout << size << std::endl;

        std::string username;
        try {
            auto copy = kj::heapArray<capnp::word>(size / sizeof(capnp::word));
            std::memcpy(copy.begin(), data, size);
            kj::ArrayPtr<capnp::word> received_array = kj::ArrayPtr<capnp::word>(copy.begin(), copy.size());
            ::capnp::FlatArrayMessageReader message_receiver_builder(received_array);
            ::Packet::Reader reader = message_receiver_builder.getRoot<Packet>();
            username = reader.getReceiverUsername().cStr();
        } catch (const std::exception& ec) {
            throw ec;
        }
        return username;

    }


    bool decode_header() {
        if(body_length_ > MAX_MESSAGE_SIZE) {
            body_length_ = 0;
            return false;
        }
        std::memcpy(&body_length_, header, sizeof body_length_);
        set_size(body_length_);
        data_.get()[3] = (body_length_>>24) & 0xFF;
        data_.get()[2] = (body_length_>>16) & 0xFF;
        data_.get()[1] = (body_length_>>8) & 0xFF;
        data_.get()[0] = body_length_ & 0xFF;
        return true;
    }

    bool encode_header() {
        if (body_length_ <= MAX_MESSAGE_SIZE && body_length_) {
            data_.get()[3] = (body_length_>>24) & 0xFF;
            data_.get()[2] = (body_length_>>16) & 0xFF;
            data_.get()[1] = (body_length_>>8) & 0xFF;
            data_.get()[0] = body_length_ & 0xFF;
            return true;
        }
        return false;
    }

private:
    std::size_t body_length_;
    enum { MAX_MESSAGE_SIZE = 9999999 };
    std::unique_ptr<std::uint8_t[]> data_;
    uint8_t header[HEADER_LENGTH]{};
};


#endif //VP8_SERVER_SERIALIZATION_H
