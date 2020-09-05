#include <cmath>
#include <sstream>

#include "PIFOGearbox_pl.h"

static class PIFOGearbox_plClass : public TclClass {
public:
        PIFOGearbox_plClass() : TclClass("Queue/PIFOGEARBOXPL") {}
        TclObject* create(int, const char*const*) {
            fprintf(stderr, "Created new TCL HCSPL instance\n"); // Debug: Peixuan 07062019
	        return (new PIFOGearbox_pl);
	}
} class_hierarchical_queue;

PIFOGearbox_pl::PIFOGearbox_pl():PIFOGearbox_pl(DEFAULT_VOLUME) {
    fprintf(stderr, "Created new PIFOGEARBOXPL instance\n"); // Debug: Peixuan 07062019
}

PIFOGearbox_pl::PIFOGearbox_pl(int volume) {
    fprintf(stderr, "Created new PIFOGEARBOXPL instance with volumn = %d\n", volume); // Debug: Peixuan 07062019
    this->volume = volume;
    currentRound = 0;
    pktCount = 0; // 07072019 Peixuan
    //pktCurRound = new vector<Packet*>;
    //12132019 Peixuan
    typedef std::map<string, Flow_pl*> FlowMap;
    FlowMap flowMap;

    //typedef std::map<int, int> TestIntMap;
    //TestIntMap testIntMap;
}

void PIFOGearbox_pl::setCurrentRound(int currentRound) {
    fprintf(stderr, "Set Current Round: %d\n", currentRound); // Debug: Peixuan 07062019
    this->currentRound = currentRound;

    level0ServingB = ((int)(currentRound/10)%2);
    level1ServingB = ((int)(currentRound/100)%2);

    fprintf(stderr, "Set Round: %d, level 0 serving B = %d, level 1 serving B = %d.\n", currentRound, level0ServingB, level1ServingB); // Debug: Peixuan 07062019

}

void PIFOGearbox_pl::setPktCount(int pktCount) {
    fprintf(stderr, "Set Packet Count: %d\n", pktCount); // Debug: Peixuan 07072019
    this->pktCount = pktCount;
}

