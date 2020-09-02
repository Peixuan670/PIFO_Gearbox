#include <cmath>
#include <sstream>

#include "PQ_pl.h"

static class PQ_plClass : public TclClass {
public:
        PQ_plClass() : TclClass("Queue/PQPL") {}
        TclObject* create(int, const char*const*) {
            fprintf(stderr, "Created new TCL PIFO instance\n"); // Debug: Peixuan 07062019
	        return (new PQ_pl);
	}
} class_PQ;

PQ_pl::PQ_pl():PQ_pl(DEFAULT_VOLUME) {
    fprintf(stderr, "Created new PIFO instance\n"); // Debug: Peixuan 07062019
}

PQ_pl::PQ_pl(int volume) {
    fprintf(stderr, "Created new PIFO instance with volumn = %d\n", volume); // Debug: Peixuan 07062019
    this->volume = volume;
    //flows.push_back(Flow_pl(0, 2, 100));
    //flows.push_back(Flow_pl(1, 2, 100));
    //flows.push_back(Flow_pl(2, 2, 100));
    //flows.push_back(Flow_pl(3, 2, 100));
    //flows.push_back(Flow_pl(4, 20, 1000));
    //flows.push_back(Flow_pl(5, 20, 1000));
    //flows.push_back(Flow_pl(6, 200, 1000));
    //flows.push_back(Flow(1, 0.2));
    // Flow(1, 0.2), Flow(2, 0.3)};
    currentRound = 0;
    pktCount = 0; // 07072019 Peixuan
    //pktCurRound = new vector<Packet*>;

    //12132019 Peixuan
    typedef std::map<string, Flow_pl*> FlowMap;
    FlowMap flowMap;
}

void PQ_pl::setCurrentRound(int currentRound) {
    fprintf(stderr, "Set Current Round: %d\n", currentRound); // Debug: Peixuan 07062019
    this->currentRound = currentRound;
}

void PQ_pl::setPktCount(int pktCount) {
    fprintf(stderr, "Set Packet Count: %d\n", pktCount); // Debug: Peixuan 07072019
    this->pktCount = pktCount;
}

void PQ_pl::enque(Packet* packet) {

    hdr_ip* iph = hdr_ip::access(packet);
    int pkt_size = packet->hdrlen_ + packet->datalen();

    ///////////////////////////////////////////////////
    // TODO: get theory departure Round
    // You can get flowId from iph, then get
    // "lastDepartureRound" -- departure round of last packet of this flow
    int departureRound = cal_theory_departure_round(iph, pkt_size);
    ///////////////////////////////////////////////////

    // 20190626 Yitao
    /* With departureRound and currentRound, we can get the insertLevel, insertLevel is a parameter of flow and we can set and read this variable.
    */

    //int flowId = iph->flowid();
    //int insertLevel = flows[flowId].getInsertLevel(); //HCS->AFQ

    // Mengqi
    //int flowId = iph->flowid();
    string key = convertKeyValue(iph->saddr(), iph->daddr());
    // Not find the current key
    if (flowMap.find(key) == flowMap.end()) {
        //flowMap[key] = Flow_pl(iph->saddr, iph->daddr, 2, 100);
        //insertNewFlowPtr(iph->saddr(), iph->daddr(), 2, 100);
        insertNewFlowPtr(iph->saddr(), iph->daddr(), DEFAULT_WEIGHT, DEFAULT_BRUSTNESS);
    }

    Flow_pl* currFlow = flowMap[key];

    int insertLevel = 0;

    departureRound = max(departureRound, currentRound);

    /*

    if ((departureRound - currentRound) >= SET_GRANULARITY * SET_NUMBER) {
        fprintf(stderr, "?????Exceeds maximum round, drop the packet from Flow %d\n", iph->saddr()); // Debug: Peixuan 07072019
        drop(packet);
        return;   // 07072019 Peixuan: exceeds the maximum round
    }
    */

    //int curFlowID = iph->saddr();   // use source IP as flow id
    //int curFlowID = iph->flowid();   // use flow id as flow id
    int curBrustness = currFlow->getBrustness();
    if ((departureRound - currentRound) >= curBrustness) {
        drop(packet);
        return;   // 07102019 Peixuan: exceeds the maximum brustness
    }

    // flows[curFlowID].setLastDepartureRound(departureRound);     // 07102019 Peixuan: only update last packet finish time if the packet wasn't dropped
    currFlow->setLastDepartureRound(departureRound);
    this->updateFlowPtr(iph->saddr(), iph->daddr(),currFlow);

    struct Unit unit;

    unit.packet = packet;
    unit.finishTime = departureRound;
    unit.id = globalcount++;
    pq.push(unit);
    fprintf(stderr, "MengqiN Enqueue Debug flowid: %d pkg id: %d finishtime: %d\n", iph->saddr(), unit.id, unit.finishTime);

    setPktCount(pktCount + 1);
}

