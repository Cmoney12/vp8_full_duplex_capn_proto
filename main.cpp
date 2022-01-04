#include <iostream>
#include "video_call.h"


int main() {
    char username[] = "user";
    auto call = new VideoCall(username);
    call->send_username();
    std::thread t([call](){call->start_video();});
    std::thread t2([call](){call->display_video();});
    //std::thread t([call](){call->start_audio();});
    call->start_audio();
    t.join();
    t2.join();
    delete call;
    return 0;
}
