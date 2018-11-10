/**
 * @file main.c
 * @brief A simple connected window example demonstrating the use of Thing Shadow
 */
 
/*******************************************************************************/
/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#include "OLED_Driver.h"
#include "OLED_GUI.h"
#include "DEV_Config.h"

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_shadow_interface.h"
/* Includes End */
/*******************************************************************************/


/*******************************************************************************/
/* Defines */
#define ROOMTEMPERATURE_UPPERLIMIT 32.0f
#define ROOMTEMPERATURE_LOWERLIMIT 25.0f
#define STARTING_ROOMTEMPERATURE ROOMTEMPERATURE_LOWERLIMIT

#define MAX_LENGTH_OF_UPDATE_JSON_BUFFER 200
#define HOST_ADDRESS_SIZE 255
/* Defnies End */ 
/*******************************************************************************/

/*******************************************************************************/
/* Variables */
static char certDirectory[PATH_MAX + 1] = "../certs";
static char HostAddress[HOST_ADDRESS_SIZE] = AWS_IOT_MQTT_HOST;
static uint32_t port = AWS_IOT_MQTT_PORT;
static uint8_t numPubs = 5;

bool ActiveState = true ;
bool SelfTestInvoked = false ;
float temperature = 30.0;
float SetTemperature = 50.0;
float current = 0.0;
int32_t TimeDate = 0 ;

IoT_Error_t rc = FAILURE;
//int32_t i = 0;
	
char JsonDocumentBuffer[MAX_LENGTH_OF_UPDATE_JSON_BUFFER];
size_t sizeOfJsonDocumentBuffer = sizeof(JsonDocumentBuffer) / sizeof(JsonDocumentBuffer[0]);
char *pJsonStringToUpdate;
	
char rootCA[PATH_MAX + 1];
char clientCRT[PATH_MAX + 1];
char clientKey[PATH_MAX + 1];
char CurrentWD[PATH_MAX + 1];

time_t now;
struct tm *timenow;
int oled_delay = 1000;
char value[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
int flag = 0;
volatile int Menu = 0;
volatile int Temp = 100;
volatile int Current = 50;
volatile int AutoMode = 0 ;
volatile int MenuItem = 0;
volatile int SelfTest = 0;

volatile bool Power = false;
volatile bool CloudConnection = false;


/* Variables end */ 
/*******************************************************************************/

/*******************************************************************************/
/* JSON Handlers */
jsonStruct_t SelfTestHandler;
jsonStruct_t ActiveStateHandler;
jsonStruct_t temperatureHandler;
jsonStruct_t SetTemperatureHandler;
jsonStruct_t CurrentHandler;
jsonStruct_t TimeHandler;
/* JSON Handlers end */
/*******************************************************************************/

/*******************************************************************************/
/* Function Definitions */
void ShadowUpdateStatusCallback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
								const char *pReceivedJsonDocument, void *pContextData);
void ActiveState_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext);
void SelfTest_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext);
void SetTemperature_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext);
void SetJSONHandlers();
void parseInputArgsForConnectParams(int argc, char **argv);
void Setup_OLED();
IoT_Error_t AWS_Setup();
IoT_Error_t AWS_Shadow_Setup();
IoT_Error_t AWS_Shadow_Reported_Send();
IoT_Error_t AWS_Shadow_Desired_Send();

void SetupISR();
void Key1_Interrupt();
void Key2_Interrupt();
void Key3_Interrupt();
void KeyUp_Interrupt();
void KeyDown_Interrupt();

void GetTime();

/* Function Definitions End */
/*******************************************************************************/

/*******************************************************************************/
/* Code Begin Main While Loop*/




// initialize the mqtt client
AWS_IoT_Client mqttClient;
ShadowInitParameters_t sp ;
ShadowConnectParameters_t scp ;
FILE *fp;

PI_THREAD(Shadow_Update){
	
	for (;;)
	{
		AWS_Shadow_Reported_Send();
		fp = fopen("Data.txt", "a+");
		if(fp == NULL)
		{
			printf("Error opening file\n");
		}
		fwrite(JsonDocumentBuffer,sizeof(char),sizeof(JsonDocumentBuffer),fp);
		fwrite("\n",sizeof(char),1,fp);
		fclose (fp);
		delay(1000);
	}
}

