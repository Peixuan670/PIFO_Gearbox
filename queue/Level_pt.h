//
// Created by Zhou Yitao on 2018-12-04.
//

#ifndef HIERARCHICALSCHEDULING_HIERARCHY_H
#define HIERARCHICALSCHEDULING_HIERARCHY_H

#include "queue_pt.h"
#include "address.h"
using namespace std;

class Level_pt {
private:
    static const int DEFAULT_VOLUME = 10;
    int volume;                         // num of fifos in one Level_pt
    int currentIndex;                   // current serve index
    PacketQueue_pt *fifos[10];
public:
    Level_pt();
    void enque(Packet_metadata* packet, int index);
    Packet_metadata* deque();
    int getCurrentIndex();
    void setCurrentIndex(int index);             // 07212019 Peixuan: set serving FIFO (especially for convergence FIFO)
    void getAndIncrementIndex();
    int getCurrentFifoSize();
    bool isCurrentFifoEmpty();
};


#endif //HIERARCHICALSCHEDULING_HIERARCHY_H
