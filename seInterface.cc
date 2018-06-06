/******************************************************************************
 *    Copyright (c) V2 Systems, LLC.  All rights reserved.
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
**********************************************************/
#include "seInterface.h"

SPL session_port_listener;
extern custEMS* emsModule;
extern AssetMgr assetMgr;

#ifdef SECURE
static const char* KEYX_ECDHE_NULL = "ALLJOYN_ECDHE_NULL";
static const char* KEYX_ECDHE_PSK = "ALLJOYN_ECDHE_PSK";
static const char* KEYX_ECDHE_ECDSA = "ALLJOYN_ECDHE_ECDSA";
static const char* KEYX_ECDHE_SPEKE = "ALLJOYN_ECDHE_SPEKE";
static const char* ECDHE_KEYX = "ALLJOYN_ECDHE_ECDSA";
#endif //SECURE

/***********************************************************************
Secure Interface
*************************************************************************/
#ifdef SECURE
/*
 * This is the local implementation of the an AuthListener.  ECDHEKeyXListener is
 * designed to only handle ECDHE Key Exchange Authentication requests.
 *
 * If any other authMechanism is used other than ECDHE Key Exchange authentication
 * will fail.
 */

// Note: good coding practices would define the class in the header file
class ECDHEKeyXListener : public AuthListener {
  public:

    ECDHEKeyXListener()
    {
    }

    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds)
    {
        QCC_UNUSED(userId);

        printf("RequestCredentials for authenticating peer name %s using mechanism %s authCount %d\n", authPeer, authMechanism, authCount);
        if (strcmp(authMechanism, KEYX_ECDHE_NULL) == 0) {
            creds.SetExpiration(100);  /* set the master secret expiry time to 100 seconds */
            return true;
        } else if (strcmp(authMechanism, KEYX_ECDHE_PSK) == 0) {
            /*
             * Solicit the Pre shared secret
             */
            if ((credMask& AuthListener::CRED_USER_NAME) == AuthListener::CRED_USER_NAME) {
                printf("RequestCredentials received psk ID %s\n", creds.GetUserName().c_str());
            }
            /*
             * Based on the pre shared secret id, the application can retrieve
             * the pre shared secret from storage or from the end user.
             * In this example, the pre shared secret is a hard coded string.
             * Pre-shared keys should be 128 bits long, and generated with a
             * cryptographically secure random number generator.
             */
            String psk("faaa0af3dd3f1e0379da046a3ab6ca44");
            creds.SetPassword(psk);
            creds.SetExpiration(100);  /* set the master secret expiry time to 100 seconds */
            return true;
        } else if (strcmp(authMechanism, KEYX_ECDHE_ECDSA) == 0) {
            /* Supply the private key and certificate. */
            String privateKeyPEM(CLIENT_KEY_PEM);
            String certChainPEM;
            /* In constructing the certificate chain, the node's certificate comes first,
             * and then each Certificate Authority appears in order, with the last entry being the
             * root certificate. In this sample, we only have a chain of length two. If there were
             * additional intermediate CAs along the path, those would appear in order between the end
             * entity certificate and the root.
             *
             * It's a common optimization to omit the root certificate since the remote peer should already
             * have it, if it's a trusted root. Since this chain has no intermediates, we include the whole
             * chain for demonstrative purposes. */
            certChainPEM.assign(CLIENT_CERTIFICATE_PEM);
            certChainPEM.append(CA_CERTIFICATE_PEM);
            if ((credMask& AuthListener::CRED_PRIVATE_KEY) == AuthListener::CRED_PRIVATE_KEY) {
                creds.SetPrivateKey(privateKeyPEM);
            }
            if ((credMask& AuthListener::CRED_CERT_CHAIN) == AuthListener::CRED_CERT_CHAIN) {
                creds.SetCertChain(certChainPEM);
            }
            creds.SetExpiration(100);  /* set the master secret expiry time to 100 seconds */
            return true;
        } else if (strcmp(authMechanism, KEYX_ECDHE_SPEKE) == 0) {
            /*
             * Based on the pre shared secret id, the application can retrieve
             * the password from storage or from the end user.
             * In this example, the password is a hard coded string.
             */
            String password("1234");
            creds.SetPassword(password);
            creds.SetExpiration(100);              /* set the master secret expiry time to 100 seconds */
            return true;
        }
        return false;
    }

    bool VerifyCredentials(const char* authMechanism, const char* authPeer, const Credentials& creds)
    {
        QCC_UNUSED(authPeer);
        /* only the ECDHE_ECDSA calls for peer credential verification */
        if (strcmp(authMechanism, KEYX_ECDHE_ECDSA) == 0) {
            if (creds.IsSet(AuthListener::CRED_CERT_CHAIN)) {
                /*
                 * AllJoyn sends back the certificate chain for the application to verify.
                 * The application has to option to verify the certificate
                 * chain.  If the cert chain is validated and trusted then return true; otherwise, return false.
                 */
                return true;
            }
            /*return VerifyCertificateChain(creds);*/
            return true;
        }
        return false;
    }

    void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
        QCC_UNUSED(authPeer);
        printf("SampleClientECDHE::AuthenticationComplete Authentication %s %s\n", authMechanism, success ? "successful" : "failed");
    }
};