static void simulateRoomTemperature(float *pRoomTemperature) {
	static float deltaChange;

	if(*pRoomTemperature >= ROOMTEMPERATURE_UPPERLIMIT) {
		deltaChange = -0.5f;
		//windowOpen = true;
	} else if(*pRoomTemperature <= ROOMTEMPERATURE_LOWERLIMIT) {
		deltaChange = 0.5f;
		
	}

	*pRoomTemperature += deltaChange;
}

int main(int argc, char **argv) {
	SetJSONHandlers();
	AWS_Setup();
	parseInputArgsForConnectParams(argc, argv);
	AWS_Shadow_Setup();
	Setup_OLED();
	
    fp = fopen("Data.txt", "w+");
	if(fp == NULL)
	{
		printf("Error opening file\n");
		exit(1);
	}
	fclose (fp);

	temperature = STARTING_ROOMTEMPERATURE;

	piThreadCreate(Shadow_Update);
	
	// loop and publish a change in temperature
	while(1) 
	{
		if (Menu == 0)
		{
			GetTime();
		}
	}
	
	System_Exit();

	return 0;
	
}
/* Code Begin Main while loop end */
/*******************************************************************************/

/*******************************************************************************/
/* Functions Begin */

void GetTime(){
	OLED_Clear(OLED_BACKGROUND);
	OLED_Display();
	time(&now);
	timenow = localtime(&now);
	
	GUI_DisChar(0+18, 22, value[timenow->tm_hour / 10], &Font24, FONT_BACKGROUND, WHITE);
	GUI_DisChar(16+18, 22, value[timenow->tm_hour % 10], &Font24, FONT_BACKGROUND, WHITE);
	GUI_DisChar(32+18, 22, ':', &Font24, FONT_BACKGROUND, WHITE);
	GUI_DisChar(48+18, 22, value[timenow->tm_min / 10],  &Font24, FONT_BACKGROUND, WHITE);
	GUI_DisChar(64+18, 22, value[timenow->tm_min % 10],  &Font24, FONT_BACKGROUND, WHITE);
	
	
	//DrawConCloud();
	//DrawComs();
	
	if (AutoMode == 1)
	{
		DrawAutoModeOn();
	}
	else if (AutoMode == 0)
	{
		DrawAutoModeOff();
	}
	if (Power)
	{
		DrawSystemPower();
	}
	if (CloudConnection)
	{
		DrawConCloud();
	}
	
	TimeDate = timenow;
	
	//DrawAutoModeOn();
	//DrawError();
	DisTemp(Temp);
	DisCurrent(Current);
	
	OLED_Display();
	Driver_Delay_ms(oled_delay+4000);
}

void Setup_OLED(){
	if(System_Init())
	{
		exit(0);
	}
	SetupISR();
	Power = true;
	OLED_SCAN_DIR OLED_ScanDir = SCAN_DIR_DFT;//SCAN_DIR_DFT = D2U_L2R
	OLED_Init(OLED_ScanDir );
	printf("OLED Show \r\n");
	OLED_Clear(0x00);
	if (Power)
	{
		DrawSystemPower();
	}
	OLED_Display();
}

IoT_Error_t AWS_Shadow_Reported_Send(){
	if (NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc) 
	{
		rc = aws_iot_shadow_yield(&mqttClient, 200);
		if(NETWORK_ATTEMPTING_RECONNECT == rc) {
			sleep(1);
			// If the client is attempting to reconnect we will skip the rest of the loop.
			//continue;
		}
		//IOT_INFO("\n=======================================================================================\n");
		
		//rc = aws_iot_shadow_delete(&mqttClient, AWS_IOT_MY_THING_NAME, NULL, NULL, 4, true);
		
/* 		IOT_INFO("On Device: window state %s", windowOpen ? "true" : "false"); */
		simulateRoomTemperature(&temperature);
		rc = aws_iot_shadow_init_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
		if(SUCCESS == rc) {
			rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 6, &temperatureHandler,
											  &CurrentHandler, &ActiveStateHandler, &SelfTestHandler, &TimeHandler, &SetTemperatureHandler);
			if(SUCCESS == rc) {
				rc = aws_iot_finalize_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
				if(SUCCESS == rc) {
					//IOT_INFO("Update Shadow: %s", JsonDocumentBuffer);
					rc = aws_iot_shadow_update(&mqttClient, AWS_IOT_MY_THING_NAME, JsonDocumentBuffer,
											   ShadowUpdateStatusCallback, NULL, 4, true);
				}
			}
		}
		//IOT_INFO("*****************************************************************************************\n");
		sleep(1);
	}
	if(SUCCESS != rc) {
		IOT_ERROR("An error occurred in the loop %d", rc);
	}

