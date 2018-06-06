#ifndef SEINTERFACES_INC
#define SEINTERFACES_INC

#include <string>

#include <alljoyn/Status.h>
#include <alljoyn/AboutObj.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/Observer.h>
#include <alljoyn/Init.h>

#include "assetMgr.h"

#ifdef SECURE
#include <alljoyn/AuthListener.h>
#include <qcc/CryptoECC.h>
#include <qcc/CertificateECC.h>
#endif // SECURE
#include <qcc/Log.h>


using namespace std;
using namespace ajn;


#define INTF_SERVER "edu.pdx.powerlab.sep.server"
#define INTF_EDEV "edu.pdx.powerlab.sep.edev"

/********************************************
* AllJoyn Bus
******************************************/
class SPL : public SessionPortListener {
    virtual bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts) {

//        if (sessionPort != port) {
//            cerr << "Join attempt on unexpected session port "<< port << endl;
//            return false;
//        }
        cout << "\t\tAccepting join session request from " << joiner << endl;
        return true;
    }
};

QStatus SetupBusAttachment(BusAttachment& bus, AboutData& aboutData, SessionPort& port);

/****************************************
* AllJoyn Producer Interface
****************************************/
class custEMS : public BusObject {
    private:
        BusAttachment& bus;
        const InterfaceDescription::Member* energySignalMember;

    public:
        custEMS(BusAttachment& bus, const char* path):
            BusObject (path),
            bus (bus),
            energySignalMember(NULL)
    {
        const InterfaceDescription* intf = bus.GetInterface(INTF_SERVER);
        QCC_ASSERT(intf);
        AddInterface(*intf, ANNOUNCED);

    }

    QStatus Get(const char* ifcName, const char* propName, MsgArg& val);
    void EmitUpdatedProperty(const char* propName);
};

QStatus SendUpdatedPropertyNotification(const char*);

/*********************************************
* AllJoyn Consumer Interface
*************************************************/

class AssetProxy {
    ProxyBusObject proxy;
    BusAttachment& bus;
  private:
    /* private assignment operator, does nothing */
    AssetProxy operator=(const AssetProxy&);
  public:
    bool IsValid();
    QStatus GetAssetName(string&);
    QStatus GetTelemetry(MsgArg&);

    AssetProxy(ProxyBusObject proxy, BusAttachment& bus) : proxy(proxy), bus(bus) {
        proxy.EnablePropertyCaching();
    }
};

class AssetListener :
    public MessageReceiver,
    public Observer::Listener,
    public ProxyBusObject::PropertiesChangedListener  {
        static const char* props[];

        public:
        Observer* observer;
        BusAttachment* bus;

        static void PrintAssetState(AssetProxy&);
        virtual void ObjectDiscovered(ProxyBusObject&);
        virtual void ObjectLost(ProxyBusObject&);
        virtual void PropertiesChanged(ProxyBusObject&, const char*, const MsgArg&, const MsgArg&, void*);
};




//Secure Interface
#ifdef SECURE
/* Client's ECDSA certificate and private key. These were generated with the command:
 *
 *   SampleCertificateUtility -createEE 1825 AllJoyn ECDHE Sample Client
 *
 * SampleCertificateUtility is a sample located in the same directory as this.
 */
static const char CLIENT_CERTIFICATE_PEM[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBgTCCASegAwIBAgIUK+FyqHDTwJQIGN8qL5LKvuMXvlYwCgYIKoZIzj0EAwIw\n"
    "NTEzMDEGA1UEAwwqQWxsSm95biBFQ0RIRSBTYW1wbGUgQ2VydGlmaWNhdGUgQXV0\n"
    "aG9yaXR5MB4XDTE1MDkxMjAyMTY0M1oXDTIwMDkxMDAyMTY0M1owJjEkMCIGA1UE\n"
    "AwwbQWxsSm95biBFQ0RIRSBTYW1wbGUgQ2xpZW50MFkwEwYHKoZIzj0CAQYIKoZI\n"
    "zj0DAQcDQgAENNaoEa6torBhw99OhA1GtHziPr3GgdSHmbggBYagf/sEj/bwim0P\n"
    "e/YuTYWkEhQkv30FdjKTybvWoCweaiZkDqMkMCIwCQYDVR0TBAIwADAVBgNVHSUE\n"
    "DjAMBgorBgEEAYLefAEBMAoGCCqGSM49BAMCA0gAMEUCIEeWUwtAKw0QKenLPPT6\n"
    "UQ5sveMbnCSBzx8MDTBMkarjAiEA1zyiRF6nst3ONfipCUr2+1lOBWb04ojZ4E+m\n"
    "oq7cR1w=\n"
    "-----END CERTIFICATE-----\n";
static const char CLIENT_KEY_PEM[] =
    "-----BEGIN EC PRIVATE KEY-----\n"
    "MDECAQEEIAsvmKOj5rmcfE56FhuKD8tRpiixXUyDycaISQslxaLIoAoGCCqGSM49\n"
    "AwEH\n"
    "-----END EC PRIVATE KEY-----\n";

/* Certificate Authority's ECDSA certificate. This is used to verify the remote peer's
 * certificate chain.
 *
 *    SampleCertificateUtility -createCA 3650 AllJoyn ECDHE Sample Certificate Authority
 *
 */
static const char CA_CERTIFICATE_PEM[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBnzCCAUWgAwIBAgIUdcyHkQndQDgjP2XnhmP43Kak/GAwCgYIKoZIzj0EAwIw\n"
    "NTEzMDEGA1UEAwwqQWxsSm95biBFQ0RIRSBTYW1wbGUgQ2VydGlmaWNhdGUgQXV0\n"
    "aG9yaXR5MB4XDTE1MDkxMjAyMTYzOFoXDTI1MDkwOTAyMTYzOFowNTEzMDEGA1UE\n"
    "AwwqQWxsSm95biBFQ0RIRSBTYW1wbGUgQ2VydGlmaWNhdGUgQXV0aG9yaXR5MFkw\n"
    "EwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEhUADDEGG1bvJ4qDDodD2maFmENFSEmhQ\n"
    "hvP4iJ82WT7XrhIx/L/XIZo9wKnwNsHJusLVXXMKjyUwcPuVpYU7JqMzMDEwDAYD\n"
    "VR0TBAUwAwEB/zAhBgNVHSUEGjAYBgorBgEEAYLefAEBBgorBgEEAYLefAEFMAoG\n"
    "CCqGSM49BAMCA0gAMEUCIAWutM+O60m/awMwJvQXHVGXq+z+6nac4KRLDT5OXqn1\n"
    "AiEAq/NwQWXJ/FYHBxVOXrKxGZXTFoBiudw9+konMAu1MaE=\n"
    "-----END CERTIFICATE-----\n";


bool VerifyCertificateChain(const ajn::AuthListener::Credentials& creds);
#endif // SECuRE

#endif // AJINTERFACES