void PIFOGearbox_pl::enque(Packet* packet) {   
    
    hdr_ip* iph = hdr_ip::access(packet);
    int pkt_size = packet->hdrlen_ + packet->datalen();

    fprintf(stderr, "AAAAA Start Enqueue Flow %d Packet\n", iph->saddr()); // Debug: Peixuan 07062019

    ///////////////////////////////////////////////////
    // TODO: get theory departure Round
    // You can get flowId from iph, then get
    // "lastDepartureRound" -- departure round of last packet of this flow
    int departureRound = cal_theory_departure_round(iph, pkt_size);
    //fprintf(stderr, "Calculated departure round of this Flow %d Packet, finish time = %d\n", iph->saddr(), departureRound); // Debug: Peixuan 12142019

    ///////////////////////////////////////////////////

    // 20190626 Yitao
    /* With departureRound and currentRound, we can get the insertLevel, insertLevel is a parameter of flow and we can set and read this variable.
    */

    //int flowId = iph->flowid();
    //string key = convertKeyValue(iph->saddr(), iph->daddr());
    string key = convertKeyValue(iph->flowid()); // Peixuan 04212020 fid
    // Not find the current key
    if (flowMap.find(key) == flowMap.end()) {
        //flowMap[key] = Flow_pl(iph->saddr, iph->daddr, 2, 100);
        //insertNewFlowPtr(iph->saddr(), iph->daddr(), 2, 100);
        //this->insertNewFlowPtr(iph->saddr(), iph->daddr(), DEFAULT_WEIGHT, DEFAULT_BRUSTNESS);
        this->insertNewFlowPtr(iph->flowid(), DEFAULT_WEIGHT, DEFAULT_BRUSTNESS); // Peixuan 04212020 fid
    }


    Flow_pl* currFlow = flowMap[key];
    int insertLevel = currFlow->getInsertLevel();

    departureRound = max(departureRound, currentRound);

    if ((departureRound / 100 - currentRound / 100) >= 10) {
        fprintf(stderr, "?????Exceeds maximum round, drop the packet from Flow %d\n", iph->saddr()); // Debug: Peixuan 07072019
        drop(packet);
        return;   // 07072019 Peixuan: exceeds the maximum round
    }

    
    //int curFlowID = convertKeyValue(iph->saddrm, iph->daddr)   // use source IP as flow id
    int curBrustness = currFlow->getBrustness();
    if ((departureRound - currentRound) >= curBrustness) {
        fprintf(stderr, "?????Exceeds maximum brustness, drop the packet from Flow %d\n", iph->saddr()); // Debug: Peixuan 07072019
        drop(packet);
        return;   // 07102019 Peixuan: exceeds the maximum brustness
    }

    currFlow->setLastDepartureRound(departureRound);     // 07102019 Peixuan: only update last packet finish time if the packet wasn't dropped
    //this->updateFlowPtr(iph->saddr(), iph->daddr(), currFlow);  //12182019 Peixuan
    this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
    
    fprintf(stderr, "At Round: %d, Enqueue Packet from Flow %d with Finish time = %d.\n", currentRound, iph->saddr(), departureRound); // Debug: Peixuan 07072019

    int level0InsertingB = ((int)(departureRound/10)%2);
    int level1InsertingB = ((int)(departureRound/100)%2);

    fprintf(stderr, "Level 1 insert B: %d, Level 0 insert B: %d\n", level1InsertingB, level0InsertingB); // Debug: Peixuan 07072019

    Packet_metadata* pkt_mt = new Packet_metadata(packet, departureRound); // 09052020 Peixuan: encapsulating pkt into metadata

    if (departureRound / 100 - currentRound / 100 > 1 || insertLevel == 2) {
        //fprintf(stderr, "Enqueue Level 2\n"); // Debug: Peixuan 07072019
        if (departureRound / 100 % 10 == 5) {
            currFlow->setInsertLevel(1);
            //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
            this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
            hundredLevel.enque(packet, departureRound / 10 % 10);
            fprintf(stderr, "Enqueue Level 2, hundred FIFO, fifo %d\n", departureRound / 10 % 10); // Debug: Peixuan 07072019
        } else {
            currFlow->setInsertLevel(2);
            //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
            this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
            levels[2].enque(packet, departureRound / 100 % 10);
            fprintf(stderr, "Enqueue Level 2, regular FIFO, fifo %d\n", departureRound / 100 % 10); // Debug: Peixuan 07072019
        }
    } else if (departureRound / 10 - currentRound / 10 > 1 || insertLevel == 1) {
        if (!level1InsertingB) {
            //fprintf(stderr, "Enqueue Level 1\n"); // Debug: Peixuan 07072019
            if (departureRound / 10 % 10 == 5) {
                currFlow->setInsertLevel(0);
                //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
                decadeLevel.enque(packet, departureRound  % 10);
                fprintf(stderr, "Enqueue Level 1, decede FIFO, fifo %d\n", departureRound  % 10); // Debug: Peixuan 07072019
            } else {
                currFlow->setInsertLevel(1);
                //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
                levels[1].enque(packet, departureRound / 10 % 10);
                fprintf(stderr, "Enqueue Level 1, regular FIFO, fifo %d\n", departureRound / 10 % 10); // Debug: Peixuan 07072019
            }
        } else {
            //fprintf(stderr, "Enqueue Level B 1\n"); // Debug: Peixuan 07072019
            if (departureRound / 10 % 10 == 5) {
                currFlow->setInsertLevel(0);
                //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
                decadeLevelB.enque(packet, departureRound  % 10);
                fprintf(stderr, "Enqueue Level B 1, decede FIFO, fifo %d\n", departureRound  % 10); // Debug: Peixuan 07072019
            } else {
                currFlow->setInsertLevel(1);
                //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
                this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
                levelsB[1].enque(packet, departureRound / 10 % 10);
                fprintf(stderr, "Enqueue Level B 1, regular FIFO, fifo %d\n", departureRound / 10 % 10); // Debug: Peixuan 07072019
            }
        }

    } else {
        if (!level0InsertingB) {
            //fprintf(stderr, "Enqueue Level 0\n"); // Debug: Peixuan 07072019
            currFlow->setInsertLevel(0);
            //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
            this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
            levels[0].enque(packet, departureRound % 10);
            fprintf(stderr, "Enqueue Level 0, regular FIFO, fifo %d\n", departureRound % 10); // Debug: Peixuan 07072019
        } else {
            //fprintf(stderr, "Enqueue Level B 0\n"); // Debug: Peixuan 07072019
            currFlow->setInsertLevel(0);
            //this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);  //12182019 Peixuan
            this->updateFlowPtr(iph->flowid(), currFlow);  // Peixuan 04212020 fid
            levelsB[0].enque(packet, departureRound % 10);
            fprintf(stderr, "Enqueue Level B 0, regular FIFO, fifo %d\n", departureRound % 10); // Debug: Peixuan 07072019
        }
        
    }

    setPktCount(pktCount + 1);
    fprintf(stderr, "Packet Count ++\n");

    fprintf(stderr, "Finish Enqueue\n"); // Debug: Peixuan 07062019
}

