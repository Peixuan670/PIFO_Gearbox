//
// Created by Zhou Yitao on 2018-12-04.
//

#ifndef HIERARCHICALSCHEDULING_HIERARCHY_H
#define HIERARCHICALSCHEDULING_HIERARCHY_H

#include "queue_pt.h"
#include "address.h"

#include <vector>   // Peixuan 10242020
#include<algorithm> // Peixuan 10242020
using namespace std;

class Level_pifo {
private:
    static const int DEFAULT_VOLUME = 10;
    int volume;                         // num of fifos in one Level_pt
    int currentIndex;                   // current serve index
    int granularity;                    // Peixuan 10242020 PIFO
    PacketQueue_pt *fifos[10];

    // 10232020 Peixuan
    // TODO: PIFO here
    vector<Packet*> pifo;    // Peixuan 10242020 PIFO
    int pifoMaxLength;       // Peixuan 10242020 PIFO

    int pktToRelad; // remainng packets to be reloaded
    int fifoIndexToReload; // index of first non-empty next to VC
    int pifoMaxFT;  // max finish time in PIFO

    int pifoLowThresh // lower threshold of the PIFO

    Packet_metadata* pifoEnque(Packet_metadata* packet); // Peixuan 10242020 PIFO
    Packet_metadata* pifoDeque(); // Peixuan 10242020 PIFO
    int pifoPeekHead(); // Peixuan 10242020 PIFO, Return smallest time stamp
    int pifoPeekTail(); // Peixuan 10242020 PIFO, Return largest time stamp

    int updatePifo_Max_FT();

public:
    Level_pifo();
    void enque(Packet_metadata* packet, int index);
    Packet_metadata* deque();
    int getCurrentIndex();
    void setCurrentIndex(int index);             // 07212019 Peixuan: set serving FIFO (especially for convergence FIFO)
    void getAndIncrementIndex();
    int getCurrentFifoSize();
    bool isCurrentFifoEmpty();

    // 10232020 Peixuan
    bool ifReloadPIFO();            // Find do we need to reload PIFO
    int startReloadPIFO();          // Find first non-empty next to VC and Set pktToRelad as its length
    int startMigrateFIFO();         // Not sure if necessary
    int reloadPkt(int curVC);                // Move one pkt from fifoIndexToReload to PIFO
    Packet_metadata* migratePkt();               // Dequeu on packet from FIFO VC and enque it to scheduler

    int findNextNonEmptyFIFO();     // Find the non-empty FIFO next to VC

    int getPifoMaxFT();
    int getPktNumToReload();

};


#endif //HIERARCHICALSCHEDULING_HIERARCHY_H
