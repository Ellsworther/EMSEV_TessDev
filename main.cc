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

#include <iostream>
#include <vector>
#include <cstdlib>
#include <string>
#include <thread>
#include <sstream>
#include <chrono>

#include "seInterface.h"
#include "assetMgr.h"
#include "emsUtils.h"
#include "TSUtilities.h"

using namespace std;
using namespace ajn;
using namespace qcc;

static bool done = false;

SessionPort port = 123;
const char* SERVICE_PATH = "/smartEnergy";

AssetMgr assetMgr;
//ELAR Hate to make these global, needed for access to EMS from sgd control
BusAttachment* bus;
Observer* obs;
custEMS* emsModule;

/******************************************************************
  User I/O
******************************************************************/

static void Help()
{
    cout << "q                          quit" << endl;
    cout << "s <int>                    set and emit update for price for EMS (10ths of cents)" << endl;
    cout << "e                          display EMS Asset Manager status" << endl;
    cout << "d                          get and display properties of all assets" << endl;
    cout << "h                          display this help message" << endl;
}

static void CmdLineHelp(string& arg)
{
 cerr << "Usage: " << arg << " [-h] [-x int] [-t int] [-p filename]" << endl;
               cerr << "\th\tHelp" << endl;
               cerr << "\tx\tTime multiplier, defaults to 300 (1 sec equals 5 minutes"<< endl;
               cerr << "\tt\tSet time to hour (24 hour clock), defaults to current hour" <<endl;
               cerr << "\tp\tPrice input file, if not specified, default constant price is used" << endl;
}

static bool Parse(string& input)
{
    char cmd;
    vector<string> tokens;
    vector<properties> props;
    string inString;
    int num;

    if (input == "") {
        return true;
    }

    stringstream s(input);
    while(!s.eof()) {
        string tmp;
        s >> tmp;
        tokens.push_back(tmp);
    }

    if (tokens.empty()) {
        return true;
    }

    cmd = input[0];

    switch (cmd) {
    case 'q':
        return false;

    case 's':
        if (tokens.size() < 2) {
            Help();
            break;
        }
        inString = tokens.at(1);
        num = strtol(inString.c_str(),nullptr,10);
        assetMgr.current_price_ = float(num)/10;  //Manually set current price
        cout << "Setting Price and sending property updates: "<< assetMgr.current_price_ << endl;  //Manually set current price
        assetMgr.SendProperties();
        break;

    case 'e':
        assetMgr.PrintEMSStatus();
        break;

    case 'd':
        props = assetMgr.GetAssetProperties();
        if (props.size() >0)
            assetMgr.PrintAssetProperties(props);
        break;

    case 'h':
    default:
        Help();
        break;
    }

    return true;
}

static void Shutdown() {

#ifdef ROUTER
    AllJoynRouterShutdown();
#endif // ROUTER
    AllJoynShutdown();
}

void getUserInput() {
    string input;

    while (!done) {
        cout << "> ";
        getline(cin, input);
        done = !Parse(input);
    }
}

/***********************************************************************
   Main
*******************************************************************/
int CDECL_CALL main(int argc, char** argv)
{
    /* parse command line arguments */

    int multiplier = 300;
    time_t emsTime= time(0);
    string pfile;

    if (argc > 1) {
        string h = "-h"; //help
        string x = "-x"; //time scale
        string t = "-t"; //set hour
        string p = "-p"; //price input file
        //string m = "-m"; //set minutes
        string name = argv[0];

        for (int i = 1; i < argc;i= i+2){
            char* argvx = argv[i];

            if(!strcmp(argvx, h.c_str())) {
               CmdLineHelp(name);
               return EXIT_FAILURE;
            }
            if(!strcmp(argvx, x.c_str())) {
               multiplier = atoi(argv[i+1]);
               continue;
            }
            if(!strcmp(argvx, t.c_str())) {
               int hour = atoi(argv[i+1]);
               int x = 0;
               emsTime = emsutils::SetTime(emsTime,hour,x,x);
               continue;
            }
            if (!strcmp(argvx, p.c_str())) {
                pfile = argv[i+1];
                continue;
            }
            // otherwise invalid argument
            cerr << "Invalid argument: "<< argvx << endl;
            CmdLineHelp(name);
            return EXIT_FAILURE;
        }
    }

    cout << "*****ENERGY MANAGER AGENT*****" << endl;


    if (AllJoynInit() != ER_OK) {
        return EXIT_FAILURE;
    }
#ifdef ROUTER
    if (AllJoynRouterInit() != ER_OK) {
        Shutdown();
        return EXIT_FAILURE;
    }
#endif // ROUTER

    /* Create a message bus */
    bus = new BusAttachment("assetMgmt", true);
    QCC_ASSERT(bus != NULL);
    AboutData aboutData("en");
    AboutObj*aboutObj = new AboutObj(*bus);
    QCC_ASSERT(aboutObj != NULL);


#ifdef SECURE
    QCC_SetDebugLevel("ALLJOYN_AUTH", 1);
    QCC_SetDebugLevel("CRYPTO", 1);
    QCC_SetDebugLevel("AUTH_KEY_EXCHANGER", 1);
#endif // SECURE

    //QCC_SetDebugLevel("UDP", 1);


    if (ER_OK != SetupBusAttachment(*bus,aboutData,port)) {
        delete aboutObj;
        aboutObj = NULL;
        delete bus;
        return EXIT_FAILURE;
    }

    const char* intfname = INTF_EDEV;
    obs = new Observer(*bus, &intfname, 1);
    AssetListener* assetListener = new AssetListener();
    assetListener->observer = obs;
    assetListener->bus = bus;
    obs->RegisterListener(*assetListener);

    emsModule = new custEMS(*bus, SERVICE_PATH);

    if (ER_OK != bus->RegisterBusObject(*emsModule)) {
        cerr << "Could not register customer EMS on the bus" << endl;
        delete &emsModule;
    }
    else
        cout << "Customer EMS registered on the AllJoyn bus" << endl;

    if (ER_OK != assetMgr.Init(multiplier,emsTime,pfile)) {
         cerr<< "Customer EMS Asset Management controller initialization failure" << endl;
         return EXIT_FAILURE;
    }

    aboutObj->Announce(port, aboutData);

    // Start thread for user input
    // User can terminate program by setting done to false
    thread cmdLineIO(getUserInput);

    while (!done) {
        // time the total ems control process
        auto time_start = chrono::high_resolution_clock::now();
        assetMgr.ControlLoop();  //Tylor
        emsModule->EmitUpdatedProperty("price");
        auto time_end = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> time_elapsed = time_end - time_start;

        // determine total wait time accounting for time scale and the real time
        // price signal frequency.
        int time_wait = assetMgr.price_frequency_ / multiplier;  // seconds
        time_wait = 1000*time_wait;  // milliseconds

        // determine sleep duration after deducting process time
        int time_remaining = (time_wait - time_elapsed.count());
        time_remaining = (time_remaining > 0) ? time_remaining : 0;
        this_thread::sleep_for (chrono::milliseconds (time_remaining));
    }

    cmdLineIO.join();

    assetMgr.Shutdown();

    // Cleanup Producer
    aboutObj->Unannounce();
    delete aboutObj;
    aboutObj = NULL;

    //Cleanup Consumer

    obs->UnregisterAllListeners();
    delete obs;
    delete assetListener;

    delete bus;
    bus = NULL;

    Shutdown();
    return EXIT_SUCCESS;
}