// Peixuan: This can be replaced by any other algorithms
int PIFOGearbox_pl::cal_theory_departure_round(hdr_ip* iph, int pkt_size) {
    //int		fid_;	/* flow id */
    //int		prio_;
    // parameters in iph
    // TODO

    // Peixuan 06242019
    // For simplicity, we assume flow id = the index of array 'flows'

    fprintf(stderr, "$$$$$Calculate Departure Round at VC = %d\n", currentRound); // Debug: Peixuan 07062019

    string key = convertKeyValue(iph->flowid());    // Peixuan 04212020 fid

    Flow_pl* currFlow = this->getFlowPtr(iph->flowid()); // Peixuan 04212020 fid



    float curWeight = currFlow->getWeight();
    int curLastDepartureRound = currFlow->getLastDepartureRound();
    int curStartRound = max(currentRound, curLastDepartureRound);

    fprintf(stderr, "$$$$$Last Departure Round of Flow%d = %d\n",iph->saddr() , curLastDepartureRound); // Debug: Peixuan 07062019
    fprintf(stderr, "$$$$$Start Departure Round of Flow%d = %d\n",iph->saddr() , curStartRound); // Debug: Peixuan 07062019

    //int curDeaprtureRound = (int)(curStartRound + pkt_size/curWeight); // TODO: This line needs to take another thought

    int curDeaprtureRound = (int)(curStartRound + curWeight); // 07072019 Peixuan: basic test

    fprintf(stderr, "$$$$$At Round: %d, Calculated Packet From Flow:%d with Departure Round = %d\n", currentRound, iph->saddr(), curDeaprtureRound); // Debug: Peixuan 07062019
    // TODO: need packet length and bandwidh relation
    //flows[curFlowID].setLastDepartureRound(curDeaprtureRound);
    return curDeaprtureRound;
}

//06262019 Peixuan deque function of Gearbox:

//06262019 Static getting all the departure packet in this virtual clock unit (JUST FOR SIMULATION PURPUSE!)

Packet* PIFOGearbox_pl::deque() {

    fprintf(stderr, "Start Dequeue\n"); // Debug: Peixuan 07062019

    //fprintf(stderr, "Queue size: %d\n",pktCurRound.size()); // Debug: Peixuan 07062019

    if (pktCount == 0) {
        fprintf(stderr, "Scheduler Empty\n"); // Debug: Peixuan 07062019
        return 0;
    }

    //int i = 0; // 07252019 loop debug

    while (!pktCurRound.size()) {   // 07252019 loop debug
    //while (!pktCurRound.size() && i < 1000) {   // 07252019 loop debug
        fprintf(stderr, "Empty Round\n"); // Debug: Peixuan 07062019
        pktCurRound = this->runRound();
        this->setCurrentRound(currentRound + 1); // Update system virtual clock

        level0ServingB = ((int)(currentRound/10)%2);
        level1ServingB = ((int)(currentRound/100)%2);

        fprintf(stderr, "Now update Round: %d, level 0 serving B = %d, level 1 serving B = %d.\n", currentRound, level0ServingB, level1ServingB); // Debug: Peixuan 07062019
        //this->deque();
        //i++; // 07252019 loop debug
        //if (i > 1000) { // 07252019 loop debug
        //    return 0;   // 07252019 loop debug
        //}   // 07252019 loop debug
    }

    Packet *p = pktCurRound.front();
    pktCurRound.erase(pktCurRound.begin());

    setPktCount(pktCount - 1);
    fprintf(stderr, "Packet Count --\n");

    hdr_ip* iph = hdr_ip::access(p);
    fprintf(stderr, "*****Dequeue Packet p with soure IP: %x\n", iph->saddr()); // Debug: Peixuan 07062019

    // Printing sequence test
    // hdr_tcp* tcph = hdr_tcp::access(p);

    return p;

}