/* 	IOT_INFO("Disconnecting");
	rc = aws_iot_shadow_disconnect(&mqttClient);

	if(SUCCESS != rc) {
		IOT_ERROR("Disconnect error %d", rc);
	}

	return rc; */
}

IoT_Error_t AWS_Shadow_Desired_Send(){
	if (NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc) 
	{
		rc = aws_iot_shadow_yield(&mqttClient, 200);
		if(NETWORK_ATTEMPTING_RECONNECT == rc) {
			sleep(1);
		}

		rc = aws_iot_shadow_init_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
		if(SUCCESS == rc) {
			rc = aws_iot_shadow_add_desired(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 2, &ActiveStateHandler, &SelfTestHandler);
			if(SUCCESS == rc) {
				rc = aws_iot_finalize_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
				if(SUCCESS == rc) {
					//IOT_INFO("Update Shadow: %s", JsonDocumentBuffer);
					rc = aws_iot_shadow_update(&mqttClient, AWS_IOT_MY_THING_NAME, JsonDocumentBuffer,
											   ShadowUpdateStatusCallback, NULL, 4, true);
				}
			}
		}
		//IOT_INFO("*****************************************************************************************\n");
		sleep(1);
	}
	if(SUCCESS != rc) {
		IOT_ERROR("An error occurred in the loop %d", rc);
	}
}

void Key1_Interrupt(){
	Menu = 1;
	if (AutoMode == 1)
	{
		MenuPage1_Auto_On(SetTemperature);
		MenuItem = 0;
	}
	else if (AutoMode == 0)
	{
		MenuPage1_Auto_Off(SetTemperature);
		MenuItem = 0;
	}
}

void Key2_Interrupt(){
	if (Menu == 1)
	{
		if (MenuItem == 0 )
		{
			if (AutoMode == 1)
			{
				AutoMode = 0;
				ActiveState = false;
				SelfTestInvoked = false;
			}
			else if (AutoMode == 0)
			{
				AutoMode = 1;
				ActiveState = true;
				SelfTestInvoked = false;
			}
			AWS_Shadow_Desired_Send();
			Key1_Interrupt();
		}
		if (MenuItem == 1)
		{
			SelfTest = 1;
			SelfTestInvoked = true;
			ActiveState = false;
			DisSelfTest();
			delay(3000);
			SelfTest = 0;
			Menu = 0 ;
		}
	}

}
	 
void Key3_Interrupt(){
	if (Menu == 1)
	{
		Menu = 0;
		OLED_Clear(OLED_BACKGROUND);
		OLED_Display();
	}
}

void KeyUp_Interrupt(){
	//Menu = 1;
	if (AutoMode == 1 && Menu == 1)
	{
		MenuPage1_Auto_On(SetTemperature);
		MenuItem = 0;
	}
	else if (AutoMode == 0 && Menu == 1)
	{
		MenuPage1_Auto_Off(SetTemperature);
		MenuItem = 0;
	}

}

void KeyDown_Interrupt(){
	//Menu = 1;
	if (AutoMode == 1 && Menu == 1)
	{
		MenuPage2_Auto_On(SetTemperature);
		MenuItem = 1;
	}
	else if (AutoMode == 0 && Menu == 1)
	{
		MenuPage2_Auto_Off(SetTemperature);
		MenuItem = 1;
	}
	//MenuPage2_Auto_Off();
}

