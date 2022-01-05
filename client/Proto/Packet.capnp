@0xde7eed51c00f0fd8;

struct Packet {
    senderUsername @0 :Text;
    receiverUsername @1 :Text;
    type @2 :Text;
    data @3 :Data;
}

# capnp compile -oc++ Packet.capnp