// Peixuan: now we only call this function to get the departure packet in the next round
vector<Packet*> PIFOGearbox_pl::runRound() {

    fprintf(stderr, "Run Round\n"); // Debug: Peixuan 07062019

    fprintf(stderr, "Level 2 serving: %d \n", levels[2].getCurrentIndex()); // Debug: Peixuan 07062019
    fprintf(stderr, "Level 1 serving: %d \n", levels[1].getCurrentIndex()); // Debug: Peixuan 07062019
    fprintf(stderr, "Level 1 B serving: %d \n", levelsB[1].getCurrentIndex()); // Debug: Peixuan 07062019
    fprintf(stderr, "Level 0 serving: %d \n", levels[0].getCurrentIndex()); // Debug: Peixuan 07062019
    fprintf(stderr, "Level 0 B serving: %d \n", levelsB[0].getCurrentIndex()); // Debug: Peixuan 07062019

    fprintf(stderr, "Hundred Level serving: %d \n", hundredLevel.getCurrentIndex()); // Debug: Peixuan 07062019
    fprintf(stderr, "Decade Level serving: %d \n", decadeLevel.getCurrentIndex()); // Debug: Peixuan 07062019
    fprintf(stderr, "Decade Level B serving: %d \n", decadeLevelB.getCurrentIndex()); // Debug: Peixuan 07062019

    vector<Packet*> result;

    // Debug: Peixuan 07062019: Bug Founded: What if the queue is empty at the moment? Check Size!

    //current round done

    vector<Packet*> upperLevelPackets = serveUpperLevel(currentRound);

    // Peixuan
    /*for (int i = 0; i < upperLevelPackets.size(); i++) {
        packetNumRecord.push_back(packetNum);
        packetNum--;
    }*/

    result.insert(result.end(), upperLevelPackets.begin(), upperLevelPackets.end());
    
    //fprintf(stderr, "Extracting packet\n"); // Debug: Peixuan 07062019

    if (!level0ServingB) {
        Packet* p = levels[0].deque();

        //fprintf(stderr, "Get packet pointer\n"); // Debug: Peixuan 07062019

        if (!p) {
            fprintf(stderr, "No packet\n"); // Debug: Peixuan 07062019
        }

        while (p) {

            hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

            fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level 0: fifo %d\n", currentRound, iph->saddr(), levels[0].getCurrentIndex()); // Debug: Peixuan 07092019

            result.push_back(p);
            p = levels[0].deque();
        }

        levels[0].getAndIncrementIndex();               // Level 0 move to next FIFO
        fprintf(stderr, "<<<<<At Round:%d, Level 0 update current FIFO as: fifo %d\n", currentRound, levels[0].getCurrentIndex()); // Debug: Peixuan 07212019

        if (levels[0].getCurrentIndex() == 0) {
            if (!level1ServingB) {
                levels[1].getAndIncrementIndex();           // Level 1 move to next FIFO
                fprintf(stderr, "<<<<<At Round:%d, Level 1 update current FIFO as: fifo %d\n", currentRound, levels[1].getCurrentIndex()); // Debug: Peixuan 07212019

                if (levels[1].getCurrentIndex() == 0)
                    levels[2].getAndIncrementIndex();       // Level 2 move to next FIFO
                    fprintf(stderr, "<<<<<At Round:%d, Level 2 update current FIFO as: fifo %d\n", currentRound, levels[2].getCurrentIndex()); // Debug: Peixuan 07212019

            } else {
                levelsB[1].getAndIncrementIndex();           // Level 1 move to next FIFO
                fprintf(stderr, "<<<<<At Round:%d, Level B 1 update current FIFO as: fifo %d\n", currentRound, levels[1].getCurrentIndex()); // Debug: Peixuan 07212019

                if (levelsB[1].getCurrentIndex() == 0)
                    levels[2].getAndIncrementIndex();       // Level 2 move to next FIFO
                    fprintf(stderr, "<<<<<At Round:%d, Level 2 update current FIFO as: fifo %d\n", currentRound, levels[2].getCurrentIndex()); // Debug: Peixuan 07212019
            }
            
        }
    } else {
        Packet* p = levelsB[0].deque();

        //fprintf(stderr, "Get packet pointer\n"); // Debug: Peixuan 07062019

        if (!p) {
            fprintf(stderr, "No packet\n"); // Debug: Peixuan 07062019
        }

        while (p) {

            hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

            fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level B 0: fifo %d\n", currentRound, iph->saddr(), levelsB[0].getCurrentIndex()); // Debug: Peixuan 07092019

            result.push_back(p);
            p = levelsB[0].deque();
        }

        levelsB[0].getAndIncrementIndex();               // Level 0 move to next FIFO
        fprintf(stderr, "<<<<<At Round:%d, Level B 0 update current FIFO as: fifo %d\n", currentRound, levelsB[0].getCurrentIndex()); // Debug: Peixuan 07212019

        if (levelsB[0].getCurrentIndex() == 0) {

            if (!level1ServingB) {
                levels[1].getAndIncrementIndex();           // Level 1 move to next FIFO
                fprintf(stderr, "<<<<<At Round:%d, Level 1 update current FIFO as: fifo %d\n", currentRound, levels[1].getCurrentIndex()); // Debug: Peixuan 07212019

                if (levels[1].getCurrentIndex() == 0)
                    levels[2].getAndIncrementIndex();       // Level 2 move to next FIFO
                    fprintf(stderr, "<<<<<At Round:%d, Level 2 update current FIFO as: fifo %d\n", currentRound, levelsB[2].getCurrentIndex()); // Debug: Peixuan 07212019

            } else {
                levelsB[1].getAndIncrementIndex();           // Level 1 move to next FIFO
                fprintf(stderr, "<<<<<At Round:%d, Level B 1 update current FIFO as: fifo %d\n", currentRound, levels[1].getCurrentIndex()); // Debug: Peixuan 07212019

                if (levelsB[1].getCurrentIndex() == 0)
                    levels[2].getAndIncrementIndex();       // Level 2 move to next FIFO
                    fprintf(stderr, "<<<<<At Round:%d, Level 2 update current FIFO as: fifo %d\n", currentRound, levelsB[2].getCurrentIndex()); // Debug: Peixuan 07212019
            }

            //levelsB[1].getAndIncrementIndex();           // Level 1 move to next FIFO
            //fprintf(stderr, "<<<<<At Round:%d, Level B 1 update current FIFO as: fifo %d\n", currentRound, levelsB[1].getCurrentIndex()); // Debug: Peixuan 07212019

            //if (levelsB[1].getCurrentIndex() == 0)
            //    levelsB[2].getAndIncrementIndex();       // Level 2 move to next FIFO
            //    fprintf(stderr, "<<<<<At Round:%d, Level 2 update current FIFO as: fifo %d\n", currentRound, levelsB[2].getCurrentIndex()); // Debug: Peixuan 07212019
        }

    }
    return result;
}