IoT_Error_t AWS_Shadow_Setup(){
	IOT_INFO("Shadow Init");
	rc = aws_iot_shadow_init(&mqttClient, &sp);
	if(SUCCESS != rc) {
		IOT_ERROR("Shadow Connection Error");
		return rc;
	}
	scp = ShadowConnectParametersDefault;
	scp.pMyThingName = AWS_IOT_MY_THING_NAME;
	scp.pMqttClientId = AWS_IOT_MQTT_CLIENT_ID;
	scp.mqttClientIdLen = (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID);

	IOT_INFO("Shadow Connect");
	rc = aws_iot_shadow_connect(&mqttClient, &scp);
	if(SUCCESS != rc) {
		IOT_ERROR("Shadow Connection Error");
		return rc;
	}
	rc = aws_iot_shadow_register_delta(&mqttClient, &ActiveStateHandler);
	rc = aws_iot_shadow_register_delta(&mqttClient, &SelfTestHandler);
	rc = aws_iot_shadow_register_delta(&mqttClient, &SetTemperatureHandler);
	if(SUCCESS != rc) {
		IOT_ERROR("Shadow Register Delta Error");
	}
	aws_iot_shadow_enable_discard_old_delta_msgs();
}

IoT_Error_t AWS_Setup(){
	//IOT_INFO("\nAWS IoT SDK Version %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

	getcwd(CurrentWD, sizeof(CurrentWD));
	snprintf(rootCA, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_ROOT_CA_FILENAME);
	snprintf(clientCRT, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_CERTIFICATE_FILENAME);
	snprintf(clientKey, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_PRIVATE_KEY_FILENAME);

	sp = ShadowInitParametersDefault;
	sp.pHost = AWS_IOT_MQTT_HOST;
	sp.port = AWS_IOT_MQTT_PORT;
	sp.pClientCRT = clientCRT;
	sp.pClientKey = clientKey;
	sp.pRootCA = rootCA;
	sp.enableAutoReconnect = false;
	sp.disconnectHandler = NULL;
	
/* 	IOT_DEBUG("rootCA %s", rootCA);
	IOT_DEBUG("clientCRT %s", clientCRT);
	IOT_DEBUG("clientKey %s", clientKey); */
	
	/*
	 * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
	 *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
	 *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
	 */
	rc = aws_iot_shadow_set_autoreconnect_status(&mqttClient, true);
	CloudConnection = true;
	if(SUCCESS != rc) {
		IOT_ERROR("Unable to set Auto Reconnect to true - %d", rc);
		CloudConnection = false;
		return rc;
	}
}

void SetupISR(){
	if(wiringPiISR(KEY1_PIN, INT_EDGE_FALLING, &Key1_Interrupt) < 0)
	{
		printf("Unable to setup ISR KEY1 \n");
	}
	if(wiringPiISR(KEY2_PIN, INT_EDGE_FALLING, &Key2_Interrupt) < 0)
	{
                printf("Unable to setup ISR KEY2\n");
	}
	if(wiringPiISR(KEY3_PIN, INT_EDGE_FALLING, &Key3_Interrupt) < 0)
	{
                printf("Unable to setup ISR KEY3\n");
	}
	if(wiringPiISR(KEY_UP_PIN, INT_EDGE_FALLING, &KeyUp_Interrupt) < 0)
	{
                printf("Unable to setup ISR KEY2\n");
	}
	if(wiringPiISR(KEY_DOWN_PIN, INT_EDGE_FALLING, &KeyDown_Interrupt) < 0)
	{
                printf("Unable to setup ISR KEY Down\n");
	}
}

void ShadowUpdateStatusCallback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
								const char *pReceivedJsonDocument, void *pContextData) {
	IOT_UNUSED(pThingName);
	IOT_UNUSED(action);
	IOT_UNUSED(pReceivedJsonDocument);
	IOT_UNUSED(pContextData);

	if(SHADOW_ACK_TIMEOUT == status) {
		IOT_INFO("Update Timeout--");
	} else if(SHADOW_ACK_REJECTED == status) {
		IOT_INFO("Update RejectedXX");
	} else if(SHADOW_ACK_ACCEPTED == status) {
		IOT_INFO("Update Accepted !!");
	}
}

void ActiveState_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
	IOT_UNUSED(pJsonString);
	IOT_UNUSED(JsonStringDataLen);

	if(pContext != NULL) {
		//IOT_INFO("Delta - System state changed to %d", *(bool *) (pContext->pData));
		if (*(bool *) (pContext->pData) == 0)
		{
			//IOT_INFO("Automode off");
			AutoMode = 0;
		}
		else if (*(bool *) (pContext->pData) == 1)
		{
			//IOT_INFO("Automode On");
			AutoMode = 1;
		}
	}
}

