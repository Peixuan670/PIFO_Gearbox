//
// Created by Peixuan Gao on 2020-09-03.
//

#include "Packet_metadata.h"

Packet_metadata::Packet_metadata() {
    //pkt_pt = nullptr;
    pkt_pt = 0;
    finish_time = -1;
    pkt_scheudled = true;
}

Packet_metadata::Packet_metadata(Packet* packet, int finishtime) {
    pkt_pt = packet;
    finish_time = finishtime;
    pkt_scheudled = false;
}

Packet* Packet_metadata::get_pkt() {
    pkt_scheudled = true;               // mark this pkt is scheduled
    return pkt_pt;
}

bool Packet_metadata::isScheduled() {
    return pkt_scheudled;
}

int Packet_metadata::get_finish_time() {
    return finish_time;
}