// Peixuan: This can be replaced by any other algorithms
int PQ_pl::cal_theory_departure_round(hdr_ip* iph, int pkt_size) {
    //int		fid_;	/* flow id */
    //int		prio_;
    // parameters in iph
    // TODO

    // Peixuan 06242019
    // For simplicity, we assume flow id = the index of array 'flows'

    fprintf(stderr, "$$$$$Calculate Departure Round at VC = %d\n", currentRound); // Debug: Peixuan 07062019

    //int curFlowID = iph->saddr();   // use source IP as flow id
    //int curFlowID = iph->flowid();   // use flow id as flow id
    //float curWeight = flows[curFlowID].getWeight();
    //int curLastDepartureRound = flows[curFlowID].getLastDepartureRound();
    string key = convertKeyValue(iph->saddr(), iph->daddr());
    Flow_pl* currFlow = this->getFlowPtr(iph->saddr(), iph->daddr());

    float curWeight = currFlow->getWeight();
    int curLastDepartureRound = currFlow->getLastDepartureRound();
    int curStartRound = max(currentRound, curLastDepartureRound);

    fprintf(stderr, "$$$$$Last Departure Round of Flow%d = %d\n",iph->saddr() , curLastDepartureRound); // Debug: Peixuan 07062019
    fprintf(stderr, "$$$$$Start Departure Round of Flow%d = %d\n",iph->saddr() , curStartRound); // Debug: Peixuan 07062019

    //int curDeaprtureRound = (int)(curStartRound + pkt_size/curWeight); // TODO: This line needs to take another thought

    int curDeaprtureRound = (int)(curStartRound + curWeight); // 07072019 Peixuan: basic test

    fprintf(stderr, "$$$$$Calculated Packet From Flow:%d with Departure Round = %d\n",iph->saddr() , curDeaprtureRound); // Debug: Peixuan 07062019
    // TODO: need packet length and bandwidh relation
    //flows[curFlowID].setLastDepartureRound(curDeaprtureRound);
    return curDeaprtureRound;
}

//06262019 Peixuan deque function of Gearbox:

//06262019 Static getting all the departure packet in this virtual clock unit (JUST FOR SIMULATION PURPUSE!)

Packet* PQ_pl::deque() {

    //fprintf(stderr, "Queue size: %d\n",pktCurRound.size()); // Debug: Peixuan 07062019

    if (pktCount == 0) {
        fprintf(stderr, "Scheduler Empty\n"); // Debug: Peixuan 07062019
        return 0;
    }

    struct Unit unit = pq.top();
    pq.pop();
    Packet* p = unit.packet;

    this->setCurrentRound(unit.finishTime);

    setPktCount(pktCount - 1);
    hdr_ip* iph = hdr_ip::access(p);
    fprintf(stderr, "MengqiN Debug flowid: %d pkg id: %d finishtime: %d\n", iph->saddr(), unit.id, unit.finishTime);
    return p;

}

