/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 * Copyright V2 Systems, LLC. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include "assetMgr.h"
#include "TSUtilities.h" //Tylor

using namespace std;
using namespace ajn;
using namespace qcc;

//LR Hack, making these global, a girl can only do so much coding
extern BusAttachment bus;
extern Observer* obs;

//Tylor v
// ReadPriceList
// * read csv into a single deliminated string
// * convert deliminated string into number matrix write to member variable
void AssetMgr::ReadPriceList (const string kFilename) {
    const char kDelimiter = ',';
    vector<string> file_string;
    tsu::ReadFile(kFilename, kDelimiter, file_string);

    //convert vector of strings to number matrix
    const unsigned int kColumns = 4;  // defined by file
    const bool kHeader = true;
    tsu::VectorToMatrix(file_string, kColumns, kHeader, price_list_);
    tsu::PrintMatrix(price_list_);
} // end ReadPriceList

void AssetMgr::GetPriceFrequency() {
    const int kTime_column = price_list_[0].size() - 2;
    //Tess v
    //const int kTimehr_column = price_list_[0].size();
    //const int kTimemin_column = price_list_[0].size() - 1;
    //Tess ^
    const unsigned int time_start = price_list_[1][kTime_column];
    tsu::Print(time_start);
    const unsigned int time_next  = price_list_[2][kTime_column];
    tsu::Print(time_next);
    price_frequency_ = time_next - time_start; //This is a period, not frequency?
} // end GetPriceFrequency

// UpdatePriceList
// * add a days worth of UTC time to each price time value to continue testing
void AssetMgr::UpdatePriceList () {
    tsu::Print("\n*** Updating Real Time Price List ***\n");
    const int kSeconds = 24*60*60;  // hours * minutes * seconds
    const int kColumn = 2;  // UTC-Time column
    unsigned int time_new;
    for (unsigned int i = 0; i < price_list_.size(); i++) {
        time_new = price_list_[i][kColumn]  + kSeconds;
        price_list_[i][kColumn] = time_new;
    }
} // end UpdatePriceList

// UpdateProperties
void AssetMgr::UpdateProperties (const unsigned int kRow) {
    const int kTime_column = price_list_[0].size() - 2;
    const int kPrice_column = price_list_[0].size() - 1;
    current_time_utc_ = price_list_[kRow][kTime_column];
    //Tess v
    //current_hr_=price_list_[kRow][kTimehr_column];
    //current_min_=price_list_[kRow][kTimemin_column];
    //Tess ^
    current_price_ = price_list_[kRow][kPrice_column];
} // end UpdateProperties

// ControlLoop
// * move through price list until end and update for next day
void AssetMgr::ControlLoop () {
    if (time_index_ < price_list_.size()) {
        AssetMgr::UpdateProperties (time_index_);
        time_index_ = time_index_ + 1;
    } else {
        time_index_ = 0;
        AssetMgr::UpdatePriceList ();
    }
} // end ControlLoop

/* Initialize the Asset Manager
*/
QStatus AssetMgr::Init(int& multiplier,time_t& emsTime, string& pfilename) {
    QStatus status = ER_OK;

    emsName = "ELAREMS";
    cout << "\tUnit Name: " << emsName << endl;

    emsTimeValues.multiplier = multiplier;
    cout << "\tTime Scale (ratio of seconds to wall clock seconds): " << emsTimeValues.multiplier << endl;


    emsTimeValues.wallClockStart = time(0);
    emsTimeValues.emsClockStart = emsTime;

    cout << "\tCurrent EMS Time: " << asctime(localtime(&emsTime)) << endl;

    //Default current price
    currentPrice = 2.5;
    if (!pfilename.empty()) {
        cout << "\tPrice Input File: " << pfilename << endl;
        AssetMgr::ReadPriceList(pfilename);
        AssetMgr::GetPriceFrequency();
        time_index_ = 0;
    }

    //cout << "\tCurrent Price: "<< currentPrice << endl;
    return status;
}
/************************************************************
* Entry point from main into the Asset Manager
* Called at each 1 second time step
***************************************************/
void AssetMgr::TimeStep() {
    //TODO: Entry point Asset Manager
    //Decisions made here are a) obtain RT price value, b) distribute price to EV agents?, c) retrieve power status info
    //(note it is cached),d) sum up total load.


}
// Returns the time for the model
time_t AssetMgr::GetModelTime(){

    time_t now;
    time(&now);

    time_t mnow;

    int deltasecs = (difftime(now,emsTimeValues.wallClockStart)) * emsTimeValues.multiplier;
    mnow = emsutils::AddTime(emsTimeValues.emsClockStart,deltasecs);

    return mnow;
}


