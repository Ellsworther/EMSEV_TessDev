/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 * Copyright V2 Systems, LLC All rights reserved.
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
 #include "evBatteryModel.h"

/* Initialize the battery  */
/* initValues is a map containing the parsed values from the initialization file */
QStatus EVBattery::Init(map<string,string>& initValues, int& mult) {
    QStatus status = ER_OK;
    map<string,string>::iterator it;

    cout << "\tEV Battery Model Init" << endl;

    it = initValues.find("evMake");
    if (it != initValues.end()) {
        evMake = it->second.c_str();
    }

    it = initValues.find("sizeInkWh");
    if (it != initValues.end()) {
        sizeInkWh = atoi(it->second.c_str());
    }
    else status=ER_FAIL;

    it = initValues.find("whPerMi");
    if (it != initValues.end()) {
        whPerMi = atoi(it->second.c_str());
    }
    else status=ER_FAIL;

    if ((whPerMi <= 0) || (sizeInkWh <= 0))
        status = ER_FAIL;

    it = initValues.find("capInAh");
    if (it != initValues.end()) {
        capInAh = atoi(it->second.c_str());
    }
    else status = ER_FAIL;

    it = initValues.find("initialSOCinPercent");
    if (it != initValues.end()) {
        chgStatus.SOC = atoi(it->second.c_str());
    }
    else status=ER_FAIL;

    it = initValues.find("chargerAmps");
    if (it != initValues.end()) {
        chargerAmps = atoi(it->second.c_str());
    }
    else status=ER_FAIL;

    timeScale = mult;

    if (status == ER_FAIL){
        cerr << "Battery model initialization failure" << endl;
        return status;
    }

    chgStatus.mode = UNKNOWN;
    time_t now;
    chgStatus.updateTime = time(&now);

    cout << "\t\tEV Make: " << evMake  << endl;
    int range = (sizeInkWh * 1000)/whPerMi;
    cout << "\t\tBattSize: " << sizeInkWh << "\t wh/mile: " << whPerMi << "\tRange: " << range << endl;
    cout << "\t\tSOC: " << chgStatus.SOC << endl;
    cout << "\t\tCapacityInAmpHours: "<< capInAh << endl;
    cout << "\t\tChargerAmperage: " << chargerAmps << endl;

    return status;
}

void EVBattery::Shutdown() {

}

// Discharge the battery based on mile and return the new SOC
int EVBattery::Drive(int& miles) {

    float sizeInWh = sizeInkWh*1000;

    float d = whPerMi * miles;   //This miles uses x.dist, not mult.dist
    float e = (chgStatus.SOC * sizeInWh)/100;

    if (d > e) {
        chgStatus.SOC = 0;
        clog << "TRIP FAILED: battery capacity (Wh): " << e << ", drive requires (Wh): " << d << endl;
    }
    else chgStatus.SOC = ((e - d)/sizeInWh) * 100;

    time_t now;
    chgStatus.updateTime = time(&now);
    return chgStatus.SOC;
}

// Updates the battery state and charge information
void EVBattery::SetState(int& newState){

    if ((chgStatus.mode == CHARGE) ||
        (chgStatus.mode == DISCHARGE)) {
        UpdateCharge();
    }

    if (newState == IDLE) {
        chgStatus.activePower= 0;
        chgStatus.realEnergy = 0;
    }
    chgStatus.mode = newState;
    time_t now;
    chgStatus.updateTime = time(&now);

}

/* Models Stage 1 of battery charge time where there is a constant current until the
* battery cell voltage limit is reached (at about 85% capacity.
* Charging equation is:
*     t = i/c   t is hours, i is battery capacity in Ah, and c is charger capacity in Amps
* Optional TODO: Add equation for discharging
*/

void EVBattery::UpdateCharge() {
    time_t now;
    double seconds;
    int xPercent;

    if (chgStatus.mode == CHARGE) {

        //The maxtime is scaled by the model time scale
        double cMaxTimeInSecs = (3600*(capInAh/chargerAmps))/timeScale;

         // determine how many seconds have elapsed since last update
        time(&now);
        seconds = difftime(now,chgStatus.updateTime);

        // Update SOC on how much time elapsed (in percentage)
        xPercent = (seconds/cMaxTimeInSecs)*100;
        chgStatus.SOC += xPercent;
        if (chgStatus.SOC > MAXCHARGE) {
            chgStatus.SOC = MAXCHARGE;
        }

///////////////////////////////////////////////////
       //TODO: Calculate real ENERGY and active power
       //Tess Edits v
    int chargeVoltage=240;
    activePower=chargerAmps*chargerVolts;
    realEnergy=activePower*.083;  //active power for 5 min/60 min in 1 hour
       //Tess Edits ^
////////////////////////////////////////////////////
    }
    if (chgStatus.mode == DISCHARGE) {
            activePower=-activerPower;//Tess
            realEnergy=ealEnergy=activePower*.083;//Tess
        //Optional TODO: Add discharge calculation
    }
    else {
        activePower=abs(activePower);
        realEnergy=ealEnergy=activePower*.083;
    }//Tess
}

// Updates battery status and returns charge needed for a specific number of miles
tuple<battState, int, int> EVBattery::QueryChg(int& miles) {
    int tSOC;
    float d;
    //miles=0;
    //Tess edit v
 //   for(i=0; i < dayaheadprices.size(); i++) {
 //   dayaheadpricehour[i]=dayaheadprices[i][0];
 //   dayaheadprice[i]=dayaheadprices[i][2];
 //   realtimeprice[i]=dayaheadprices[i][3];
//for(x=0;x< dayaheadprices[i].size(); x++){}
//}  Jose Sample Code

    //Tess edit ^
    //Need to update charge level
    if ((chgStatus.mode == CHARGE) ||
        (chgStatus.mode == DISCHARGE)) {
        UpdateCharge();
    }

    // If miles is 0 than assume a full charge
    float sizeInWh = sizeInkWh*1000;
    if (miles == 0) {
        d = sizeInWh* MAXCHARGE/100;  //max charge is 85% so /100
    }
    else {
        //Calculate how many wh are needed for miles
        d = whPerMi * miles;  //Tess - this miles should reflect multdist variable
    }

    tSOC = (d/sizeInWh) * 100;   //percent of battery needed for next trips

    //TODO: Calculate min charge time needed for tSOC( divided by timescale)
    //int minChgInSecs = 0;
//Tess Edit v
    int secsPerHour=3600;
    int minChgInSecs = d*secsPerHour*(1/chargingPowerNow)*(1/multiplier); //Wh*(secs/h)*(1/rateofcharge (W))*(1/300Timescale) timescale necessary?

//Tess Edit ^
    return make_tuple(chgStatus,tSOC,minChgInSecs);
    //equivalent to (battState, targetSOC, minChargeDurationInSecs/minChargeTime)
}