void SelfTest_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
	IOT_UNUSED(pJsonString);
	IOT_UNUSED(JsonStringDataLen);

	if(pContext != NULL) {
		IOT_INFO("Delta - Self test state changed to %d", *(bool *) (pContext->pData));
		if (*(bool *) (pContext->pData) == 0)
		{
			IOT_INFO("SelfTest off");
			SelfTest = 0;
		}
		else if (*(bool *) (pContext->pData) == 1)
		{
			IOT_INFO("SelfTest On");
			SelfTest = 1;
		}
	}
}

void SetTemperature_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
	IOT_UNUSED(pJsonString);
	IOT_UNUSED(JsonStringDataLen);
	IOT_INFO("Callback");
	if(pContext != NULL) {
		IOT_INFO("Delta - Temperature changed to %f", *(float *) (pContext->pData));
		SetTemperature = *(float *) (pContext->pData);
	}
}

void SetJSONHandlers(){
	SelfTestHandler.cb = SelfTest_Callback;
	SelfTestHandler.pData = &SelfTestInvoked;
	SelfTestHandler.dataLength = sizeof(bool);
	SelfTestHandler.pKey = "SelfTestInvoked";
	SelfTestHandler.type = SHADOW_JSON_BOOL;
	
	ActiveStateHandler.cb = ActiveState_Callback;
	ActiveStateHandler.pData = &ActiveState;
	ActiveStateHandler.dataLength = sizeof(bool);
	ActiveStateHandler.pKey = "SystemState";
	ActiveStateHandler.type = SHADOW_JSON_BOOL;

	temperatureHandler.cb = NULL;
	temperatureHandler.pKey = "Temperature";
	temperatureHandler.pData = &temperature;
	temperatureHandler.dataLength = sizeof(float);
	temperatureHandler.type = SHADOW_JSON_FLOAT;
	
	SetTemperatureHandler.cb = SetTemperature_Callback;
	SetTemperatureHandler.pKey = "SetTemperature";
	SetTemperatureHandler.pData = &SetTemperature;
	SetTemperatureHandler.dataLength = sizeof(float);
	SetTemperatureHandler.type = SHADOW_JSON_FLOAT;
	
	CurrentHandler.cb = NULL;
	CurrentHandler.pKey = "Current";
	CurrentHandler.pData = &current;
	CurrentHandler.dataLength = sizeof(float);
	CurrentHandler.type = SHADOW_JSON_FLOAT;
	
	TimeHandler.cb = NULL;
	TimeHandler.pKey = "SendDataAt";
	TimeHandler.pData = &TimeDate;
	TimeHandler.dataLength = sizeof(int32_t);
	TimeHandler.type = SHADOW_JSON_INT32;
	
}

void parseInputArgsForConnectParams(int argc, char **argv) {
	int opt;

	while(-1 != (opt = getopt(argc, argv, "h:p:c:n:"))) {
		switch(opt) {
			case 'h':
				strncpy(HostAddress, optarg, HOST_ADDRESS_SIZE);
				//IOT_DEBUG("Host %s", optarg);
				break;
			case 'p':
				port = atoi(optarg);
				//IOT_DEBUG("arg %s", optarg);
				break;
			case 'c':
				strncpy(certDirectory, optarg, PATH_MAX + 1);
				//IOT_DEBUG("cert root directory %s", optarg);
				break;
			case 'n':
				numPubs = atoi(optarg);
				//IOT_DEBUG("num pubs %s", optarg);
				break;
			case '?':
				if(optopt == 'c') {
					//IOT_ERROR("Option -%c requires an argument.", optopt);
				} else if(isprint(optopt)) {
					//IOT_WARN("Unknown option `-%c'.", optopt);
				} else {
					//IOT_WARN("Unknown option character `\\x%x'.", optopt);
				}
				break;
			default:
				//IOT_ERROR("ERROR in command line argument parsing");
				break;
		}
	}

}
/* Functions End */
/*******************************************************************************/