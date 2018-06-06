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
#include "sgdControl.h"

//LR Hack, making these global, a girl can only do so much coding
extern BusAttachment bus;
extern Observer* obs;
//extern custEV* evModule;

/* Initialize the smart grid device controller*/
/*    initValues is a map containing the parsed values from the initialization file */
QStatus SgdControl::Init(map<string,string>& initValues,int& mult,string& dfile, string& pfile) {
    QStatus status = ER_OK;
    map<string,string>::iterator it;
    vector<vector<int>> intValues;

    cout << "\tSmart Energy Device Init" << endl;

    evTimeValues.multiplier = mult;
    clog << "\tTime Scale (ratio of seconds to wall clock seconds): " << evTimeValues.multiplier << endl;
    SyncTime();

    it = initValues.find("SGDName");
    if (it != initValues.end()) {
        evName = it->second.c_str();
        clog << "\tUnit Name: " << evName << endl;
    }

    defaultPluggedInState = CHARGE;
    it = initValues.find("defaultPluggedInState");
    if (it != initValues.end()) {
        defaultPluggedInState = atoi(it->second.c_str());
    }
    clog << "\tDefault Plugged in State (1=CHARGE, 2=IDLE): " << defaultPluggedInState << endl;

    // Initialize driving patterns
    if (!dfile.empty()) {
        clog << "\tInitializing driving patterns from: " << dfile << endl;
        drivingPatterns = evutils::LoadFromCSVInt(dfile,HEADER);  //Tess - need to change this in bash to use others
        if ((drivingPatterns.size() == 0)) {
            cerr << "Unable to initialize driving patterns from: " << dfile << endl;
            status = ER_FAIL;
        }
    }

    //TODO: Initialize day ahead pricing values
    if (!pfile.empty()){
        clog << "\tInitializing day ahead prices from: " << pfile << endl;
        dayaheadprices=evutils::LoadFromCSVInt(pfile, HEADER); //Jose
    } else clog << "\tDay ahead prices not available" <<endl;

    //price=fixedPrice;  //Annie
    // Initialize first trip
//Jose's code:
size_t i;
size_t x;

int dayaheadpricehour[24];
int dayaheadprice[24];
int orderedDayAhead[24];//Tess

for(i=0; i < dayaheadprices.size(); i++) {
    //cout<<"ROW "<<i<<":";
    dayaheadpricehour[i]=dayaheadprices[i][0];
    dayaheadprice[i]=dayaheadprices[i][2];
    ordererDayAhead[i]=dayaheadprices[i][2];//Tess
    for(x=0;x< dayaheadprices[i].size(); x++){}
    }
int medIndex=dayaheadprices.size()/2;//Tess
for(int j=0; j<dayaheadprices.size(); j++){
    for(int k=0; k<dayaheadprices.size(); k++){
        if(orderedDayAhead[k]>=orderedDayAhead[k+1]){
            int temp=orderedDayAhead[k+1]
            orderedDayAhead[k+1]=orderedDayAhead[k]
            orderedDayAhead[k]=temp
        }
    }
}
cout<<"Sorted day ahead prices:";
	for(int i=0; i<dayaheadprices.size(); i++)
	{
		cout<<i+1<<") "<<orderedDayAhead[i]<<endl;
	}
float medianprice=dayaheadprice[medIndex]
    //TODO: Set initial price
    price = medianprice;  //Tess
 //Jose code ^ + Tess medianprice stuff, fixed realtimeprice typo -Tess

    tripInfo = GetTrip(evTimeValues.evClockStart);
    // Initialize EV Battery Model //
    status = evbattery.Init(initValues,mult);
    pluggedIn = true;
    evbattery.SetState(defaultPluggedInState);

    return status, medianprice; //Tess
}

void SgdControl::Shutdown() {
//TODO: Print any final output or summaries and cLose any open files,
}