#endif // SECURE


/***********************************************************************
Emulation of Smart Energy Profile Interfaces
*************************************************************************/
static QStatus BuildServerInterface(BusAttachment& bus)
{
    QStatus status;

    InterfaceDescription* intf = NULL;

#ifdef SECURE
    status = bus.CreateInterface(INTF_SERVER, intf,true);
    QCC_ASSERT (ER_OK == status);
#else // SECURE
    status = bus.CreateInterface(INTF_SERVER, intf);
    QCC_ASSERT (ER_OK == status);
#endif

    status = intf->AddProperty("EMSName","s",PROP_ACCESS_READ);
    QCC_ASSERT (ER_OK == status);
    status = intf->AddPropertyAnnotation("EMSName","org.freedesktop.DBus.Property.EmitsChangedSignal","const");
    QCC_ASSERT (ER_OK == status);
    status = intf->AddProperty("Time","u",PROP_ACCESS_READ);
    QCC_ASSERT (ER_OK == status);
    status = intf->AddPropertyAnnotation("Time","org.freedesktop.DBus.Property.EmitsChangedSignal","true");
    QCC_ASSERT (ER_OK == status);
    status = intf->AddProperty("price","i",PROP_ACCESS_READ);
    QCC_ASSERT (ER_OK == status);
    status = intf->AddPropertyAnnotation("price","org.freedesktop.DBus.Property.EmitsChangedSignal","true");
    QCC_ASSERT (ER_OK == status);

    intf->Activate();

    return status;
}

static QStatus BuildEndDeviceInterface(BusAttachment& bus)
{
    QStatus status;

    InterfaceDescription* intf = NULL;

#ifdef SECURE
    status = bus.CreateInterface(INTF_EDEV, intf,true);
    QCC_ASSERT (ER_OK == status);
#else // SECURE
    status = bus.CreateInterface(INTF_EDEV, intf);
    QCC_ASSERT (ER_OK == status);
#endif

    status = intf->AddProperty("AssetName","s",PROP_ACCESS_READ);
    QCC_ASSERT (ER_OK == status);
    status = intf->AddPropertyAnnotation("AssetName","org.freedesktop.DBus.Property.EmitsChangedSignal","const");
    QCC_ASSERT (ER_OK == status);
    status = intf->AddProperty("PowerStatus","a{sv}",PROP_ACCESS_READ);
    QCC_ASSERT (ER_OK == status);
    status = intf->AddPropertyAnnotation("PowerStatus","org.freedesktop.DBus.Property.EmitsChangedSignal","true");
    QCC_ASSERT (ER_OK == status);

    intf->Activate();

    return status;
}

