//
// Created by Zhou Yitao on 2018-12-04.
//

#include "Level_pifo.h"

Level_pifo::Level_pifo(): volume(10), currentIndex(0) {
    // Debug: Peixuan 07062019 Level_pifo Constructor
    // Bug: where is FIFO itself?

    // Debug: Peixuan 07062019 Level_pifo Constructor
    for (int i=0; i<10; i++) {
        fifos[i] = new PacketQueue_pt;
    }
    //fprintf(stderr, "Constructed Level_pifo\n"); // Debug: Peixuan 07062019

}

void Level_pifo::enque(Packet_metadata* packet, int index) {
    // packet.setInsertFifo(index);
    // packet.setFifoPosition(static_cast<int>(fifos[index].size()));
    // hdr_ip* iph = hdr_ip::access(packet);

    // Peixuan 10232020  Solved TODO: if pkt->FT < pifoMaxFT: enqueu PIFO, else:
    if (!this->pifoMaxFT < packet->get_finish_time()) {
        pifoEnque(packet);
    } else {
        fifos[index]->enque(packet);
    }
}

Packet_metadata* Level_pifo::deque() {
    Packet_metadata *packet;

    fprintf(stderr, "Dequeue from this Level_pifo\n"); // Debug: Peixuan 07062019

    if (isCurrentFifoEmpty()) {
        fprintf(stderr, "No packet in the current serving FIFO\n"); // Debug: Peixuan 07062019
        return 0;
    }
    packet = fifos[currentIndex]->deque();
    return packet;
}

int Level_pifo::getCurrentIndex() {
    return currentIndex;
}

void Level_pifo::setCurrentIndex(int index) {
    currentIndex = index;
}

void Level_pifo::getAndIncrementIndex() {
    if (currentIndex + 1 < volume) {
        currentIndex++;
    } else {
        currentIndex = 0;
    }
}

bool Level_pifo::isCurrentFifoEmpty() {
    fprintf(stderr, "Checking if the FIFO is empty\n"); // Debug: Peixuan 07062019
    //fifos[currentIndex]->length() == 0;
    //fprintf(stderr, "Bug here solved\n"); // Debug: Peixuan 07062019
    return fifos[currentIndex]->length() == 0;
}

int Level_pifo::getCurrentFifoSize() {
    return fifos[currentIndex]->length();
}

// 10232020 Peixuan
bool Level_pifo::ifReloadPIFO() {
    // if (PIFO->length() <= this->pifoLowThresh) {return true;}
    return false;
}

int Level_pifo::startReloadPIFO() {
    // Find first non-empty next to VC and Set pktToRelad as its length
    
    this->fifoIndexToReload = this->findNextNonEmptyFIFO();
    return 1;
}          
int Level_pifo::startMigrateFIFO() {
    // Not sure if necessary
    return 0;
}         
int Level_pifo::reloadPkt(int curVC) {
    // Move one pkt from fifoIndexToReload to PIFO
    Packet_metadata* packet = fifos[fifoIndexToReload]->deque();
    // Solved TODO: Packet_metadata* evi_pkt = PIFO.enque(packet);
    Packet_metadata* pkt_pushed_out = pifoEnque(packet);
    // Solved TODO: update this->pifoMaxFT;
    updatePifo_Max_FT();   
    // TODO: if (evi_pkt) {level->enque(evi_pkt) / OR scheduler->enque(evi_pkt) }
    if (pkt_pushed_out) {
        int finish_time = pkt_pushed_out->get_finish_time();
        int index = (int) (finish_time - curVC)/this->granularity; // TODO: index + 1 or not?
        enque(pkt_pushed_out, index); // solved TODO: we need to know the FIFO to push back
    }
    return 1;
}                
Packet_metadata* Level_pifo::migratePkt() {
    Packet_metadata* packet = fifos[currentIndex]->deque();
    return packet; // re-enque this packet to the scheduler

    // Dequeu on packet from FIFO VC and enque it to scheduler
}

int findNextNonEmptyFIFO() {
    // Find the non-empty FIFO next to VC
    int index = (this->currentIndex + 1);
    do{
        if (index==this->volume) {
            index = 0;
        }
    } while (fifos[index]->length() == 0);
    return index;
}     

int Level_pifo::getPifoMaxFT() {
    return this->pifoMaxFT;
}
int Level_pifo::getPktNumToReload() {
    return this->pktToRelad;
}



// Peixuan 10242020, PIFO, Private:

bool cmp(Packet_metadata* a, Packet_metadata* b)
{
    return a->finish_time - b->finish_time;
    //return a>b;
}

Packet_metadata* Level_pifo::pifoEnque(Packet_metadata* packet) {
    this->pifo.push_back(packet);
    sort(pifo.begin(),pifo.end(),cmp)
    if (this->pifo.size() > this->pifoMaxLength) {
        Packet_metadata* pkt_pushed_out = this->pifo[this->pifo.size()-1];
        pifo.erase(pifo.end()); // OR this->pifo.size()-1?
        return pkt_pushed_out;
    }
    return 0;
} 

Packet_metadata* Level_pifo::pifoDeque() {
    Packet_metadata* pkt_dequed = pifo[0];
    pifo.erase(pifo.begin()); // OR pifo[0]?
    return pkt_dequed;
} 

int Level_pifo::pifoPeekHead() {
    // Return smallest time stamp
    return pifo[0]->finish_time;
} 

int Level_pifo::pifoPeekTail() {
    // Return largest time stamp
    return pifo[pifo.size()-1]->finish_time;
} 

int Level_pifo::updatePifo_Max_FT() {
    this->pifoMaxFT = this->pifo[this->pifo.size()-1]->finish_time; // Update Max FT in PIFO
    return 1
}