//Peixuan: This is also used to get the packet served in this round (VC unit)
// We need to adjust the order of serving: level0 -> level1 -> level2
vector<Packet*> PIFOGearbox_pl::serveUpperLevel(int currentRound) {

    fprintf(stderr, "Serving Upper Level\n"); // Debug: Peixuan 07062019

    vector<Packet*> result;

    // ToDo: swap the order of serving levels

    //First: then level 2
    if (currentRound / 100 % 10 == 5) {
        //int size = static_cast<int>(ceil(hundredLevel.getCurrentFifoSize() * 1.0 / (10 - currentRound % 10)));  // 07212019 Peixuan *** Fix Level 2 serving order (ori)
        //int size = static_cast<int>(ceil((hundredLevel.getCurrentFifoSize() + levels[1].getCurrentFifoSize()) * 1.0 / (10 - currentRound % 10)));  // 07212019 Peixuan *** Fix Level 2 serving order (fixed)

        int size = 0;

        if (!level1ServingB) {
            size = static_cast<int>(ceil((hundredLevel.getCurrentFifoSize() + levels[1].getCurrentFifoSize()) * 1.0 / (10 - currentRound % 10)));  // 07212019 Peixuan *** Fix Level 2 serving order (fixed)
        } else {
            size = static_cast<int>(ceil((hundredLevel.getCurrentFifoSize() + levelsB[1].getCurrentFifoSize()) * 1.0 / (10 - currentRound % 10)));  // 07212019 Peixuan *** Fix Level 2 serving order (fixed)
        }

        //size = static_cast<int>(ceil((hundredLevel.getCurrentFifoSize() + levels[1].getCurrentFifoSize() + levelsB[1].getCurrentFifoSize()) * 1.0 / (10 - currentRound % 10)));  // 07212019 Peixuan *** Fix Level 2 serving order (fixed)

        //int size = hundredLevel.getCurrentFifoSize();
        for (int i = 0; i < size; i++) {
            Packet* p = hundredLevel.deque();
            if (p == 0)
                break;
            hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

            fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level 2 Convergence FIFO: fifo %d\n", currentRound, iph->saddr(), hundredLevel.getCurrentIndex()); // Debug: Peixuan 07092019
            result.push_back(p);
        }

        if (hundredLevel.getCurrentFifoSize() && currentRound / 10 % 10 != 5)  // 07222019 Peixuan ***: If hundredLevel not empty, serve it until it is empty (Except Level 1 is serving Convergence FIFO (decade FIFO))
        //if (hundredLevel.getCurrentFifoSize())  // 07212019 Peixuan ***: If hundredLevel not empty, serve it until it is empty
            return result;                      // 07212019 Peixuan ***

        if (currentRound % 10 == 9)
            hundredLevel.getAndIncrementIndex();


    } else if (!levels[2].isCurrentFifoEmpty()) {
        int size = static_cast<int>(ceil(levels[2].getCurrentFifoSize() * 1.0 / (100 - currentRound % 100)));
        for (int i = 0; i < size; i++) {
            Packet* p = levels[2].deque();
            if (p == 0)
                break;
            hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

            fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level 2, fifo: %d\n", currentRound, iph->saddr(), levels[2].getCurrentIndex()); // Debug: Peixuan 07092019
            result.push_back(p);
        }
    }

    //Then: first level 1
    if (currentRound / 10 % 10 == 5) {

        if (!level1ServingB) {
            int size = decadeLevel.getCurrentFifoSize();
            fprintf(stderr, ">>>>>At Round:%d, Serve Level 1 Convergence FIFO with fifo: %d, size: %d\n", currentRound, decadeLevel.getCurrentIndex(), size); // Debug: Peixuan 07222019
            for (int i = 0; i < size; i++) {
                Packet* p = decadeLevel.deque();
                if (p == 0)
                    break;
                result.push_back(p);

                hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

                fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level 1 Convergence FIFO, fifo: %d\n", currentRound, iph->saddr(), decadeLevel.getCurrentIndex()); // Debug: Peixuan 07092019
            }
            decadeLevel.getAndIncrementIndex();
        } else {
            // serving backup decade level
            int size = decadeLevelB.getCurrentFifoSize();
            fprintf(stderr, ">>>>>At Round:%d, Serve Level B 1 Convergence FIFO with fifo: %d, size: %d\n", currentRound, decadeLevelB.getCurrentIndex(), size); // Debug: Peixuan 07222019
            for (int i = 0; i < size; i++) {
                Packet* p = decadeLevelB.deque();
                if (p == 0)
                    break;
                result.push_back(p);

                hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

                fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level B 1 Convergence FIFO, fifo: %d\n", currentRound, iph->saddr(), decadeLevelB.getCurrentIndex()); // Debug: Peixuan 07092019

            }
            decadeLevelB.getAndIncrementIndex();

        }

       
    }
    else {
        if (!level1ServingB) {
            if (!levels[1].isCurrentFifoEmpty()) {
                int size = static_cast<int>(ceil(levels[1].getCurrentFifoSize() * 1.0 / (10 - currentRound % 10)));   // 07212019 Peixuan *** Fix Level 1 serving order (ori)
                //int size = static_cast<int>(ceil((hundredLevel.getCurrentFifoSize() + levels[1].getCurrentFifoSize()) * 1.0 / (10 - currentRound % 10)));  // 07212019 Peixuan *** Fix Level 1 serving order (fixed)
                fprintf(stderr, ">>>At Round:%d, Serve Level 1 Regular FIFO with fifo: %d, size: %d\n", currentRound, levels[1].getCurrentIndex(), size); // Debug: Peixuan 07222019
                for (int i = 0; i < size; i++) {
                    Packet* p = levels[1].deque();
                    if (p == 0)
                        break;
                    hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

                    fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level 1, fifo: %d\n", currentRound, iph->saddr(), levels[1].getCurrentIndex()); // Debug: Peixuan 07092019
                    result.push_back(p);
                }  
            }
        } else {
            if (!levelsB[1].isCurrentFifoEmpty()) {
                int size = static_cast<int>(ceil(levelsB[1].getCurrentFifoSize() * 1.0 / (10 - currentRound % 10)));   // 07212019 Peixuan *** Fix Level 1 serving order (ori)
                //int size = static_cast<int>(ceil((hundredLevel.getCurrentFifoSize() + levels[1].getCurrentFifoSize()) * 1.0 / (10 - currentRound % 10)));  // 07212019 Peixuan *** Fix Level 1 serving order (fixed)
                fprintf(stderr, ">>>At Round:%d, Serve Level 1 B Regular FIFO with fifo: %d, size: %d\n", currentRound, levelsB[1].getCurrentIndex(), size); // Debug: Peixuan 07222019
                for (int i = 0; i < size; i++) {
                    Packet* p = levelsB[1].deque();
                    if (p == 0)
                        break;
                    hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

                    fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level B 1, fifo: %d\n", currentRound, iph->saddr(), levelsB[1].getCurrentIndex()); // Debug: Peixuan 07092019
                    result.push_back(p);
                }  
            }

        }
    } 

   

    return result;
}