/***********************************************************************
Set up Consumer Interface for Assets
*************************************************************************/
void AssetListener::PrintAssetState(AssetProxy& asset) {

    MsgArg telemetry;
    string assetName;

    if (ER_OK != asset.GetAssetName(assetName)) {
        cerr << "\tCould not retrieve asset Name" << endl;
    } else {
        cout << "\tAsset Name: " << assetName << endl;};

    if (ER_OK != asset.GetTelemetry(telemetry)) {
        cerr << "Could not retrieve Asset Telemetry" << endl;
        return;
    }

       // process telemetry
        size_t nelem = 0;
        MsgArg* elems = NULL;

        if (ER_OK != telemetry.Get("a{sv}", &nelem, &elems)) {
           cerr << "Error in getting telemetry elements" << endl;
        } else {
            cout << "\tEndDevice Properties: " << endl;
            char* name;
            uint32_t uint32_value;
            uint16_t uint16_value;
            uint8_t uint8_value;
            int32_t int32_value;
            int16_t int16_value;

            char* str_value;
            for (size_t i = 0; i < nelem; i++) {
                if (ER_OK == elems[i].Get("{su}", &name, &uint32_value)) {
                    cout << "\t\t" << name;
                    if (!strcmp(name,"changedTime")) {
                        time_t timeStamp = uint32_value;
                        cout << "\t" << asctime(localtime(&timeStamp)) << endl;
                    } else cout << "\t" << uint32_value << endl;
                    continue;
                }
                if (ER_OK == elems[i].Get("{sq}", &name, &uint16_value)) {
                    cout << "\t\t" << name << "\t" << uint16_value << endl;
                    continue;
                }
                if (ER_OK == elems[i].Get("{si}", &name, &int32_value)) {
                    cout << "\t\t" << name << "\t" << int32_value << endl;
                    continue;
                }
                if (ER_OK == elems[i].Get("{sn}", &name, &int16_value)) {
                    cout << "\t\t" << name << "\t" << int16_value << endl;
                    continue;
                }
                if (ER_OK == elems[i].Get("{ss}", &name, &str_value)) {
                    cout << "\t\t" << name  << "\t" << str_value << endl;
                    continue;
                }
                if (ER_OK == elems[i].Get("{sy}", &name, &uint8_value)) {
                    cout << "\t\t" << name << "\t" << uint8_value << endl;
                    continue;
                }
                cout << "unknown telemetry value" << endl;
            }
        }
    }

void AssetListener::ObjectDiscovered(ProxyBusObject& proxy) {
        bus->EnableConcurrentCallbacks();
        cout << "\t\t[listener] Asset " << proxy.GetUniqueName() << " has been discovered" << endl;
        AssetProxy asset(proxy, *bus);
        string assetName;
        if (ER_OK != asset.GetAssetName(assetName)) {
        cerr << "\tCould not retrieve asset Name" << endl;
        } else cout << "\tAsset Name: " << assetName << endl;
        proxy.RegisterPropertiesChangedListener(INTF_EDEV, props, 2, *this, NULL);
        cout << "> ";
        cout.flush();

}

void AssetListener::ObjectLost(ProxyBusObject& proxy) {
        cout << "\t\t[listener]  Asset " << proxy.GetUniqueName() << ":"
            << proxy.GetPath() << "no longer exists" << endl;
        cout << "> ";
        cout.flush();
}

void AssetListener::PropertiesChanged(ProxyBusObject& obj,
                                   const char* ifacename,
                                   const MsgArg& changed,
                                   const MsgArg& invalidated,
                                   void* context) {
        QCC_UNUSED(context);
        QStatus status = ER_OK;


        cout << "Properties changed signal received: " << endl;
        if (strcmp(ifacename, INTF_EDEV)) {
            cout << ifacename << "does not contain endevice properties" << endl;
            return;
        }
        bus->EnableConcurrentCallbacks();
        AssetProxy asset(observer->Get(ObjectId(obj)), *bus);
        if (!asset.IsValid()){
            cout << "/t from an asset we don't know." << endl;
            status = ER_FAIL;
        }

        string assetName;
        if (ER_OK != asset.GetAssetName(assetName)) {
            cerr << "\tCould not retrieve asset Name" << endl;
        } else {
            cout << "\tAsset Name: " << assetName << endl;}

        size_t nelem = 0;
        MsgArg* elems = NULL;
        if (ER_OK == status) {
            status = changed.Get("a{sv}", &nelem, &elems);
        }
        if (ER_OK == status) {
            for (size_t i = 0; i < nelem; i++) {
            const char* prop;
            MsgArg* val;
            status = elems[i].Get("{sv}", &prop, &val);
                if (ER_OK == status) {
                string propname = prop;
                    if (propname == "PowerStatus") {
                        cout << "\tPowerStatus property update" << endl;
                        assetMgr.PowerStatusChange();
                    } else {
                        cout << "\tunknown property " << propname << endl;
                    }
                }
            }
        }

        cout << "> ";
        cout.flush();
}

bool AssetProxy::IsValid() {
    return proxy.IsValid();
}

