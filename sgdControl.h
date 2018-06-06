#ifndef SGDCONTROL_INC
#define SGDCONTROL_INC

#include <iostream>
#include <fstream>
#include <map>
#include <ctime>

#include <alljoyn/Status.h>
#include <alljoyn/Init.h>
#include <alljoyn/BusAttachment.h>

#include "seInterface.h"
#include "evUtils.h"
#include "evBatteryModel.h"

using namespace std;
using namespace ajn;
using namespace qcc;

#define MAX_TELEMETRY_VALUES 50
#define NO_HEADER 0
#define HEADER 1

struct etime {
    time_t wallClockStart;
    time_t evClockStart;
    int multiplier;
};

struct drive_t {
    time_t dep;
    time_t ret;
    int dist;
    int multdist; //Tess
};

struct pevStatus {
    int16_t  chargingPowerNow;
    long int energyRequestNow;
    long int minChargeDurationInSecs;
    int targetSOC;
    time_t timeChargeIsNeeded;
    };

struct pwrStatus {
    time_t changedTime;
    int mode;
    int SOC;
    pevStatus pevInfo;
    };

class SgdControl {

      bool pluggedIn;

      int tripIdx;
      drive_t tripInfo;

      etime evTimeValues;
      //TODO create structure for day ahead prices
      vector< vector<int>> drivingPatterns;
      //Jose:
      vector<vector<int>>dayaheadprices;
      //Jose^
      int defaultPluggedInState;
      EVBattery evbattery;

      pwrStatus GetPwrState();
      drive_t GetTrip(time_t);

    public:
        MsgArg telemetry[MAX_TELEMETRY_VALUES];
        string evName;
        int optIn; //Annie
        int fixedPriceTimesTen; //Annie
        int dayaheadpricegiven; //Jose
        int realtimepricegiven; //Jose
        int previousdayaheadpricegiven; //Jose
        int previousrealtimepricegiven; //Jose
        int CREAM; //Jose
        int current_hour; //Jose
        double fixedPrice; //Annie




        float price;

        QStatus Init(map<string,string>&,int&,string&,string&);
        void TimeStep();
        void SetSGDState(char&);
        void DisplayPwrState();
        tuple<size_t,bool> Get_Telemetry();
        void UpdatePrice();
        void SyncTime();
        time_t GetModelTime(time_t);
        void PriceChanged(float);
        void SendProperties();
        void Shutdown();
};
# endif // SGDCONTROL_INC
