//
// Created by Zhou Yitao on 2018-12-04.
//

//#ifndef HIERARCHICALSCHEDULING_HIERARCHY_H
//#define HIERARCHICALSCHEDULING_HIERARCHY_H

#include "packet.h"
//#include "queue.h"
//#include "address.h"
using namespace std;
class Packet;

class Packet_metadata {
private:
    //static const int DEFAULT_VOLUME = 10;
    //int volume;                         // num of fifos in one Level_pt
    //int currentIndex;                   // current serve index
    //PacketQueue *fifos[10];

    Packet *pkt_pt;                       // Pointer to the packet
    bool pkt_scheudled;                   // Is the packet already scheduled?
    int finish_time;

public:
    Packet_metadata();
    Packet_metadata(Packet* packet, int finishtime);

    Packet* get_pkt();
    bool isScheduled();
    int get_finish_time();

    // TODO cmp

};


//#endif //HIERARCHICALSCHEDULING_HIERARCHY_H