// 12132019 Peixuan
//Flow_pl* PIFOGearbox_pl::getFlowPtr(nsaddr_t saddr, nsaddr_t daddr) {
Flow_pl* PIFOGearbox_pl::getFlowPtr(int fid) {
    
    string key = convertKeyValue(fid);  // Peixuan 04212020
    Flow_pl* flow; 
    if (flowMap.find(key) == flowMap.end()) {
        //flow = this->insertNewFlowPtr(saddr, daddr, 2, 100);
        //flow = this->insertNewFlowPtr(saddr, daddr, DEFAULT_WEIGHT, DEFAULT_BRUSTNESS);
        flow = this->insertNewFlowPtr(fid, DEFAULT_WEIGHT, DEFAULT_BRUSTNESS); // Peixuan 04212020
    }
    flow = this->flowMap[key];
    return flow;
}

//Flow_pl* PIFOGearbox_pl::insertNewFlowPtr(nsaddr_t saddr, nsaddr_t daddr, int weight, int brustness) {
Flow_pl* PIFOGearbox_pl::insertNewFlowPtr(int fid, int weight, int brustness) { // Peixuan 04212020
    //pair<ns_addr_t, ns_addr_t> key = make_pair(saddr, daddr);
    //string key = convertKeyValue(saddr, daddr);
    string key = convertKeyValue(fid);  // Peixuan 04212020
    Flow_pl* newFlowPtr = new Flow_pl(1, weight, brustness);
    //this->flowMap.insert(pair<pair<ns_addr_t, ns_addr_t>, Flow_pl*>(key, newFlowPtr));
    this->flowMap.insert(pair<string, Flow_pl*>(key, newFlowPtr));
    //flowMap.insert(pair(key, newFlowPtr));
    //return 0;
    return this->flowMap[key];
}