QStatus AssetProxy::GetAssetName(string& val) {
    MsgArg value;
    char* cstr;
    QStatus status = proxy.GetProperty(INTF_EDEV,"AssetName",value);
    if (ER_OK == status) {
        status = value.Get("s", &cstr);
        if (ER_OK ==status)
            val = string(cstr);
    }
    return status;
}

QStatus AssetProxy::GetTelemetry(MsgArg& val) {

    QStatus status = proxy.GetProperty(INTF_EDEV, "PowerStatus", val);

    return status;
}

const char* AssetListener::props[] = {
    "AssetName", "PowerStatus"
};

/***********************************************************************
Set up Producer Interface for EMS
*************************************************************************/
QStatus SendUpdatedPropertyNotification(const char* propName){

        emsModule->EmitUpdatedProperty(propName);
        return ER_OK;
}

QStatus custEMS::Get(const char* ifcName, const char* propName, MsgArg& val)
{
    if (strcmp(ifcName, INTF_SERVER)) {
            return ER_FAIL;
    }

    if (!strcmp(propName,"EMSName")) {
        string name = assetMgr.GetEMSName();
        val.Set("s", name.c_str());
        return ER_OK;
    } else if (!strcmp(propName,"Time")) {
        uint32_t x = 0;
        x = assetMgr.GetModelTime();
        val.Set("u", x);
        return ER_OK;
    } else if (!strcmp(propName,"price")) {
        int32_t p = assetMgr.current_price_*10; //Tess set current price
        val.Set("i", p);
        return ER_OK;
    } else {
        cerr << "Unknown property request " << propName << endl;
        return ER_FAIL;
    }
}

void custEMS::EmitUpdatedProperty(const char* propName)
{
    static MsgArg val;

    if (!strcmp(propName,"price")) {
        int32_t p = assetMgr.current_price_;  //Tess, removed *10 multiplier pull in new current price
        if (ER_OK != val.Set("i", &p)) {
                cerr << " Error in setting price MsgArg" << endl;
        } else {
            cout <<"Emitting price: " << p << endl;
            EmitPropChanged(INTF_SERVER, "price", val, SESSION_ID_ALL_HOSTED);
        }
    }
}

// Tess consider adding additional hour and minute information here
/***********************************************************************
Set up Message Bus for AllJoyn Agent
*************************************************************************/
QStatus SetupBusAttachment(BusAttachment& bus, AboutData& aboutData, SessionPort& port)
{

    QStatus status = ER_OK;
    status = bus.Start();
    QCC_ASSERT (ER_OK == status);
    status = bus.Connect();
    if (status != ER_OK) {
        return status;
    }

    status = BuildServerInterface(bus);
    QCC_ASSERT (ER_OK == status);

    status = BuildEndDeviceInterface(bus);
    QCC_ASSERT (ER_OK == status);


    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    bus.BindSessionPort(port, opts, session_port_listener);

    /* Set up some about data*/
    // AppId is a 128 bit uuid
    uint8_t appID[] = { 0x47, 0xFD, 0xB0, 0x08,
                        0x25, 0x22, 0x48, 0x98,
                        0x9B, 0xAE, 0xE2, 0xE2,
                        0x05, 0xBD, 0x79, 0xEE };
    aboutData.SetAppId(appID, 16);
    aboutData.SetDeviceName("Customer Energy Management Systems Agent");
    aboutData.SetDeviceId("7a439b61-3128-49f9-8a8b-65471a8fdeeb");
    aboutData.SetAppName("Smart Energy");
    aboutData.SetManufacturer("SomeName");
    aboutData.SetModelNumber("00001");
    aboutData.SetDescription("Grid Friendly Energy Management Application");
    aboutData.SetDateOfManufacture("2016-06-06");
    aboutData.SetSoftwareVersion("0.0.1");
    aboutData.SetHardwareVersion("0.0.1");
    aboutData.SetSupportUrl("http://www.example.org");

    if (!aboutData.IsValid()) {
        cerr << "Invalid About data. " << endl;
        return ER_FAIL;
    }

#ifdef SECURE
    status = bus.EnablePeerSecurity(ECDHE_KEYX, new ECDHEKeyXListener(),".alljoyn_keystore/custEMS_ecdhe.ks", false);
    QCC_ASSERT(ER_OK == status);
#endif //SECURE
    return status;
}





