#ifndef ASSETMGR_INC
#define ASSETMGR_INC

#include <ctime>

#include <alljoyn/Status.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/Observer.h>
#include <alljoyn/Init.h>

#include "seInterface.h"
#include "emsUtils.h"
#include "TSUtilities.h"

using namespace std;
using namespace ajn;
using namespace qcc;


#define MAX_INTERVALS 1

#define INTF_EDEV "edu.pdx.powerlab.sep.edev"

struct etime {
    time_t wallClockStart;
    time_t emsClockStart;
    int multiplier;
};

struct properties {
    string assetname;
    uint8_t batteryStatus;
    time_t changedTime;
    int32_t chargingPowerNow;
    int32_t energyRequestNow;
};

class AssetMgr {
    etime emsTimeValues;
    int    tmult;

public:
    // variables
    unsigned int price_frequency_;
    vector<vector<int>> price_list_;
    vector<vector<int>> actual_price_list_;
    int current_price_;  //Tess changed unsigned to signed
    unsigned int current_time_utc_;
    unsigned int current_hour_;
    unsigned int current_min_;
    unsigned int time_index_;

    string emsName;
    float currentPrice;

public:
    // functions
    void ReadPriceList (const string kFilename);
    void UpdatePriceList ();
    void UpdateProperties (const unsigned int kRow);
    void ControlLoop ();
    void GetPriceFrequency();

    string GetEMSName();
    time_t GetModelTime();
    void PrintEMSStatus();
    QStatus Init(int&,time_t&,string&);
    vector<properties> GetAssetProperties();
    void PrintAssetProperties(vector<properties>);
    void TimeStep();
    void PowerStatusChange();
    void SendProperties();
    void Shutdown();
};

#endif //ASSETMGR_INC