/* Entry point from main into the SGD Controller */
void SgdControl::TimeStep() {

   time_t mnow;
   int nextEVState;
   battState bState;
   uint16_t tSOC;
   int SOC;
   int minChgTime;

   time_t now = time(0);
   mnow = GetModelTime(now);

   //TODO Entry point into charger, called every second
   // Decisions made here are a) get price?, b) if plugged in, charge or discharge based on consumer input, battery status,
   // and next trip, 3) calculate average hourly price (or getdata), 4) measure hourly kwH usage, 5) calculate
   // customer cost, 6) update EMS power Status information.

pwrStatus chargingstatus;  //creating an instance of power status called charging status
chargingstatus = GetPwrState();
int drivinghour;
drivinghour=evutils::AddTime2(mnow);

dayaheadpricegiven=dayaheadprice[drivinghour];


//int realtimeprice[480];//Tess
//int drivingmin; //Tess
drivingmin=evutils::AddTime2(mnow);
//realtimepricegiven=realtimeprice[drivingmin];//Tess
//	previousdayaheadpricegiven=dayaheadprice[drivinghour];
//        previousrealtimepricegiven=realtimeprice[drivinghour];
//Jose code

   // Update the battery and the garaged status based on trip information
   if (difftime(mnow,tripInfo.dep) < 0) {
        return;
   }
  // Starting a new trip
   if ((difftime(mnow,tripInfo.dep) >= 0) &&
       (difftime(mnow, tripInfo.ret) < 0) &&
       (pluggedIn == true)) {
          pluggedIn = false;
          clog << "Departing: " << evutils::DateTime(mnow);
          clog << "\tDistance: " << tripInfo.dist;
          tie(bState,tSOC,minChgTime) = evbattery.QueryChg(tripInfo.multdist); //Tess changed dist to multdist

          clog << " Current SOC: " << bState.SOC;
          clog << "\tMinimum % needed: " << tSOC << endl;
          if (tSOC > bState.SOC) {
              cerr << "Insufficient SOC for trip" << endl;
          }
          nextEVState = UNKNOWN;
          evbattery.SetState(nextEVState);
          SendProperties();
          return;
          }

    // Just returned from trip
    if ((difftime(mnow,tripInfo.ret) >= 0) &&
           (pluggedIn == false)) {
        // Discharge the battery based on Miles
        SOC = evbattery.Drive(tripInfo.dist);
        clog << "Returning: " << evutils::DateTime(mnow) << "Battery SOC: " << SOC << endl;
        // get information for the next trip
        tripInfo = GetTrip(mnow);

        if (defaultPluggedInState == CHARGE){
            nextEVState = CHARGE;
        }
        else nextEVState = IDLE;

        pluggedIn = true;
        evbattery.SetState(nextEVState);
        SendProperties();
    }
}


/* Handles state change commands after parsing from CLI
*/
void SgdControl::SetSGDState(char& state) {
    int nextEVState;

    if (pluggedIn == true) {
        switch (state) {

        case 'c':
            nextEVState = CHARGE;
            clog << "Setting battery to CHARGE"<< endl;
            evbattery.SetState(nextEVState);
            break;
        case 'i':
            nextEVState = IDLE;
            clog << "Setting battery to IDLE" << endl;
            evbattery.SetState(nextEVState);
            break;
        case 'd':
            nextEVState = DISCHARGE;
            clog << "Setting battery state to DISCHARGE" << endl;
            evbattery.SetState(nextEVState);
            break;
        default:
            cout << "Unknown battery state requested, no change to current battery state: " << state << endl;
        }
    } else cout << "EV unavailable to change state" << endl;
}

//Converts an entry in driving patterns to a trip start time,return time and distance
drive_t SgdControl::GetTrip(time_t now)
{
     drive_t x;
     int deltaSecs;

     x.dep = 0; x.ret= 0; x.dist =  0; x.multdist=0; //Tess
     // test if this vehicle is garaged
     if (drivingPatterns.empty()) return x;

     // Search for next departure time row
     // First column in driving patterns is departure hour, second column is minutes
     size_t idx = 0;
     time_t tmptime;
     int daysecs = 24*60*60;
     int secs = 0;
     for (size_t i=0; i < drivingPatterns.size(); i++) {
          if (drivingPatterns[i].size() < 4) {
            cerr << "Driving pattern invalid at row: " << i << endl;
            return x;
          }
          idx = i;
          tmptime = evutils::SetTime(now,drivingPatterns[i][0],drivingPatterns[i][1],secs);
          cout<<tmptime<<endl;  //Jose
          if (((difftime(now, tmptime)) < 0) &&
              (idx == 0)) {
            //less than the first entry in the patterns
            break;
          } else { // The current time is greater than the current row
              idx = (i+1) % drivingPatterns.size();
              tmptime = evutils::SetTime(now,drivingPatterns[idx][0],drivingPatterns[idx][1],secs);
              if (difftime(now,tmptime) < 0) {
                // less than the next row, found the correct one
                break;
              }
              if (idx == 0) { //greater than all, wraparound to first row and add a day
                  now = evutils::AddTime(now,daysecs);
                  tmptime = evutils::SetTime(now,drivingPatterns[0][0],drivingPatterns[0][1],secs);
                  break;
              }
          }
        }

     x.dep = tmptime;

     //Third entry is trip duration in minutes
     deltaSecs = (drivingPatterns[idx][2] * 60);
     x.ret = evutils::AddTime(x.dep,deltaSecs);

     //Fourth entry is distance
     x.dist = drivingPatterns[idx][3];
     //Tess Editv
     x.multdist=drivingPatterns[idx][3]+drivingPatterns[idx+1][3]+drivingPatterns[idx+2][3]
     //Tess Edit ^
     return x;
}

