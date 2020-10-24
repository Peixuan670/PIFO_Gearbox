#include "Level_pt.h"
#include "Flow_pl.h"
#include <vector>
#include <string>

#include <map>

using namespace std;

class PIFOGearbox_plClass : public Queue {
private:
    static const int DEFAULT_LEVLES = 3;
    static const int DEFAULT_WEIGHT = 2;         // 01032019 Peixuan default weight
    static const int DEFAULT_BRUSTNESS = 1000;    // 01032019 Peixuan default brustness
    static const int TIMEUNIT = 1;    // 01032019 Peixuan default brustness
    int levels;                     // num of levels in scheduler
    int currentRound;           // current Round
    int pktCount;           // packet count

    Level_pt levels[3];
    //Level_pt levelsB[2];       // Back up Levels    // Peixuan 10232020 Not using

    //Level_pt hundredLevel;  // Peixuan 10232020 Not using
    //Level_pt decadeLevel;   // Peixuan 10232020 Not using

    //Level_pt decadeLevelB;     // Back up Levels // Peixuan 10232020 Not using

    //bool level0ServingB;          // is serve Back up Levels // Peixuan 10232020 Not using
    //bool level1ServingB;          // is serve Back up Levels // Peixuan 10232020 Not using

    //06262019 Peixuan
    //vector<Packet*> pktCurRound;    // Peixuan 10232020 Not using

    // 06262019 Peixuan
    //vector<Packet*> runRound(); // Peixuan 10232020 Not using
    vector<Packet*> serveUpperLevel(int);
    void setPktCount(int);

    //12132019 Peixuan
    typedef std::map<string, Flow_pl*> FlowMap;
    FlowMap flowMap;

    Flow_pl* getFlowPtr(int fid);   // Peixuan 04212020
    Flow_pl* insertNewFlowPtr(int fid, int weight, int brustness);   // Peixuan 04212020

    //int updateFlowPtr(nsaddr_t saddr, nsaddr_t daddr, Flow_pl* flowPtr);
    int updateFlowPtr(int fid, Flow_pl* flowPtr);    // Peixuan 04212020

    //string convertKeyValue(nsaddr_t saddr, nsaddr_t daddr);
    string convertKeyValue(int fid);    // Peixuan 04212020

    // Peixuan 10222020 PIFO

    //int reloadLevel(Level_pt level)
    //int migration(Level_pt level)

    //int reloadLevel_deq(Level_pt level, int spdFactor)
    //int migration_deq(Level_pt level, int spdFactor)

    int spdUpFactor;


public:
    PIFOGearbox_plClass();
    explicit PIFOGearbox_plClass(int);
    void enque(Packet*);
    Packet* deque();
    void setCurrentRound(int);
    int cal_theory_departure_round(hdr_ip*, int);
    int cal_insert_level(int, int);

    

};
