//------------------------------------------------------------------------------
//
//  SimConnect	Nautipilot - Autopilot for boats
// 
//------------------------------------------------------------------------------

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <cmath>

#include "SimConnect.h"

int     quit = 0;
HANDLE  hSimConnect = NULL;

static enum EVENT_ID {
    EVENT_SIM_START,
    EVENT_A,
};

static enum DATA_DEFINE_ID {
    DEFINITION_RUDDER, DEFINITION_GPS, DEFINITION_HDG
};

static enum DATA_REQUEST_ID {
    REQUEST_RUDDER, REQUEST_GPS, REQUEST_HDG
};

struct structRudderInput
{
	double rudderInput;
};

structRudderInput		ru;

struct structGpsDev
{
    double gpsDev;
};

structGpsDev		gps;

struct structHdg
{
    double Hdg;
};

structHdg		hdg;

void CALLBACK MyDispatchProcTC(SIMCONNECT_RECV* pData, DWORD cbData, void *pContext)
{
    HRESULT hr;
    
    switch(pData->dwID)
    {
        case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
        {
            SIMCONNECT_RECV_SIMOBJECT_DATA *pObjData = (SIMCONNECT_RECV_SIMOBJECT_DATA*)pData;
            
            switch(pObjData->dwRequestID)
            {
                case REQUEST_RUDDER:
                {
					// Read the received rudder input
					structRudderInput*pS = (structRudderInput*)&pObjData->dwData;
					ru.rudderInput	= pS->rudderInput;
					printf("\nrudder = %2.1f", pS->rudderInput);
                }

                case REQUEST_GPS:
                {
                    // Read the desired bearing
                    structGpsDev* pS = (structGpsDev*)&pObjData->dwData;
                    gps.gpsDev = pS->gpsDev;
                    printf("\nDesired bearing = %2.1f", pS->gpsDev);
                }

                case REQUEST_HDG:
                {
                    // Read the received heading
                    structHdg* pS = (structHdg*)&pObjData->dwData;
                    hdg.Hdg = pS->Hdg;
                    printf("\nHeading = %2.1f", pS->Hdg);
                }

                default:
                    break;
            }
            break;
        }

        case SIMCONNECT_RECV_ID_EVENT:
        {
            SIMCONNECT_RECV_EVENT *evt = (SIMCONNECT_RECV_EVENT*)pData;

            switch(evt->uEventID)
            {

		        case EVENT_SIM_START:
                    {
			            // Request rudder input and GPS deviation
		                hr = SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_RUDDER, DEFINITION_RUDDER, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_ONCE);
                        hr = SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_GPS, DEFINITION_GPS, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_ONCE);
                        hr = SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_HDG, DEFINITION_HDG, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_ONCE);
                    }
			        break;
						
			    case EVENT_A:
                    {
                                // I'm the captain now
                                if (round(gps.gpsDev * 100.0) / 100.0 < round(hdg.Hdg * 100.0) / 100.0) {
                                    ru.rudderInput = -0.1;
                                    printf("\nrudder = %2.1f", ru.rudderInput);
                                    hr = SimConnect_SetDataOnSimObject(hSimConnect, DEFINITION_RUDDER, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(ru), &ru);
                                    hr = SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_GPS, DEFINITION_GPS, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_ONCE);
                                    hr = SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_HDG, DEFINITION_HDG, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_ONCE);
                                }

                                else if (round(gps.gpsDev * 100.0) / 100.0 > round(hdg.Hdg * 100.0) / 100.0) {
                                    ru.rudderInput = 0.1;
                                    printf("\nrudder = %2.1f", ru.rudderInput);
                                    hr = SimConnect_SetDataOnSimObject(hSimConnect, DEFINITION_RUDDER, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(ru), &ru);
                                    hr = SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_GPS, DEFINITION_GPS, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_ONCE);
                                    hr = SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_HDG, DEFINITION_HDG, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_ONCE);
                                }

                                else {
                                    ru.rudderInput = 0;
                                    printf("\nrudder = %2.1f", ru.rudderInput);
                                    hr = SimConnect_SetDataOnSimObject(hSimConnect, DEFINITION_RUDDER, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(ru), &ru);
                                    hr = SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_GPS, DEFINITION_GPS, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_ONCE);
                                    hr = SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_HDG, DEFINITION_HDG, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_ONCE);
                                }
                    }
                    break;

                default:
                    break;
            }
            break;
        }

        case SIMCONNECT_RECV_ID_QUIT:
        {
            quit = 1;
            break;
        }

        default:
            break;
    }
}

void applicationSetup()
{
    HRESULT hr;

    if (SUCCEEDED(SimConnect_Open(&hSimConnect, "Nautipilot", NULL, 0, 0, 0)))
    {
        
        // Set up a data definition for the rudder
        hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_RUDDER,
			"RUDDER PEDAL POSITION", "position");

        // Set up a data definition for the GPS
        hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_GPS,
            "GPS WP BEARING", "Radians");

        // Set up a data definition for the heading
        hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_HDG,
            "GPS GROUND TRUE HEADING", "Radians");

        // Run event 6x a second
        hr = SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_A, "6Hz");

        // Create a private key event
		hr = SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_A);

        while( 0 == quit )
        {
            SimConnect_CallDispatch(hSimConnect, MyDispatchProcTC, NULL);
            Sleep(1);
        } 

        hr = SimConnect_Close(hSimConnect);
    }
}

int __cdecl _tmain(int argc, _TCHAR* argv[])
{
    applicationSetup();

	//return 0;
}
