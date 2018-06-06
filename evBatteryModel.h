#ifndef EVBATTERYMODEL_INC
#define EVBATTERYMODEL_INC


#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <iomanip>

#include <alljoyn/Status.h>
#include <alljoyn/Init.h>
#include <alljoyn/BusAttachment.h>

using namespace std;
using namespace ajn;
using namespace qcc;

#define UNKNOWN 0
#define CHARGE 1
#define IDLE 2
#define DISCHARGE 3
#define NOTAPPLICABLE 4
#define MAXCHARGE 85

struct battState {
    time_t updateTime;
    uint8_t mode;
    uint16_t SOC;
    int32_t  activePower;
    float realEnergy; //Tess
    //int32_t realEnergy;
};


class EVBattery {

    battState chgStatus;
    string evMake;
    int sizeInkWh;
    int whPerMi;
    int capInAh;
    int chargerAmps;
    int timeScale;
    void UpdateCharge();

    public :

        QStatus Init(map<string,string>&,int&);
        int Drive(int&);
        void SetState(int&);
        battState GetState();
        tuple<battState,int,int> QueryChg(int&);
        void Shutdown();

};

#endif //EVBATTERYMODEL_INC