pwrStatus SgdControl::GetPwrState() {
    pwrStatus pState;

    time_t now = time(0);
    time_t mnow = GetModelTime(now);
    pState.changedTime= mnow;

    // If not plugged in, status cannot be determined
    if (pluggedIn == false) {
        pState.mode = UNKNOWN;
        return pState;
    }
/*#define UNKNOWN 0
#define CHARGE 1
#define IDLE 2
#define DISCHARGE 3
#define NOTAPPLICABLE 4*/

    battState batt;
    int tSOC;
    int minChgTime;
    // Get current battery status
    tie(batt,tSOC,minChgTime) = evbattery.QueryChg(tripInfo.multdist);  //Tess changed dist to multdist

    //Now convert to power status
    pState.mode = batt.mode;
    pState.SOC = batt.SOC;
    pState.pevInfo.chargingPowerNow = batt.activePower;
    pState.pevInfo.energyRequestNow = batt.realEnergy;
    pState.pevInfo.minChargeDurationInSecs = minChgTime;
    pState.pevInfo.targetSOC = tSOC;
    if (tripInfo.dep !=0) {
        pState.pevInfo.timeChargeIsNeeded = tripInfo.dep;
    } else pState.pevInfo.timeChargeIsNeeded= 0;

    return pState;

}

/* Updates and displays the SGD Power Status
*/
void SgdControl::DisplayPwrState() {
    pwrStatus evpwr;

    evpwr = GetPwrState();

    cout << "\tChanged Time:\t\t"<< evutils::DateTime(evpwr.changedTime) << endl;
    cout << "\tStatus (0 Unknown, 1 Charge, 2 Idle, 3 discharge):\t" << evpwr.mode << endl;
    if (evpwr.mode != UNKNOWN) {
        cout << "\tState of Charge:\t" << evpwr.SOC << endl;
        cout << "\tCharging Power Now:\t" << evpwr.pevInfo.chargingPowerNow << endl;
        cout << "\tEnergy Request Now:\t" << evpwr.pevInfo.energyRequestNow << endl;
        cout << "\tTarget State of Charge:\t" << evpwr.pevInfo.targetSOC << " (for miles " << tripInfo.dist << " )" << endl;
        cout << "\tMinimum Charge Duration:\t"<< evpwr.pevInfo.minChargeDurationInSecs << endl;
   //Jose:
        cout << "\tCurrent day ahead price:\t"<< dayaheadpricegiven << endl;
        cout << "\tCurrent Real Time Price:\t"<< realtimepricegiven << endl;
        cout << "\tPrevious Day Ahead Price:\t"<< previousdayaheadpricegiven << endl;
        cout << "\tPrevious Real Time price is:\t"<< CREAM << endl;
        cout << "\tTotal cost is:\t"<< CREAM << endl;
    //Jose^
        if (evpwr.pevInfo.timeChargeIsNeeded != 0) {
            cout << "\tTimeCharge Is Needed:\t" << asctime(localtime(&evpwr.pevInfo.timeChargeIsNeeded)) << endl;
        }
    }
}

// Returns the time for the model
// Converts a wall clock time value to model time
time_t SgdControl::GetModelTime(time_t w){


    int deltasecs = (difftime(w,evTimeValues.wallClockStart)) * evTimeValues.multiplier;
    time_t mnow = evutils::AddTime(evTimeValues.evClockStart,deltasecs);

    return mnow;
}

/*********************************************************
* AllJoyn Interface Support
*********************************************************/