//int PIFOGearbox_pl::updateFlowPtr(nsaddr_t saddr, nsaddr_t daddr, Flow_pl* flowPtr) {
int PIFOGearbox_pl::updateFlowPtr(int fid, Flow_pl* flowPtr) { // Peixuan 04212020
    //pair<ns_addr_t, ns_addr_t> key = make_pair(saddr, daddr);
    //string key = convertKeyValue(saddr, daddr);
    string key = convertKeyValue(fid);  // Peixuan 04212020
    //Flow_pl* newFlowPtr = new Flow_pl(1, weight, brustness);
    //this->flowMap.insert(pair<pair<ns_addr_t, ns_addr_t>, Flow_pl*>(key, newFlowPtr));
    this->flowMap.insert(pair<string, Flow_pl*>(key, flowPtr));
    //flowMap.insert(pair(key, newFlowPtr));
    //return 0;
    return 0;
}

//string PIFOGearbox_pl::convertKeyValue(nsaddr_t saddr, nsaddr_t daddr) {
string PIFOGearbox_pl::convertKeyValue(int fid) { // Peixuan 04212020
    stringstream ss;
    //ss << saddr;
    //ss << ":";
    //ss << daddr;
    ss << fid;  // Peixuan 04212020
    string key = ss.str();
    return key; //TODO:implement this logic
}

