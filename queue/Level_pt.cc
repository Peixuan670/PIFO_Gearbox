//
// Created by Zhou Yitao on 2018-12-04.
//

#include "Level_pt.h"

Level_pt::Level_pt(): volume(10), currentIndex(0) {
    // Debug: Peixuan 07062019 Level_pt Constructor
    // Bug: where is FIFO itself?

    // Debug: Peixuan 07062019 Level_pt Constructor
    for (int i=0; i<10; i++) {
        fifos[i] = new PacketQueue_pt;
    }
    //fprintf(stderr, "Constructed Level_pt\n"); // Debug: Peixuan 07062019

}

void Level_pt::enque(Packet_metadata* packet, int index) {
    // packet.setInsertFifo(index);
    // packet.setFifoPosition(static_cast<int>(fifos[index].size()));
    // hdr_ip* iph = hdr_ip::access(packet);

    fifos[index]->enque(packet);
}

Packet_metadata* Level_pt::deque() {
    Packet_metadata *packet;

    fprintf(stderr, "Dequeue from this Level_pt\n"); // Debug: Peixuan 07062019

    if (isCurrentFifoEmpty()) {
        fprintf(stderr, "No packet in the current serving FIFO\n"); // Debug: Peixuan 07062019
        return 0;
    }
    packet = fifos[currentIndex]->deque();
    return packet;
}

int Level_pt::getCurrentIndex() {
    return currentIndex;
}

void Level_pt::setCurrentIndex(int index) {
    currentIndex = index;
}

void Level_pt::getAndIncrementIndex() {
    if (currentIndex + 1 < volume) {
        currentIndex++;
    } else {
        currentIndex = 0;
    }
}

bool Level_pt::isCurrentFifoEmpty() {
    fprintf(stderr, "Checking if the FIFO is empty\n"); // Debug: Peixuan 07062019
    //fifos[currentIndex]->length() == 0;
    //fprintf(stderr, "Bug here solved\n"); // Debug: Peixuan 07062019
    return fifos[currentIndex]->length() == 0;
}

int Level_pt::getCurrentFifoSize() {
    return fifos[currentIndex]->length();
}
