//
// Created by corey on 1/3/22.
//

#ifndef VP8_FULL_DUPLEX_CAPN_SERIALIZATION_H
#define VP8_FULL_DUPLEX_CAPN_SERIALIZATION_H

#include <cstring>
#include <cinttypes>
#include <memory>
#include <capnp/common.h>
#include "Proto/Packet.capnp.h"
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

    void create_packet(const char* receiver_username, const char* sender_username,
                       const char* type, std::uint8_t* data, std::size_t size) {

        ::capnp::MallocMessageBuilder message;
        Packet::Builder packet = message.initRoot<Packet>();
        packet.setReceiverUsername(receiver_username);
        packet.setSenderUsername(sender_username);
        packet.setType(type);
        packet.setData(kj::arrayPtr(data, size));
        //capnp::serialize::write_message()

        kj::byte result[capnp::messageToFlatArray(message).asBytes().size()];
        kj::ArrayPtr<kj::byte> bufferPtr = kj::arrayPtr(result, sizeof(result));
        kj::ArrayOutputStream arrayOutputStream(bufferPtr);
        capnp::writeMessage(arrayOutputStream, message);

        //const auto m = capnp::messageToFlatArray(message);
        //const auto c = m.asBytes();

        set_size(arrayOutputStream.getArray().size());
        std::memcpy(data_.get() + HEADER_LENGTH, arrayOutputStream.getArray().begin(),  arrayOutputStream.getArray().size());
        encode_header();

    }

    static NetworkPacket parse_contents(unsigned char* data, const std::size_t size) {

        NetworkPacket packet{};

        auto copy = kj::heapArray<capnp::word>(size / sizeof(capnp::word));
        std::memcpy(copy.begin(), data, size);
        kj::ArrayPtr<capnp::word> received_array = kj::ArrayPtr<capnp::word>(copy.begin(), copy.size());
        ::capnp::FlatArrayMessageReader message_receiver_builder(received_array);
        ::Packet::Reader reader = message_receiver_builder.getRoot<Packet>();

        packet.sender_username = reader.getSenderUsername().cStr();
        packet.receiver_username = reader.getReceiverUsername().cStr();
        packet.type = reader.getType().cStr();
        packet.size = reader.getData().size();
        packet.data = reader.getData().begin();
        return packet;

    }


    bool decode_header() {
        std::memcpy(&body_length_, header, sizeof body_length_);
        set_size(body_length_);
        data_.get()[3] = (body_length_>>24) & 0xFF;
        data_.get()[2] = (body_length_>>16) & 0xFF;
        data_.get()[1] = (body_length_>>8) & 0xFF;
        data_.get()[0] = body_length_ & 0xFF;
        if(body_length_ > MAX_MESSAGE_SIZE) {
            body_length_ = 0;
            return false;
        }
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
    enum { MAX_MESSAGE_SIZE = 999999999 };
    std::unique_ptr<std::uint8_t[]> data_;
    uint8_t header[HEADER_LENGTH]{};
};

#endif //VP8_FULL_DUPLEX_CAPN_SERIALIZATION_H