// Peixuan: now we only call this function to get the departure packet in the next round
vector<Packet*> PQ_pl::runRound() {

    fprintf(stderr, "Run Round\n"); // Debug: Peixuan 07062019

    vector<Packet*> result;

    int curServeSet = (currentRound / SET_GRANULARITY) % SET_NUMBER;    // Find the current serving set

    fprintf(stderr, "Serving Set %d\n", curServeSet); // Debug: Peixuan 08022019

    Packet* p = levels[curServeSet].deque();

    //fprintf(stderr, "Get packet pointer\n"); // Debug: Peixuan 07062019

    if (!p) {
        fprintf(stderr, "No packet\n"); // Debug: Peixuan 07062019
    }

    while (p) {

        hdr_ip* iph = hdr_ip::access(p);                   // 07092019 Peixuan Debug

        fprintf(stderr, "^^^^^At Round:%d, Round Deque Flow %d Packet From Level %d: fifo %d\n", currentRound, iph->saddr(), curServeSet, levels[curServeSet].getCurrentIndex()); // Debug: Peixuan 07092019

        result.push_back(p);
        p = levels[curServeSet].deque();
    }

    levels[curServeSet].getAndIncrementIndex();               // Level 0 move to next FIFO
    fprintf(stderr, "<<<<<At Round:%d, Level %d update current FIFO as: fifo %d\n", currentRound, curServeSet, levels[curServeSet].getCurrentIndex()); // Debug: Peixuan 07212019

    return result;
}
// 12132019 Peixuan
Flow_pl* PQ_pl::getFlowPtr(nsaddr_t saddr, nsaddr_t daddr) {
    fprintf(stderr, "Getting flows with src address %d , dst address = %d\n",saddr, daddr); // Debug: Peixuan 12142019
//int hierarchicalQueue_pl::getFlowPtr(ns_addr_t saddr, ns_addr_t daddr) {
    //pair<ns_addr_t, ns_addr_t> key = make_pair(saddr, daddr);
    //FlowMap::const_iterator iter; 
    //iter = this->flowMap.find(make_pair<ns_addr_t, ns_addr_t>);
    //typedef std::map<pair<ns_addr_t, ns_addr_t>, Flow_pl*> FlowMap;
    //FlowMap::const_iterator iter = this->flowMap.find(key);
    //return iter->second;
    //return this->flowMap[key];
    //Flow_pl* flow = this->flowMap[key];
    //printf("Map size: %d",this->flowMap.size());
    string key = convertKeyValue(saddr, daddr);
    Flow_pl* flow; 
    if (flowMap.find(key) == flowMap.end()) {
        flow = this->insertNewFlowPtr(saddr, daddr, DEFAULT_WEIGHT, DEFAULT_BRUSTNESS);
    }
    flow = this->flowMap[key];
    return flow;
}

Flow_pl* PQ_pl::insertNewFlowPtr(nsaddr_t saddr, nsaddr_t daddr, int weight, int brustness) {
    string key = convertKeyValue(saddr, daddr);
    Flow_pl* newFlowPtr = new Flow_pl(1, weight, brustness);
    this->flowMap.insert(pair<string, Flow_pl*>(key, newFlowPtr));
    return this->flowMap[key];
}

int PQ_pl::updateFlowPtr(nsaddr_t saddr, nsaddr_t daddr, Flow_pl* flowPtr) {
    //pair<ns_addr_t, ns_addr_t> key = make_pair(saddr, daddr);
    string key = convertKeyValue(saddr, daddr);
    //Flow_pl* newFlowPtr = new Flow_pl(1, weight, brustness);
    //this->flowMap.insert(pair<pair<ns_addr_t, ns_addr_t>, Flow_pl*>(key, newFlowPtr));
    this->flowMap.insert(pair<string, Flow_pl*>(key, flowPtr));
    //flowMap.insert(pair(key, newFlowPtr));
    //return 0;
    return 0;
}

string PQ_pl::convertKeyValue(nsaddr_t saddr, nsaddr_t daddr) {
    stringstream ss;
    ss << saddr;
    ss << ":";
    ss << daddr;
    string key = ss.str();
    return key;
}