string AssetMgr::GetEMSName(){

    return emsName;
}

void AssetMgr::PrintEMSStatus(){
    cout << "\tName: \t" << emsName << endl;
    time_t t = GetModelTime();
    cout << "\tTime: \t" << asctime(localtime(&t)) << endl;
    cout << "\tPrice: \t" << current_price_ << endl;  //Tess
}

void AssetMgr::Shutdown() {
//TODO: Finalize any data output and close files
}

//Pretty prints an array of properties
void AssetMgr::PrintAssetProperties(vector<properties> props) {
    for (size_t i = 0 ; i < props.size(); i++) {
        cout << "\tAssetName: " << props[i].assetname << endl;
        cout << "\t\tbatteryStatus: " << (int)props[i].batteryStatus <<endl;
        cout << "\t\tchangedTime: " << emsutils::DateTime(props[i].changedTime) << endl;
        cout << "\t\tchargingPowerNow: " << props[i].chargingPowerNow << endl;
        cout << "\t\tenergyRequestNow: " << props[i].energyRequestNow << endl;
    }
}


/*********************************************************
* AllJoyn Interface Support
*********************************************************/

// Called by AllJoyn Interface if it receives a properties changed signal
void AssetMgr::PowerStatusChange() {
    //TODO Determine if an action is takenhere
}

// Returns the properties for all of the assets that are connected
vector<properties> AssetMgr::GetAssetProperties() {

     vector<properties> props;

     ProxyBusObject proxy = obs->GetFirst();

     if (!proxy.IsValid()) {
        cout << "\tNo valid asset connected" << endl;
        return props;
     }

     properties p;
     for (; proxy.IsValid(); proxy = obs->GetNext(proxy)) {
        AssetProxy asset(proxy,bus);
        string assetName;
        if (ER_OK != asset.GetAssetName(assetName)) {
            cerr << "\tCould not retrieve asset Name" << endl;
        } else {
            p.assetname = assetName;
        }

        MsgArg telemetry;
        if (ER_OK != asset.GetTelemetry(telemetry)) {
            cerr << "\tCould not retrieve asset Telemetry" << endl;
            continue;
        }
        // process telemetry
        size_t nelem = 0;
        MsgArg* elems = NULL;

        if (ER_OK != telemetry.Get("a{sv}", &nelem, &elems)) {
           cerr << "Error in getting telemetry elements" << endl;
           continue;
        }

        // Telemetry is valid, assign to properties structure
        char* name;
        uint32_t uint32_value;
        uint8_t uint8_value;
        int32_t int32_value;

        for (size_t i = 0; i < nelem; i++) {
            if (ER_OK == elems[i].Get("{su}", &name, &uint32_value)) {
                if (!strcmp(name,"changedTime")) {
                    p.changedTime = uint32_value;
                    continue;
                }
            }
            if (ER_OK == elems[i].Get("{si}", &name, &int32_value)) {
                 if (!strcmp(name,"chargingPowerNow")) {
                    p.chargingPowerNow = int32_value;
                    continue;
                }
                if (!strcmp(name,"energyRequestNow")) {
                    p.energyRequestNow = int32_value;;
                    continue;
                }
            }
           if (ER_OK == elems[i].Get("{sy}", &name, &uint8_value)) {
                if (!strcmp(name,"batteryStatus")) {
                    p.batteryStatus = uint8_value;;
                    continue;
                }
            }
            cout << "unknown telemetry value" << name << endl;
            }
        props.push_back(p);
      }
    return props;
}

void AssetMgr::SendProperties() {
    QStatus status = ER_OK;

    QCC_UNUSED(status);
    status = SendUpdatedPropertyNotification("price");
    tsu::Print("price has been updated.");
}



