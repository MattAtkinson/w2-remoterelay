#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <letmecreate/letmecreate.h>
#include <awa/static.h>

#define RELAY_INSTANCES 1

/**Define DeviceDetails Object**/
typedef struct
{
    char Manufacturer[64];

} DeviceDetailsObject;

static DeviceDetailsObject device[1];

/**Define Relay Object**/
typedef struct
{
    AwaBoolean Outputstate;

} RelayObject;

static RelayObject relay[RELAY_INSTANCES];

/**Handler that takes action when the Relay Object or Resource is created/changed**/
AwaResult handler(AwaStaticClient * client, AwaOperation operation, AwaObjectID objectID,
                  AwaObjectInstanceID objectInstanceID, AwaResourceID resourceID, AwaResourceInstanceID resourceInstanceID,
                  void ** dataPointer, size_t * dataSize, bool * changed)
{
    AwaResult result = AwaResult_InternalError;
    if ((objectID == 3201) && (objectInstanceID >= 0) && (objectInstanceID < RELAY_INSTANCES))
    {
        switch (operation)
        {   
            case AwaOperation_CreateObjectInstance:
            {
                memset(&relay[objectInstanceID], 0, sizeof(relay[objectInstanceID]));
                result = AwaResult_SuccessCreated;
                break;
            }
            case AwaOperation_CreateResource:
            {
                relay[objectInstanceID].Outputstate = false; 
                result = AwaResult_SuccessCreated;
                break;
            }
            case AwaOperation_Write:
            {
                AwaBoolean newState = **((AwaBoolean **)dataPointer);
                if (newState != relay[objectInstanceID].Outputstate)
                {
                    *changed = true;
                    relay[objectInstanceID].Outputstate = newState;
                    if (relay[objectInstanceID].Outputstate)
                    {
                        led_switch_on(ALL_LEDS);                                        // Actions taken when state changes to true
                        relay2_click_enable_relay(MIKROBUS_1, RELAY2_CLICK_RELAY_1);    //
                    }
                    else
                    {
                        led_switch_off(ALL_LEDS);                                       // Actions taken when state changes to false
                        relay2_click_disable_relay(MIKROBUS_1, RELAY2_CLICK_RELAY_1);   //
                    }

                    result = AwaResult_SuccessChanged;
                }
                break;
            }
            case AwaOperation_Read:
            {
                *dataPointer = &relay[objectInstanceID].Outputstate;
                *dataSize = sizeof(relay[objectInstanceID].Outputstate);
                result = AwaResult_SuccessContent;
                break;
            }
            default:
                result = AwaResult_InternalError;
                break;
        }
    }
    return result;
}

/**Define IPSO Objects**/
static void DefineDeviceDetailsObject(AwaStaticClient * awaClient)
{
    AwaStaticClient_DefineObject(awaClient, 3, "DeviceDetails", 0, 1);
    AwaStaticClient_DefineResource(awaClient, 3, 0, "Manufacturer", AwaResourceType_String, 0, 1, AwaResourceOperations_ReadOnly);
    AwaStaticClient_SetResourceStorageWithPointer(awaClient, 3, 0, &device[0].Manufacturer, sizeof(device[0].Manufacturer), sizeof(device[0]));
}

static void DefineRelayObject(AwaStaticClient * awaClient)
{
    AwaStaticClient_DefineObject(awaClient, 3201, "Digital Output", 0, RELAY_INSTANCES);
    AwaStaticClient_DefineResource(awaClient, 3201, 5550, "Relay",  AwaResourceType_Boolean, 0, 1, AwaResourceOperations_ReadWrite);
    AwaStaticClient_SetResourceStorageWithPointer(awaClient, 3201, 5550, &relay[0].Outputstate, sizeof(relay[0].Outputstate), sizeof(relay[0]));
    AwaStaticClient_SetResourceOperationHandler(awaClient, 3201, 5550, handler);
}

static void SetInitialValues(AwaStaticClient * awaClient)
 {
    int instance = 0;
 
    AwaStaticClient_CreateObjectInstance(awaClient, 3201, instance);
    AwaStaticClient_CreateObjectInstance(awaClient, 3, instance);
 
    AwaStaticClient_CreateResource(awaClient, 3, instance, 0);
    strcpy(device[instance].Manufacturer, "Creator Remote Relay");
 
    AwaStaticClient_CreateResource(awaClient, 3201, instance, 5550);
    relay[instance].Outputstate = false;
}

bool ReadCertificate(const char *filePath, char **certificate)
{
    size_t inputFileSize;
    FILE *inputFile = fopen(filePath, "rb");
    if (inputFile == NULL)
    {
        printf("Unable to open certificate file under: %s", filePath);
        return false;
    }
    if (fseek(inputFile, 0, SEEK_END) != 0)
    {
        printf("Can't set file offset.");
        return false;
    }
    inputFileSize = ftell(inputFile);
    rewind(inputFile);
    *certificate = malloc(inputFileSize * (sizeof(char)));
    fread(*certificate, sizeof(char), inputFileSize, inputFile);
    if (fclose(inputFile) == EOF)
    {
        printf("Couldn't close certificate file.");
    }
    return true;
}

int main(void)
{

    char *clientName = "Creator Remote Relay";
 
    srand(time(NULL));
    int port = 6000 + (rand() % 32768);
 
    char *cert = NULL;
    char *certFilePath = "/etc/config/creatorworkshop.crt"; // Path to Certificate file

    AwaStaticClient * awaClient = AwaStaticClient_New();
 
    AwaStaticClient_SetLogLevel(AwaLogLevel_Verbose);
    AwaStaticClient_SetEndPointName(awaClient, clientName);
    ReadCertificate(certFilePath, &cert);
    AwaStaticClient_SetCertificate(awaClient, cert, strlen(cert), AwaSecurityMode_Certificate);
    AwaStaticClient_SetCoAPListenAddressPort(awaClient, "0.0.0.0", port);
    AwaStaticClient_SetBootstrapServerURI(awaClient, "coaps://deviceserver.creatordev.io:15684");
 
    AwaStaticClient_Init(awaClient);
 
    DefineRelayObject(awaClient);
    DefineDeviceDetailsObject(awaClient);
    SetInitialValues(awaClient);

    led_init();

    while (1)
    {
        AwaStaticClient_Process(awaClient);
    }

    AwaStaticClient_Free(&awaClient);

    return 0;
}