/* Retrieves and returns device status in AllJoyn Interface format
*/
tuple<size_t,bool> SgdControl::Get_Telemetry() {

    bool updated = false;
    string sigValue = "";
    map<string,int>::iterator it;
    static MsgArg telValue;
    pwrStatus evPwr;

    evPwr = GetPwrState();

    // Returns an array of string value pairs based on PowerStatus
    size_t numValues = 0;
    string name;

    name= "batteryStatus";
    uint8_t mode;
    //Assign mode values according to SEP specfication
    if (evPwr.mode == UNKNOWN) {
        mode = UNKNOWN;
    }
    else mode = NOTAPPLICABLE;
    telValue.Set("y",mode);
    telemetry[numValues].Set("{sv}", name.c_str(), &telValue);
    telemetry[numValues].Stabilize();
    numValues++;

    unsigned long int t = static_cast<unsigned long int> (evPwr.changedTime);
    name = "changedTime";
    telValue.Set("u",t);
    telemetry[numValues].Set("{sv}", name.c_str(), &telValue);
    // Stabilize allows reuse of MsgArg telValue
    telemetry[numValues].Stabilize();
    numValues++;

    int32_t cPn = 0;
    int32_t eRn = 0;
    if (evPwr.mode != UNKNOWN) {
        cPn = evPwr.pevInfo.chargingPowerNow;
        eRn = evPwr.pevInfo.energyRequestNow;
    }

    name = "chargingPowerNow";
    telValue.Set("i",cPn);
    telemetry[numValues].Set("{sv}", name.c_str(), &telValue);
    // Stabilize allows reuse of MsgArg telValue
    telemetry[numValues].Stabilize();
    numValues++;

    name = "energyRequestNow";
    telValue.Set("i",eRn);
    telemetry[numValues].Set("{sv}", name.c_str(), &telValue);
    // Stabilize allows reuse of MsgArg telValue
    telemetry[numValues].Stabilize();
    numValues++;

    updated = true;

    return make_tuple(numValues,updated);
}

/* Obtains the price from EMS
*/
void SgdControl::UpdatePrice() {

    ProxyBusObject proxy = obs->GetFirst();
    //if connected to EMS, retrieves current price

    if (proxy.IsValid()) {
            EMSProxy ems(proxy,bus);
            string name;
            ems.GetEMSName(name);
            if (ER_OK == ems.GetPrice(price)){
                cout << "\tPrice (retrieved from " << name << "): " << price << endl;
            }
    } else cout << "\tPrice unchanged (EMS unavailable): \t" << price << endl;
}

/* Called when a price update signal is received
*/
void SgdControl::PriceChanged(float p) {

    price=p;
    cout << "The ev has received price"<< p<< endl;
     //TODO A price update signal has been received, any processing of the signal goes here
     //Tess v
    // Add time function - update price every 5 secs
    /*int hrlysum[0]=0
    int hravg[0]=0
    int runsum[0]=0
    for (pricehrindex=1; priceindex<=25; priceindex++){
        for (priceminindex=1; priceminindex<=21; priceminindex++){
            hrlysum[priceminindex]=hrlysum[priceminindex-1]+price;
        }
         hravg[pricehrindex]= hrlysum[priceminindex]/priceminindex ;
         runsum[pricehrindex]=runsum[pricehrindex-1]+hravg[pricehrindex];
    }
    int dailysum=runsum[pricehrindex]
    cout << "The daily sum of energy cost is: " <<dailysum<<endl;*/

    if((price<medianprice)&&(chgstatus.mode!=UNKNOWN) {
        chgstatus.mode=CHARGE;}
    else if ((price>=medianprice)&&(chgstatus.mode!=UNKNOWN)){
        chgstatus.mode=IDLE;
    }
     //Tess ^
     //Jose v
//        pwrStatus chargingstatus;
//        chargingstatus = GetPwrState();
//if(chargingstatus.mode!=1){
//    price = p;
//    cout<<"price value overridden by ems"<<endl;  //Tess changed typo
//}
//else{
//cout<<"price value cannot be overridden, price charging is "<<chargingstatus.mode<<endl;
//}
//Jose ^
/* Obtains the time from the EMS and synchronizes the model
* This is always called when an EMS is discovered
*/
void SgdControl::SyncTime() {

    ProxyBusObject proxy = obs->GetFirst();

    evTimeValues.wallClockStart = time(0);
    evTimeValues.evClockStart = time(0);

    if (proxy.IsValid()) {
        EMSProxy ems(proxy,bus);
        //time_t t;
        string name;
        ems.GetEMSName(name);
        ems.GetTime(evTimeValues.evClockStart);
        cout << "\tSetting time (synchronizing with " << name << "): ";
    } else cout << "\tModel time unchanged (EMS unavailable): ";
    cout << evutils::DateTime(evTimeValues.evClockStart) << endl;

    // Get the next trip
    tripInfo = GetTrip(evTimeValues.evClockStart);
    pluggedIn = true;
}

void SgdControl::SendProperties() {
    QStatus status = ER_OK;

    QCC_UNUSED(status);
    status = SendUpdatedPropertyNotification("PowerStatus");
}
