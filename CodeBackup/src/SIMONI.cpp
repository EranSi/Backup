/*
//####################################################
//Connection setup
//                                               |_|
//                                          *           *
//                                          *           *
//                                          *           *
//                                          *           *
//                       micro switch 1 D8  *           *
//                          Micro Switch D7 *           *
//                          Steps        D6 *           *
//                          Dir          D5 *           *
//                                          *           *
//                          3v3             *           *
//                                          *           *
//                                          *           *
//          HomeButton , Yellow LED,     D2 *           *
//          Backword return, RED LED     D1 *           *
//   //                     OnBoardLed   D0 *___________*
//####################################################  */

// Hello  .... . .-.. .-.. ---
#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(x)                 \
    Serial.print(millis());            \
    Serial.print(": ");                \
    Serial.print(__PRETTY_FUNCTION__); \
    Serial.print(" in ");              \
    Serial.print(__FILE__);            \
    Serial.print(":");                 \
    Serial.print(__LINE__);            \
    Serial.print(" ");                 \
    Serial.println(x);
#else
#define DEBUG_PRINT(x)
#endif

#include <Bounce2.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <inttypes.h>
#include "DRV8834.h"
#include "LittleFS.h"

//####################################################
// Connection setup
//####################################################
// Motor definition
//####################################################
#define MotorFullStep 200 // 360 degree / 1.8 degree of a singel step  = 200 [fix]
#define MICROSTEPS 8
#define Pitch_mm 4
#define Pitch_Inch 0.03937009 * Pitch_mm

int ScrowLangh = 300;

//####################################################
//HW pin definition
//####################################################
//Step and direction pin setting
#define Dir_Pin_Driver_1 D5  //
#define Step_Pin_Driver_1 D6 //

// Target RPM for cruise speed
#define RPM_High 300 // for MICROSTEPS = 4 , RPM = 450 ; for MICROSTEPS = 8 , RPM = 550
#define RPM_Low 300
// Acceleration and deceleration values are always in FULL steps / s^2
#define MOTOR_ACCEL 4000
#define MOTOR_DECEL 4000

DRV8834 stepper(MotorFullStep, Dir_Pin_Driver_1, Step_Pin_Driver_1);

#define OnBoardLed D0
#define HomeButton D7
#define HomeMicroSwitch D8
#define ForewordButton D2
#define ReversButton D1

//####################################################
// for ESP wIfI MODULE
//####################################################

ESP8266WebServer server(80);
IPAddress ip(192, 168, 4, 200); // static server IP
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

#define WIFI_ACCESS_POINT
const char *ssid = "SIMONI"; // your ssid
const char *password = "";

//String arratSelection = "Possion";

#define SAW_UNITS_MM 0
#define SAW_UNITS_INCH 1
#define MAX_NUM_SAWS 2
struct saw_configuration
{
    uint16_t index;
    float thickness;
    float thickness_d;
    float thickness_h;
    float thickness_D;
    float steps;
    uint16_t boardtype;
    //uint16_t type;
    uint16_t units;
    //String description;
    int16_t pressure;
    float HomeBmovmentOffset;
};

struct saw_configuration saw_configs[MAX_NUM_SAWS];
uint8_t installed_saw = 0;

//####################################################################################################################
#define arraySize 120
int PossionDifferentAB[2][arraySize];
int PosissionMovment = 1;
float ArrayStep = 0.0;
int Temp[4][arraySize] = {0};

int pressureReference = 200 ;  
float FullStep = (MotorFullStep * MICROSTEPS); //  200   /   0.25 =800 ;  0.125 =1600 ; 0.0625 =3200

// mm definition
float OneStepLength = (Pitch_mm / FullStep); //  4mm   /  800 = 0.005 mm ; 1600 = 0.0025 mm

// Inch definition
float OneStepLengthInch = (Pitch_Inch / FullStep);

float UnitSawDefinition[2] = {OneStepLength, OneStepLengthInch};

//########################################################################
// Instantiate 4 Bounce object
Bounce HomeButtonButtonState = Bounce();
Bounce ForewordButtonState = Bounce();
Bounce ReversButtonState = Bounce();
Bounce HomeMicroSwitchButtonState = Bounce();
#define DELAY_OF_DEBOUNCE 150 // amount of time that needs to be expired between presses

int HomeMicroSwitchButtonRead = LOW;
//########################################################################
//WEB indication  (part of them are not in use)
//########################################################################

// indication of the selected board
bool bIsSawBusy = false;

// defferent Plan selection carecters
//########################################################################
float PlanX = 0.0;
float PlanY = 0.0;
float PlanZ = 0.0;
int SelectPlanNumber = 1;
String plan_name = "";

int remaindSteps = 0;
float remaindStepsA = 0;
float remaindStepsB = 0;
int RepetitionsMovment = 0;
int Incrimantal = 0 ;

//########################################################################

int left = -1;
int right = 1;

//int direction = 0 ;
int reverseCutDirection = 0;
int Direction = right;
int Board_Change = 0;

// change the step motor speed

int counter = 1;
int speedresolution = 1;

// use for A type board
int movmentOffset = 0;
// use for B type board
int HomeBmovmentOffset = 0;

int number_of_repetitions = 1;
float number_of_repetitionsA = 1;
float number_of_repetitionsB = 1.0;
int CompleteStepsA = 0;
int CompleteStepsB = 0;
float CurrentMovment = 0;
int MultipaleStepas = 0;
//float CurrentMovmentHighA = 0.000;
//float SpaceCutBetweenToothB = 0.000;
//float CurrentMovmentLowA = 0.000;

float NewToothSize = 0.0;
int BoardRepetissionNumber = 0;

#define toothThikness 4
float cutStructure[toothThikness];

int x = 0;
int y = 0;
int z = 0;
int PlanNumber = 1;
char RXdata = '1';




void serverStarting();

void load_calib();
void save_calib();
void get_json_calib();
void build_json_calib(String *calibjson);
void parse_json_calib(String calibjson);
int StepsMovment(int);
void blinkLed(int);
void home_releas_func();
void startcut();
void forwardCut();
void reverseCut();
void stepCounterFunc(int StepsNumber, int Direction, int installed_saw);
void printJsonStringToSerial(String json);

void setup()
{

    //********* Serial configuration  *********

	Serial.begin(115200);
    Serial.println("Booting...");
    Serial.println("FW v3.0");
    if (!LittleFS.begin())
    {
        Serial.println("LittleFS Mount failed");
        Serial.println("Reset in 5 seconds..");
        delay(5000);
        ESP.restart();
    }
    else
    {
        Serial.println("LittleFS Mount successful\n");
    }
    //********* JSON configuration  *********
    load_calib();
    //start the server and upload the web pages
    serverStarting();

    //********* start to configure the esp8266 pins *********
    pinMode(OnBoardLed, OUTPUT);
    pinMode(Dir_Pin_Driver_1, OUTPUT);
    pinMode(Step_Pin_Driver_1, OUTPUT);

    //Home micro switch ebable
    pinMode(HomeMicroSwitch, INPUT);

    //Home Button LED ebable
    pinMode(HomeButton, INPUT);
    //Foreword Button push
    pinMode(ForewordButton, INPUT);
    //Revers Button push
    pinMode(ReversButton, INPUT);

    //********* Stepper configuration *********
    stepper.begin(RPM_High, MICROSTEPS);
    stepper.enable();
    stepper.setSpeedProfile(stepper.LINEAR_SPEED, MOTOR_ACCEL, MOTOR_DECEL); //Set LINEAR_SPEED (accelerated) profile.

    //********* Debuncer configuration *********
    HomeButtonButtonState.attach(HomeButton);
    HomeButtonButtonState.interval(DELAY_OF_DEBOUNCE);
    ForewordButtonState.attach(ForewordButton);
    ForewordButtonState.interval(DELAY_OF_DEBOUNCE);
    ReversButtonState.attach(ReversButton);
    ReversButtonState.interval(DELAY_OF_DEBOUNCE);
    HomeMicroSwitchButtonState.attach(HomeMicroSwitch);
    HomeMicroSwitchButtonState.interval(DELAY_OF_DEBOUNCE);

    HomeMicroSwitchButtonRead = HIGH; // disable the micro switch

    //********* return to home if the user left and turnoff the power in the middle *********

    if (saw_configs[installed_saw].steps != 0.00)
    {
        home_releas_func();
    }

    // TODO: Check if needed to startcut here
    // startcut();

    // Hello in Morse code  = .... . .-.. .-.. ---
    //...

    blinkLed(3); //....
    delay(500);

    blinkLed(0); //.
    delay(500);

    blinkLed(0); //.-..
    digitalWrite(OnBoardLed, LOW);
    delay(300);
    blinkLed(1);
    delay(500);

    blinkLed(0); //.-..
    digitalWrite(OnBoardLed, LOW);
    delay(300);
    blinkLed(1);
    delay(500); // ---

    digitalWrite(OnBoardLed, LOW); //---
    delay(300);
    digitalWrite(OnBoardLed, HIGH);
    delay(150);
    digitalWrite(OnBoardLed, LOW);
    delay(300);
    digitalWrite(OnBoardLed, HIGH); //.-..
    delay(150);
    digitalWrite(OnBoardLed, LOW);
    delay(300);
    digitalWrite(OnBoardLed, HIGH);

    delay(1000);
}

void loop()
{
    //# On board LED on
    digitalWrite(OnBoardLed, HIGH);
    HomeButtonButtonState.update();
    ForewordButtonState.update();
    ReversButtonState.update();
    HomeMicroSwitchButtonState.update();

    server.handleClient(); // listens to requests from clients

    if (ForewordButtonState.rose())
    {

        Serial.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
        Serial.print("\n LOOP Foreword is called: ");
        Serial.println("\n @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
        forwardCut();
    }

    if (HomeButtonButtonState.rose())
    {
        Serial.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
        Serial.print("\n LOOP Home is called: ");
        Serial.println("\n @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
        home_releas_func();
        x = 0;
    }

    if (ReversButtonState.rose())
    {

        Serial.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
        Serial.print("\n LOOP ReversButtonRead is called: ");
        Serial.println("\n @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
        reverseCut();
    }
    //----------------------------------------------------------------------------
    // SW reset function
    if (Serial.available() > 0)
    { //check if any data was received
        RXdata = Serial.read();
		if(RXdata=='0'){
            Serial.println("Reset..");
            ESP.restart();
        }
    }
    //-----------------------------------------------------------------------------
}

void rotate(int steps, int Direction)
{
    if (HomeMicroSwitchButtonRead == LOW)
    {
        // if saw is busy, we just bail
        return;
    }

    Serial.print("#######################\n");
    Serial.print(" rotate function\n");
    Serial.print("#######################\n");
    Serial.print("CurrentMovment = ");
    Serial.println(CurrentMovment);

    //int DirectionMode = (Direction > 0) ? -1 : 1 ; //select the pin direction
    //Serial.print("DirectionMode = ");
    //Serial.println(DirectionMode);

    Serial.print("steps = ");
    Serial.println(steps);
    steps = abs(steps); // privent to forword a (-) to the eeprom

    //PWM Function duty cycle set by the usDelay

    digitalWrite(OnBoardLed, LOW);

    stepper.move(Direction * steps);

    stepCounterFunc(steps, Direction, installed_saw);

    digitalWrite(OnBoardLed, HIGH);

    //bIsSawBusy  = false;
    //checksawbusy();
}

void stepCounterFunc(int StepsNumber, int Direction, int installed_saw)
{
    //Serial.print("#######################\n");
    //Serial.print(" stepCounterFunc\n ");
    //Serial.print("#######################\n");

    if (StepsNumber == 0 && Direction == 0)
    { // אפס את מספר הצעדים בהתאם למסור שנבחר
		Serial.print(" set to 0 the steps ");
        saw_configs[installed_saw].steps = 0.0;
		save_calib();
    }
    else
    {
        Serial.print("StepsNumber = ");
        Serial.println(StepsNumber);
        Serial.print("Direction = ");
        Serial.println(Direction);

        //Serial.print("Before update saw_configs[installed_saw].steps = ");
        //Serial.print(saw_configs[installed_saw].steps);

        saw_configs[installed_saw].steps = StepsNumber * Direction + saw_configs[installed_saw].steps;
        Serial.print("update steps to = ");
        Serial.println(saw_configs[installed_saw].steps);

		save_calib();
        //return saw_configs[installed_saw].steps;
    }
}

void home_releas_func()
{
    if (HomeMicroSwitchButtonRead == LOW)
    {
        // if saw is busy, we just bail
        return;
    }

    // Serial.print(" ####################### \n");
    // Serial.print(" inside home_releas function \n");
    // Serial.print(" ####################### \n");
    int home_releas_direction = 0;

	counter = 1;
	if (SelectPlanNumber !=6)
	{
    PosissionMovment = 1;
	}
	else 
	{
	PosissionMovment = 0 ; 	
	}
    //bIsSawBusy  = true;
    //stepper.begin(RPM_High);
    //bIsSawBusy = 1;
    blinkLed(3);

    switch (saw_configs[installed_saw].boardtype)
    {
    case 0:
        Serial.print("Selected board is *** A *** \n");
        if (saw_configs[installed_saw].steps >= 0)
        {
            home_releas_direction = left;
        }
        else
        {
            home_releas_direction = right;
        }

        //use case 1:
        // 1. selected board A and the user push on home button
        // 2.
        // 3. reset the position to A Board by reading the current steps number inside the eeprom
        rotate(saw_configs[installed_saw].steps, home_releas_direction);

        //comment the stepCounterFunc since it allready set the step to "0" outcome form the steps calculation
        stepCounterFunc(0, 0, installed_saw);
        break;

    case 1:
        Serial.print("Selected board is *** B *** \n");


        if (saw_configs[installed_saw].steps >= 0)
        {
            home_releas_direction = left;
        }
        else
        {
            home_releas_direction = right;
        }

        //use case 3:
        // 1. selected board is B Or the user push on home button will the B board was allready selected
        // 3. reset the position to B Board by reading the current steps number inside the eeprom

        rotate(saw_configs[installed_saw].steps, home_releas_direction);
        //comment the stepCounterFunc since it allready set the step to "0" outcome form the steps calculation
        stepCounterFunc(0, 0, installed_saw);
        break;
    }
    digitalWrite(OnBoardLed, HIGH);
}

void speedSelect()
{
    StaticJsonDocument<500> jsonBuffer;
    deserializeJson(jsonBuffer, server.arg("plain"));

    String speedStr = jsonBuffer["speed"];
    int speed = speedStr.toInt();
    int InternalSpeed = 0;

    switch (speed)
    {
    case 1:
        Serial.println("X1");
        InternalSpeed = 10;
        break;

    case 10:
        Serial.println("X10");
        InternalSpeed = 50;
        break;

    case 100:
        Serial.println("X100");
        InternalSpeed = 200;
        break;

    default:
        Serial.println("Invalid speed sent back");
    }

    speedresolution = InternalSpeed;
    server.send(200, "application/text", "Speed X" + String(speed) + " Selected");
}

void boardtypeSelect()
{
    StaticJsonDocument<500> jsonBuffer;
    deserializeJson(jsonBuffer, server.arg("plain"));
    String boardTypeStr = jsonBuffer["boardType"];
    server.send(200, "application/text", "Board " + boardTypeStr + " Selected");

    Serial.print("\n\n  boardtypeSelect \n\n");

    saw_configs[installed_saw].boardtype = boardTypeStr.toInt();

    Serial.print(" Board type select = ");
    //Serial.print(" saw_configs[installed_saw].boardtype = ");
    Serial.println(saw_configs[installed_saw].boardtype);

    Serial.print("installed_saw = ");
    Serial.println(installed_saw);

    home_releas_func();
}

void forwardCut()
{
    Serial.print("\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ \n forwardCut \n @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    Serial.print("selected Plan = ");
    Serial.println(SelectPlanNumber);

    //float z = 0.0;
    if (Direction == left)
    {
        Serial.print("Direction (left) = ");
        Serial.println(Direction);
        //Serial.print("\n");
        rotate(StepsMovment(Direction), Direction);
        Serial.println("END (left)");
    }
    else
    {
        Serial.print("Direction (right) = ");
        Serial.println(Direction);
        rotate(StepsMovment(Direction), Direction);
        Serial.println("END (Right)");
    }
    server.send(200, "application/json", "{\"is_saw_busy\":\"false\"}");
}

int StepsMovment(int dir)
{
    if (HomeMicroSwitchButtonRead == LOW)
    {
        // if saw is busy, we just bail
        return 0;
    }

    //startcut();
    Serial.print("\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ StepsMovment @@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
    //printf("\n dir = ,  %i ", dir);

    switch (SelectPlanNumber)
    {
    case 1: // *Asymmetrical* Out Of the Box Cat
    {
        // Serial.print("plan_name =");
        // Serial.println(plan_name);
        Serial.println("case = 1 *Asymmetrical* Out Of the Box");

        if (dir == right)
        {
            for (int n = 1; n < 10; n++)
            {
                Serial.println(PossionDifferentAB[saw_configs[installed_saw].boardtype][n]);
            }
            ArrayStep = PossionDifferentAB[saw_configs[installed_saw].boardtype][PosissionMovment];
            //ArrayStep = PossionDifferentAB[0][PosissionMovment]*CurrentMovment;
            PosissionMovment = PosissionMovment + 1;
            //Serial.print("PosissionMovment = ");
            //Serial.println(PosissionMovment);
            CurrentMovment = saw_configs[installed_saw].thickness / (UnitSawDefinition[saw_configs[installed_saw].units]); // for example 3.2mm saw tichkness / 0.005 equel to 640 motore steps
            HomeBmovmentOffset = (number_of_repetitions)*CurrentMovment;
        }

        if (dir == left)
        {
            for (int n = 1; n < 10; n++)
            {
                Serial.println(PossionDifferentAB[saw_configs[installed_saw].boardtype][n]);
            }
            //verify you dont exit the '0' point and have negative steps and array opssion
            if (saw_configs[installed_saw].steps <= 0.00)
            {
                PosissionMovment = 1;
                ArrayStep = 0;
                //Serial.print("PosissionMovment = ");
                //Serial.println(PosissionMovment);
                //Serial.print("ArrayStep =");
                //Serial.println(ArrayStep);
                //Serial.print("Possion[PosissionMovment] ");
                //Serial.println(Possion[PosissionMovment]);
            }
            else
            {

                //Serial.print("PosissionMovment = ");
                //Serial.println(PosissionMovment);
                PosissionMovment = PosissionMovment - 1;
                //Serial.print("PosissionMovment = ");
                //Serial.println(PosissionMovment);
                //Serial.println("Possion[PosissionMovment]*CurrentMovment");
                //Serial.println(Possion[PosissionMovment]*CurrentMovment);
                //ArrayStep = PossionDifferentAB[0][PosissionMovment]*CurrentMovment;
                ArrayStep = PossionDifferentAB[saw_configs[installed_saw].boardtype][PosissionMovment];
                Serial.print("ArrayStep =");
                Serial.println(ArrayStep);
            }
        }
        return ArrayStep;
    }
    break;

    case 2: // *Asymmetrical* Finger Joint for any given tooth size
    {
        Serial.println("case = 2 *Asymmetrical*");
        Serial.print("SelectPlanNumber =");
        Serial.println(SelectPlanNumber);
        Serial.print("PosissionMovment = ");
        Serial.println(PosissionMovment);

        if (dir == right)
        {
            Serial.print("SelectPlanNumber =");
            Serial.println(SelectPlanNumber);
            Serial.print("PosissionMovment = ");
            Serial.println(PosissionMovment);
            for (int n = 1; n < 10; n++)
            {
                Serial.println(PossionDifferentAB[saw_configs[installed_saw].boardtype][n]);
            }

            ArrayStep = PossionDifferentAB[saw_configs[installed_saw].boardtype][PosissionMovment];
            PosissionMovment = PosissionMovment + 1;
            Serial.print("PosissionMovment = ");
            Serial.println(PosissionMovment);
            Serial.print("ArrayStep =");
            Serial.println(ArrayStep);
            // Serial.print("Possion[PosissionMovment] ");
            // Serial.println(Possion[PosissionMovment]);
        }

        if (dir == left)
        {
            Serial.println("if (dir == left)");
            for (int n = 1; n < 10; n++)
            {
                Serial.println(PossionDifferentAB[saw_configs[installed_saw].boardtype][n]);
            }
            //verify you dont exit the '0' point and have negative steps and array opssion
            if (saw_configs[installed_saw].steps <= 0.00)
            {
                PosissionMovment = 1;
                ArrayStep = 0;
                // Serial.print("PosissionMovment = ");
                // Serial.println(PosissionMovment);
                // Serial.print("ArrayStep =");
                // Serial.println(ArrayStep);
                // Serial.print("Possion[PosissionMovment] ");
                // Serial.println(Possion[PosissionMovment]);
                return ArrayStep;
            }
            PosissionMovment = PosissionMovment - 1;
            ArrayStep = PossionDifferentAB[saw_configs[installed_saw].boardtype][PosissionMovment];
            Serial.print("PosissionMovment = ");
            Serial.println(PosissionMovment);
            Serial.println("PossionDifferentAB[0][PosissionMovment]");
            Serial.println(PossionDifferentAB[saw_configs[installed_saw].boardtype][PosissionMovment]);
        }
        return ArrayStep;
    }
    break;

    case 3: //   **Symmetrical** Finger Joint for any given tooth
    {
        Serial.println("case = 3 **Symmetrical** Finger Joint");
        Serial.print("SelectPlanNumber =");
        Serial.println(SelectPlanNumber);
        Serial.print("PosissionMovment = ");
        Serial.println(PosissionMovment);
        if (dir == right)
        {

            for (int n = 1; n < 10; n++)
            {
                Serial.println(PossionDifferentAB[saw_configs[installed_saw].boardtype][n]);
            }
            ArrayStep = PossionDifferentAB[saw_configs[installed_saw].boardtype][PosissionMovment];
            PosissionMovment = PosissionMovment + 1;
            Serial.print("PosissionMovment = ");
            Serial.println(PosissionMovment);
            Serial.print("ArrayStep =");
            Serial.println(ArrayStep);
            // Serial.print("Possion[PosissionMovment] ");
            // Serial.println(Possion[PosissionMovment]);
        }
        if (dir == left)
        {
            Serial.println("if (dir == left)");

            for (int n = 1; n < 10; n++)
            {
                Serial.println(PossionDifferentAB[saw_configs[installed_saw].boardtype][n]);
            }

            if (saw_configs[installed_saw].steps <= 0.00) //verify you dont exit the '0' point and have negative steps and array opssion
            {
                PosissionMovment = 1;
                ArrayStep = 0;
                // Serial.print("PosissionMovment = ");
                // Serial.println(PosissionMovment);
                // Serial.print("ArrayStep =");
                // Serial.println(ArrayStep);
                // Serial.print("Possion[PosissionMovment] ");
                // Serial.println(Possion[PosissionMovment]);
                return ArrayStep;
            }
            PosissionMovment = PosissionMovment - 1;
            ArrayStep = PossionDifferentAB[saw_configs[installed_saw].boardtype][PosissionMovment];
            Serial.print("PosissionMovment = ");
            Serial.println(PosissionMovment);
            Serial.println("PossionDifferentAB[0][PosissionMovment]");
            Serial.println(PossionDifferentAB[saw_configs[installed_saw].boardtype][PosissionMovment]);
        }
        return ArrayStep;
    }
    break;

    case 4: // incremental_cut
    {
        Serial.println("case = 4 incremental_cut");
        Serial.print("SelectPlanNumber =");
        Serial.println(SelectPlanNumber);

        if (dir == right)
        {
            for (int n = 1; n < 10; n++)
            {
                Serial.println(PossionDifferentAB[saw_configs[installed_saw].boardtype][n]);
            }
            ArrayStep = PossionDifferentAB[saw_configs[installed_saw].boardtype][PosissionMovment];
            PosissionMovment = PosissionMovment + 1;
            Serial.print("PosissionMovment = ");
            Serial.println(PosissionMovment);
            Serial.print("ArrayStep =");
            Serial.println(ArrayStep);
            // Serial.print("Possion[PosissionMovment] ");
            // Serial.println(Possion[PosissionMovment]);
            return ArrayStep;
        }
        if (dir == left)
        {
            Serial.println("if (dir == left)");

            for (int n = 1; n < 10; n++)
            {
                Serial.println(PossionDifferentAB[saw_configs[installed_saw].boardtype][n]);
            }

            if (saw_configs[installed_saw].steps <= 0.00) //verify you dont exit the '0' point and have negative steps and array opssion
            {
                PosissionMovment = 1;
                ArrayStep = 0;
                // Serial.print("PosissionMovment = ");
                // Serial.println(PosissionMovment);
                // Serial.print("ArrayStep =");
                // Serial.println(ArrayStep);
                // Serial.print("Possion[PosissionMovment] ");
                // Serial.println(Possion[PosissionMovment]);
                return ArrayStep;
            }
            PosissionMovment = PosissionMovment - 1;
            ArrayStep = PossionDifferentAB[saw_configs[installed_saw].boardtype][PosissionMovment];
            Serial.print("PosissionMovment = ");
            Serial.println(PosissionMovment);
            Serial.println("PossionDifferentAB[0][PosissionMovment]");
            Serial.println(PossionDifferentAB[saw_configs[installed_saw].boardtype][PosissionMovment]);
            return ArrayStep;
        }
    }
    break;

    case 5: // Custom
    {

        Serial.println("case = 5 Custom ");
        Serial.print("SelectPlanNumber =");
        Serial.println(SelectPlanNumber);
        Serial.print("PosissionMovment = ");
        Serial.println(PosissionMovment);
        

        if (dir == right)
        {
				for (int n  = 1; n < 20 ; n++)
            {
                Serial.println(PossionDifferentAB[saw_configs[installed_saw].boardtype][n]);
            }
            ArrayStep = PossionDifferentAB[saw_configs[installed_saw].boardtype][PosissionMovment];
            PosissionMovment = PosissionMovment + 1;
            Serial.print("PosissionMovment = ");
            Serial.println(PosissionMovment);
            Serial.print("ArrayStep =");
            Serial.println(ArrayStep);
            // Serial.print("Possion[PosissionMovment] ");
            // Serial.println(Possion[PosissionMovment]);
            return ArrayStep;
        }
        if (dir == left)
        {
            Serial.println("if (dir == left)");

            for (int n = 1; n < 10; n++)
            {
                Serial.println(PossionDifferentAB[saw_configs[installed_saw].boardtype][n]);
            }

            if (saw_configs[installed_saw].steps <= 0.00) //verify you dont exit the '0' point and have negative steps and array opssion
            {
                PosissionMovment = 1;
                ArrayStep = 0;
                // Serial.print("PosissionMovment = ");
                // Serial.println(PosissionMovment);
                // Serial.print("ArrayStep =");
                // Serial.println(ArrayStep);
                // Serial.print("Possion[PosissionMovment] ");
                // Serial.println(Possion[PosissionMovment]);
                return ArrayStep;
            }
            PosissionMovment = PosissionMovment - 1;
            ArrayStep = PossionDifferentAB[saw_configs[installed_saw].boardtype][PosissionMovment];
            Serial.print("PosissionMovment = ");
            Serial.println(PosissionMovment);
            Serial.println("PossionDifferentAB[0][PosissionMovment]");
            Serial.println(PossionDifferentAB[saw_configs[installed_saw].boardtype][PosissionMovment]);
            return ArrayStep;
        }
    }
    break;

    case 6: // Bridle Joint

    {
		Serial.println("case = 6 **Bridle Joint** ");
        Serial.print("SelectPlanNumber =");
        Serial.println(SelectPlanNumber);
        Serial.print("PosissionMovment = ");
        Serial.println(PosissionMovment);
        if (dir == right)
        {
				for (int n  = 0; n < 10 ; n++)
            {
                Serial.println(PossionDifferentAB[saw_configs[installed_saw].boardtype][n]);
            }
            ArrayStep = PossionDifferentAB[saw_configs[installed_saw].boardtype][PosissionMovment];
            PosissionMovment = PosissionMovment + 1;
            Serial.print("PosissionMovment = ");
            Serial.println(PosissionMovment);
            Serial.print("ArrayStep =");
            Serial.println(ArrayStep);
            // Serial.print("Possion[PosissionMovment] ");
            // Serial.println(Possion[PosissionMovment]);
            return ArrayStep;
        }
        if (dir == left)
        {
            Serial.println("if (dir == left)");
				for (int n  = 0; n < 10 ; n++)
            {
                Serial.println(PossionDifferentAB[saw_configs[installed_saw].boardtype][n]);
            }
            if (saw_configs[installed_saw].steps <= 0.00) //verify you dont exit the '0' point and have negative steps and array opssion
            {
				PosissionMovment = 0 ;
                ArrayStep = 0;
                // Serial.print("PosissionMovment = ");
                // Serial.println(PosissionMovment);
                // Serial.print("ArrayStep =");
                // Serial.println(ArrayStep);
                // Serial.print("Possion[PosissionMovment] ");
                // Serial.println(Possion[PosissionMovment]);
                return ArrayStep;
            }
            PosissionMovment = PosissionMovment - 1;
            ArrayStep = PossionDifferentAB[saw_configs[installed_saw].boardtype][PosissionMovment];
            Serial.print("PosissionMovment = ");
            Serial.println(PosissionMovment);
            Serial.println("PossionDifferentAB[0][PosissionMovment]");
            Serial.println(PossionDifferentAB[saw_configs[installed_saw].boardtype][PosissionMovment]);
            return ArrayStep;
        }
    }
    break;

    case 7: // **Symmetrical** DoveTail Flat Joint
    {
        Serial.println("case = 6 Bridle Joint");
        Serial.print("SelectPlanNumber =");
        Serial.println(SelectPlanNumber);
        Serial.print("PosissionMovment = ");
        Serial.println(PosissionMovment);
        Serial.print("saw_configs[1].boardtype = ");
        Serial.println(saw_configs[1].boardtype);

        if (dir == right)
        {

            ArrayStep = PossionDifferentAB[saw_configs[1].boardtype][PosissionMovment];
            PosissionMovment = PosissionMovment + 1;
            Serial.print("PosissionMovment = ");
            Serial.println(PosissionMovment);
            Serial.print("ArrayStep =");
            Serial.println(ArrayStep);

            for (int n = 1; n < 10; n++)
            {
                Serial.println(PossionDifferentAB[saw_configs[installed_saw].boardtype][n]);
            }

            // Serial.print("Possion[PosissionMovment] ");
            // Serial.println(Possion[PosissionMovment]);
        }

        if (dir == left)
        {
            Serial.println("if (dir == left)");
            for (int n = 1; n < 10; n++)
            {
                Serial.println(PossionDifferentAB[saw_configs[installed_saw].boardtype][n]);
            }

            if (saw_configs[1].steps <= 0.00) //verify you dont exit the '0' point and have negative steps and array opssion
            {
                PosissionMovment = 0;
                ArrayStep = 0;
                // Serial.print("PosissionMovment = ");
                // Serial.println(PosissionMovment);
                // Serial.print("ArrayStep =");
                // Serial.println(ArrayStep);
                // Serial.print("Possion[PosissionMovment] ");
                // Serial.println(Possion[PosissionMovment]);
                return ArrayStep;
            }
            PosissionMovment = PosissionMovment - 1;
            ArrayStep = PossionDifferentAB[saw_configs[1].boardtype][PosissionMovment];
            Serial.print("PosissionMovment = ");
            Serial.println(PosissionMovment);
            Serial.print("ArrayStep = ");
            Serial.println(ArrayStep);
            Serial.println("PossionDifferentAB[saw_configs[1].boardtype][PosissionMovment]");
            Serial.println(PossionDifferentAB[saw_configs[1].boardtype][PosissionMovment]);
        }
        return ArrayStep;
    }
    break;
    }
    return 0;
}

void reverseCut()
{
    //bIsSawBusy  = true;
    Serial.println("Reverse cut");

    Direction = left;
    forwardCut();
    Direction = right;
}

void home()
{
    Serial.println("Home");
    Serial.print("\n\n inside home_releas presed button");
    Serial.print("\n#######################\n");
    //bIsSawBusy  = true;
    home_releas_func();
    server.send(200, "application/json", "{\"is_saw_busy\":\"false\"}");
}

void setHome()
{
    blinkLed(3);

    Serial.println("Set home position");
    stepCounterFunc(0, 0, installed_saw);

    digitalWrite(OnBoardLed, HIGH);
}

void directionSelect()
{
    digitalWrite(OnBoardLed, LOW);
    StaticJsonDocument<500> jsonBuffer;
    deserializeJson(jsonBuffer, server.arg("plain"));

    String directionStr = jsonBuffer["direction"];
    int AppDirection = directionStr.toInt();
    AppDirection = AppDirection * -1; // to set the correct direction
    Serial.println("User selected AppDirection " + directionStr);

    Serial.print("Steps [prior] = ");
    Serial.println(saw_configs[installed_saw].steps);
    stepper.move(AppDirection * speedresolution);

    digitalWrite(OnBoardLed, HIGH);
    server.send(200, "application/text", "Direction " + directionStr + " set");
}

float SymmetricalDistance(float ToothThickness, float BoardSize)
{
    Serial.print("\n\n\n SymmetricalDistance Begin \n\n\n ");

    Serial.print("ToothThickness = ");
    Serial.println(ToothThickness);
    Serial.print("BoardSize = ");
    Serial.println(BoardSize);

    float repetitionCounter = BoardSize / ToothThickness;
    BoardRepetissionNumber = BoardSize / ToothThickness;
    float Remaind = repetitionCounter - BoardRepetissionNumber;
    float RemainTooth = Remaind / BoardRepetissionNumber * ToothThickness;
    NewToothSize = RemainTooth + ToothThickness;

    Serial.print("repetitionCounter = ");
    Serial.println(repetitionCounter);
    Serial.print("Remaind = ");
    Serial.println(Remaind);
    Serial.print("RemainTooth = ");
    Serial.println(RemainTooth);
    Serial.print("NewToothSize = ");
    Serial.println(NewToothSize);
    Serial.print("BoardRepetissionNumber = ");
    Serial.println(BoardRepetissionNumber);

    //eran = 3;
    Serial.print("\n\n\n SymmetricalDistance END \n\n\n ");
    return BoardRepetissionNumber;
}

float toothThiknesCutRepetition(float RequiredToothThickness, float installedSawThickness)
{
    //installed_saw = 0;
    Serial.print("\n\n\n toothThiknesCutRepetition Begin \n\n\n ");

    Serial.print("RequiredToothThickness = ");
    Serial.println(RequiredToothThickness);
    Serial.print("installedSawThickness = ");
    Serial.println(installedSawThickness);

    float repetitionCut = RequiredToothThickness / installedSawThickness;
    int repetissionNoRemaind = RequiredToothThickness / installedSawThickness;
    float Remaind = repetitionCut - repetissionNoRemaind;
    float RemainTooth = Remaind * installedSawThickness / UnitSawDefinition[saw_configs[installed_saw].units];

    Serial.print("repetitionCut = ");
    Serial.println(repetitionCut);
    Serial.print("repetissionNoRemaind = ");
    Serial.println(repetissionNoRemaind);
    Serial.print("Remaind = ");
    Serial.println(Remaind);
    Serial.print("RemainTooth = ");
    Serial.println(RemainTooth);

    cutStructure[0] = repetissionNoRemaind;

    if (Remaind != 0)
    {
        repetissionNoRemaind = repetissionNoRemaind + 1;
    }
    cutStructure[1] = repetissionNoRemaind;
    cutStructure[2] = RemainTooth;

    Serial.print("\n\n\n toothThiknesCutRepetition END \n\n\n ");
    return repetissionNoRemaind;
}

void startcut()
{
    int Xrepetission = 0;
    // int formula = 0 ;
    // float StepSizeA = 0.0;
    // float StepSizeB = 0.0;

    StaticJsonDocument<500> jsonBuffer;
    deserializeJson(jsonBuffer, server.arg("plain"));
    server.send(200, "text/plain", "Start cutting!!!"); //TODO: Check if need to respond in the end of the function

    String plan = jsonBuffer["plan"];
    String x_str = jsonBuffer["x"];
    String y_str = jsonBuffer["y"];
    String z_str = jsonBuffer["z"];

    String plan_name_mode = jsonBuffer["planb_mode"]; //TODO: To be removed. there is no parameter "planb_mode" from JS

    plan_name = plan_name_mode;

    SelectPlanNumber = plan.toInt();
    Serial.print("Plan selected inside startcut= ");
    Serial.println(SelectPlanNumber);

    PlanX = x_str.toFloat();
    PlanY = y_str.toFloat();
    PlanZ = z_str.toFloat();

    Serial.println("x = ");
    Serial.println(PlanX);
    Serial.println("y = ");
    Serial.println(PlanY);
    Serial.println("z = ");
    Serial.println(PlanZ);

    // fill up all the array content with 1

    for (int i = 0; i <= 1; i++)
    {
        for (int n = 0; n < arraySize - 1; n++)
        {
            PossionDifferentAB[i][n] = 1;
        }
    }

	Serial.print("installed_saw= ");
    Serial.println(installed_saw);
    if (installed_saw == 0)
    {
        Serial.print("saw_configs[0].thickness = ");
        Serial.println(saw_configs[0].thickness);
        
    }
    else
    {
        Serial.print("saw_configs[1].thickness_d = Alfha Degree");
        Serial.println(saw_configs[1].thickness_d);
        Serial.print("saw_configs[1].thickness_h = Cut Height");
        Serial.println(saw_configs[1].thickness_h);
        Serial.print("saw_configs[1].thickness_D = Bit Width");
        Serial.println(saw_configs[1].thickness_D);
    }


    switch (SelectPlanNumber)
    {
    case 1: // *Asymmetrical* Out Of the Box Cat
    {

        installed_saw = 0; //TODO: Need to be removed
        number_of_repetitions = 1;
        CurrentMovment = saw_configs[installed_saw].thickness / (UnitSawDefinition[saw_configs[installed_saw].units]);
        RepetitionsMovment = (number_of_repetitions + 1) * CurrentMovment;
        saw_configs[installed_saw].HomeBmovmentOffset = RepetitionsMovment;

        PossionDifferentAB[0][1] = CurrentMovment;
        PossionDifferentAB[1][1] = CurrentMovment * 2;
        for (int n = 2; n < arraySize - 1; n++)
        {
            PossionDifferentAB[0][n] = 2 * CurrentMovment - (saw_configs[installed_saw].pressure/10*pressureReference) ;
            PossionDifferentAB[1][n] = 2 * CurrentMovment;

				 if (n*saw_configs[installed_saw].thickness >= ScrowLangh)
            {
                n = arraySize + 1; // stop the loop
                                   //Serial.println("n=arraySize + 1");
            }
        }
    }
    break;

    case 2: // *Asymmetrical* Finger Joint for any given tooth size
    {

        // Asymmetrical Finger Joint for any given tooth size
        Serial.print("\n\n\n@@@@@@@@@@@@@@@@@@@@@@@@@ 2.0 Asymmetrical Finger Joint  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n\n");
        Serial.print("SelectPlanNumber = ");
        Serial.println(SelectPlanNumber);
        installed_saw = 0; // i have a bug if the user select and change the instal saw , so hard coode it

        // return the toothRemind  + number Tooth Repetition
        toothThiknesCutRepetition(PlanX, saw_configs[installed_saw].thickness);

        //int ToothRepetitionsMovmentOrig = cutStructure[0];
        int ToothRepetitionsMovment = cutStructure[1];
        float toothRemind = cutStructure[2];

        CurrentMovment = saw_configs[installed_saw].thickness / (UnitSawDefinition[saw_configs[installed_saw].units]);
        float ToothThickness = PlanX / (UnitSawDefinition[saw_configs[installed_saw].units]); // the number of stepps

        for (int n = 0; n < arraySize - 1; n++)
        {
            PossionDifferentAB[0][n] = CurrentMovment;
            PossionDifferentAB[1][n] = CurrentMovment;
            //Serial.print("PossionDifferentAB[0][n] = ");
            //Serial.println( PossionDifferentAB[0][n]);
        }

        //debug print
        Serial.print("ToothThickness = ");
        Serial.println(ToothThickness);
        Serial.print("ToothRepetitionsMovment = ");
        Serial.println(ToothRepetitionsMovment);
        Serial.print("CurrentMovment = ");
        Serial.println(CurrentMovment);
        Serial.print("toothRemind = ");
        Serial.println(toothRemind);

        //Array step filed up
        PossionDifferentAB[1][1] = ToothThickness + CurrentMovment;

        for (int n = 1; n < arraySize; n++)
        {

            Xrepetission = (ToothRepetitionsMovment + 1) + ((n - 1) * (ToothRepetitionsMovment));

            Serial.print("Xrepetission = ");
            Serial.println(Xrepetission);
            if (Xrepetission >= arraySize - 1)
            {
                n = arraySize;
                //exit ;
            }

            PossionDifferentAB[0][Xrepetission] = ToothThickness + CurrentMovment;
            PossionDifferentAB[1][Xrepetission - ToothRepetitionsMovment] = ToothThickness + CurrentMovment;
            if (toothRemind != 0)
            {
                PossionDifferentAB[0][Xrepetission - 1] = toothRemind;
                PossionDifferentAB[1][Xrepetission - ToothRepetitionsMovment - 1] = toothRemind;
            }
            else
            {
                PossionDifferentAB[0][Xrepetission - 1] = CurrentMovment;
                PossionDifferentAB[1][Xrepetission - ToothRepetitionsMovment - 1] = CurrentMovment;
            }
        }
        // array printing
        Serial.println("*********PossionDifferentAB[0][n]********");
        for (int n = 1; n < 20; n++)
        {
            Serial.println(PossionDifferentAB[0][n]);
        }

        Serial.println("*********PossionDifferentAB[1][n]********");
        for (int n = 1; n < 20; n++)
        {
            Serial.println(PossionDifferentAB[1][n]);
        }

        Serial.print("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 2.0 Asymmetrical Finger Joint @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n\n");
    }
    break;

    case 3: // **Symmetrical** Finger Joint for any given tooth size and Board Lenght
    {
        installed_saw = 0;
        // Symmetrical Finger Joint
        Serial.print("\n\n\n@@@@@@@@@@@@@@@@@@@@@@@@@ 3 Symmetrical Finger Joint   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n\n");
        Serial.print("SelectPlanNumber = ");
        Serial.println(SelectPlanNumber);

        Serial.print(" PlanX = ToothThickness = ");
        Serial.println(PlanX);
        Serial.print("PlanY = BoardSize = ");
        Serial.println(PlanY);

        SymmetricalDistance(PlanX, PlanY); //(float ToothThickness , float BoardSize)

        Serial.print("NewToothSize = ");
        Serial.println(NewToothSize);
        Serial.print("BoardRepetissionNumber = ");
        Serial.println(BoardRepetissionNumber);

        // return the toothRemind  + number Tooth Repetition
        toothThiknesCutRepetition(NewToothSize, saw_configs[installed_saw].thickness);

        //int ToothRepetitionsMovmentOrig = cutStructure[0];
        int ToothRepetitionsMovment = cutStructure[1];
        float toothRemind = cutStructure[2];

        CurrentMovment = saw_configs[installed_saw].thickness / (UnitSawDefinition[saw_configs[installed_saw].units]);
        float ToothThickness = NewToothSize / (UnitSawDefinition[saw_configs[installed_saw].units]); // the number of stepps

        // to currect the result of odd/2 = X.5
        if (BoardRepetissionNumber % 2 == 0)
        {
            Serial.println("BoardRepetissionNumber = Even ");
        }
        else
        {
            BoardRepetissionNumber = (BoardRepetissionNumber + 1); // Odd
            Serial.println("BoardRepetissionNumber = Odd ");
        }
        ///

        for (int n = 0; n < arraySize - 1; n++)
        {
            PossionDifferentAB[0][n] = CurrentMovment;
            PossionDifferentAB[1][n] = CurrentMovment;
            Serial.print("PossionDifferentAB[0][n] = ");
            Serial.println(PossionDifferentAB[0][n]);
        }

        //debug print
        Serial.print("ToothThickness = ");
        Serial.println(ToothThickness);
        Serial.print("ToothRepetitionsMovment = ");
        Serial.println(ToothRepetitionsMovment);
        Serial.print("CurrentMovment = ");
        Serial.println(CurrentMovment);
        Serial.print("toothRemind = ");
        Serial.println(toothRemind);
        Serial.print("BoardRepetissionNumber = ");
        Serial.println(BoardRepetissionNumber);

        float arrayCount = 0; // Count the progress of the steps

        //Array step filed up
        PossionDifferentAB[1][1] = ToothThickness + CurrentMovment;
        for (int n = 1; n < arraySize; n++)
        {

            Xrepetission = (ToothRepetitionsMovment + 1) + ((n - 1) * (ToothRepetitionsMovment));

            Serial.print("Xrepetission = ");
            Serial.println(Xrepetission);
            Serial.print("arrayCount = ");
            Serial.println(arrayCount);
            PossionDifferentAB[0][Xrepetission] = ToothThickness + CurrentMovment;
            PossionDifferentAB[1][Xrepetission - ToothRepetitionsMovment] = ToothThickness + CurrentMovment;
            if (toothRemind != 0)
            {
                PossionDifferentAB[0][Xrepetission - 1] = toothRemind;
                PossionDifferentAB[1][Xrepetission - ToothRepetitionsMovment - 1] = toothRemind;
            }
            else
            {
                PossionDifferentAB[0][Xrepetission - 1] = CurrentMovment;
                PossionDifferentAB[1][Xrepetission - ToothRepetitionsMovment - 1] = CurrentMovment;
            }
            for (int i = 1; i <= Xrepetission; i++)
            {
                arrayCount = arrayCount + PossionDifferentAB[0][i];
            }
            Serial.print("arrayCount = ");
            Serial.println(arrayCount);
            Serial.print("PlanY = ");
            Serial.println(PlanY / UnitSawDefinition[saw_configs[installed_saw].units]);

            Serial.print("n = ");
            Serial.println(n);

            if (PlanY / UnitSawDefinition[saw_configs[installed_saw].units] < arrayCount)
            {
                Serial.print("\n\n\n exit \n\n");
                n = arraySize + 1;
                //exit ;
            }
            arrayCount = 0; // reset and start to conut up to new Xrepetission number
        }
        // array printing
        Serial.println("*********PossionDifferentAB[0][n]********");
        Serial.println(Xrepetission);
        for (int n = 1; n <= Xrepetission; n++)
        {
            Serial.println(PossionDifferentAB[0][n]);
        }

        Serial.println("*********PossionDifferentAB[1][n]********");
        for (int n = 1; n <= Xrepetission; n++)
        {
            Serial.println(PossionDifferentAB[1][n]);
        }

        Serial.print("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 3 Symmetrical Finger Joint @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n\n");
    }
    break;

    case 4: // incremental_cut
    {
        Serial.print("\n\n\n@@@@@@@@@@@@@@@@@@@@@@@@@ startcut 4: // incremental_cut @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");

        int a0 = PlanX; // initial width  in saw tooth units
        int d = PlanX;  // step size in saw tooth units
        int n = 0;      // term counter, tart term is zero
        int a = a0 + n * d;
        int stepsCounter = 0;
        installed_saw = 0;
        //Incrimantal = PlanX;
        CurrentMovment = saw_configs[installed_saw].thickness / (UnitSawDefinition[saw_configs[installed_saw].units]);

        saw_configs[installed_saw].HomeBmovmentOffset = RepetitionsMovment;

        Serial.print("PlanX =");
        Serial.println(PlanX);

        /*for (int n = 1; n < arraySize ; n++)
			{
				Serial.println(PossionDifferentAB[0][n]=CurrentMovment);
				Serial.println(PossionDifferentAB[1][n]=CurrentMovment);
			} 			
			Serial.println("\n\n\n ########### ");
			*/

        // For A board type
        for (int i = 1; i < arraySize; i++)
        {
            // for (n = 0; n<stepVect.size(); n++)
            PossionDifferentAB[0][i] = CurrentMovment;
            if (stepsCounter == a)
            {
                PossionDifferentAB[0][i] = ((a + 1) * CurrentMovment);
                stepsCounter = 1; // resets steps Counter
                n = n + 1;        // calculate new poaition in series
                a = a0 + n * d;   // calcualte new a and maxSteps
            }
            else
            {
                stepsCounter = stepsCounter + 1;
            }
        }

        //stepsCounter = 1 ;

        // For B board type
        n = 0;
        a = a0 + n * d;
        stepsCounter = 0;

        for (int i = 1; i < arraySize; i++)
        {
            PossionDifferentAB[1][i] = CurrentMovment;
            // for (n = 0; n<stepVect.size(); n++)
            if (stepsCounter == (a - PlanX))
            {
                PossionDifferentAB[1][i] = ((a + 1) * CurrentMovment);
                stepsCounter = 1; // resets steps Counter
                n = n + 1;        // calculate new poaition in series
                a = a0 + n * d;   // calcualte new a and maxSteps
            }
            else
            {
                stepsCounter = stepsCounter + 1;
            }
        }

        for (int z = 0; z <= 1; z++)

        {
            Serial.println("Incrimental Arry \n\n");
            for (int i = 1; i < 20; i++)
            {
                Serial.println(PossionDifferentAB[z][i]);
            }
        }

        stepsCounter = 0;
    }
    break;

    case 5: // Custom shape 
    {
        Serial.print("\n\n\n@@@@@@@@@@@@@@@@@@@@@@@@@ startcut 5: // Custom  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");

        Serial.print("SelectPlanNumber = ");
        Serial.println(SelectPlanNumber);
        //int arrayCount = 0 ;
        installed_saw = 0;
        CurrentMovment = saw_configs[installed_saw].thickness / (UnitSawDefinition[saw_configs[installed_saw].units]);

        // user input

			float CentralCut = PlanY ;
			float TotalBoardLength = PlanX ;
			float ToothSize = PlanZ ;
			float SideCut = (TotalBoardLength - CentralCut) / 2;

        Serial.print("Uset Input : \n\n");
        Serial.print("CentralCut = ");
        Serial.println(CentralCut);
        Serial.print("TotalBoardLength ");
        Serial.println(TotalBoardLength);
        Serial.print("User input ToothSize = ");
        Serial.println(ToothSize);
        Serial.print("SideCut ");
        Serial.println(SideCut);

        // re calcultae the *NEW* ToothSize on given board length while removing the central cut
        SymmetricalDistance(ToothSize, SideCut);
        Serial.print("NewToothSize = ");
        Serial.println(NewToothSize);
        Serial.print("BoardRepetissionNumber = ");
        Serial.println(BoardRepetissionNumber);
        int SideBoardRepetissionNumber = BoardRepetissionNumber;

        // return the  side toothRemind  + number Tooth Repetition
        toothThiknesCutRepetition(NewToothSize, saw_configs[installed_saw].thickness);
		int SideToothRepetitionsOrig = cutStructure[0];
        int SideToothRepetitions = cutStructure[1];
        float SidetoothRemind = cutStructure[2];

        float NewToothThickness = NewToothSize / (UnitSawDefinition[saw_configs[installed_saw].units]); // the number of stepps of the *NEW* ToothSize

        Serial.print("New ToothThickness = ");
        Serial.println(NewToothThickness);
        Serial.print("SideToothRepetitions = ");
        Serial.println(SideToothRepetitions);

        // return the toothRemind  + number Tooth Repetition for the central cut
        toothThiknesCutRepetition(CentralCut, saw_configs[installed_saw].thickness);
        int CentralCutToothRepetitionsOrig = cutStructure[0];
        int CentralCutToothRepetitions = cutStructure[1];
        float CentralCuttoothRemind = cutStructure[2];

        // change the unit motor to steps,  insted of mm / inch
        CentralCut = CentralCut / (UnitSawDefinition[saw_configs[installed_saw].units]);
        TotalBoardLength = TotalBoardLength / (UnitSawDefinition[saw_configs[installed_saw].units]);
        ToothSize = ToothSize / (UnitSawDefinition[saw_configs[installed_saw].units]);
        SideCut = (TotalBoardLength - CentralCut) / 2;

        // print Debug
        Serial.print("CentralCut = ");
        Serial.println(CentralCut);
        Serial.print("TotalBoardLength ");
        Serial.println(TotalBoardLength);
        Serial.print("User input ToothSize = ");
        Serial.println(ToothSize);
        Serial.print("SideCut ");
        Serial.println(SideCut);

        Serial.print("CentralCutToothRepetitionsOrig = ");
        Serial.println(CentralCutToothRepetitionsOrig);
        Serial.print("CentralCuttoothRemind , in step unit = ");
        Serial.println(CentralCuttoothRemind);
        Serial.print("SidetoothRemind , in step unit = ");
        Serial.println(SidetoothRemind);

        Serial.print("SideBoardRepetissionNumber = ");
        Serial.println(SideBoardRepetissionNumber);
        // clear 1 for all temp array
        for (int i = 0 ; i <= 3 ; i++) 
        {
				for (int n = 0 ; n < arraySize-2 ; n++) 
            {
						Temp[i][n] = CurrentMovment;
            }
        }
        for (int i = 0 ; i <= 1 ; i++) 
			{ 
				for (int n = 0 ; n < arraySize-2 ; n++) 
					{ 
						PossionDifferentAB[i][n] = 1;
					}
			}
        /// fill up the central cut temp attay
        int counterA = 1 ; // Board A start to cut the fist tooth side 
        int counterB = 1 ; // Board B start to cut on the secund tooth
        int counterC = 1 ; // array of the central cut when the number of the side cut are Odd
        int counterD = 1 ; // Board D central cut when the number of the side cut are  Even

        
        // counterC
        for (int n = 1; n < CentralCutToothRepetitions ; n++)  // 
        {
            Temp[2][counterC] = CurrentMovment; 
            counterC = counterC + 1;
        }

        if (CentralCuttoothRemind != 0) // verify if you have toothRemind and fill up correctly
        {
            Temp[2][counterC] = CentralCuttoothRemind; 
            counterC = counterC + 1;
        }
        else 
        {
            Temp[2][counterC] = CurrentMovment ;   
            counterC=counterC+1;				
        }


        // counterD 
        Temp[3][counterD] = CentralCut + CurrentMovment ;  
        counterD = counterD +1 ;

        //    ____     ____    ____         ____    ____    ____
        //   |   |    |   |   |   |        |   |   |   |   |   |
        //___|   |___ |   |___|   |________|   |___|   |___|   |___
        //

        int z = 0;
        if (SideBoardRepetissionNumber %2 != 0)
        {
            z=1;
        }
			
        //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        // A Board first side fillup the array 
        //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%	
        Serial.println("****************************");
        Serial.println("A Board first side fillup the array ");
        Serial.println("****************************");

        for (int n = 1; n <= SideBoardRepetissionNumber / 2 + z ; n++) // numebr of the tooth / 2  = giv you 1 side of reppetition
        {
            Xrepetission = (SideToothRepetitions + 1 )+((n-1)*(SideToothRepetitions)) ;
            
            Serial.print("Xrepetission = ");
            Serial.println(Xrepetission);
            Temp[0][Xrepetission] = NewToothThickness + CurrentMovment;
            
            if (SidetoothRemind == 0) 
            {
                Serial.print("SidetoothRemind == 0 , = ? ");
                Serial.println(Xrepetission);
                Temp[0][Xrepetission-1] = CurrentMovment ;
            }
            else 
            {
                Serial.print("SidetoothRemind =! 0 ");
                Serial.println(Xrepetission);
                Temp[0][Xrepetission-1] = SidetoothRemind;
            }
        }
        // debug print 
        counterA=Xrepetission+1;
        Serial.print("counterA =");
        Serial.println(counterA);
        Serial.print("counterC =");
        Serial.println(counterC);
        Serial.print("counterD =");
        Serial.println(counterD);
        Serial.print("SideBoardRepetissionNumber =");
        Serial.println(SideBoardRepetissionNumber);
        Serial.print("SideToothRepetitions =");
        Serial.println(SideToothRepetitions);
        
        Serial.println("Temp  0 , A " );
        for (int n = 1; n < counterA - z ; n++)
        {
            Serial.println(Temp[0][n]);
        }
        Serial.println("\n\n" );

        //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        // B Board first side fillup the array 
        //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        Serial.println("****************************");
        Serial.println("B Board first side fillup the array ");
        Serial.println("****************************");

        for (int n = 1; n <= SideBoardRepetissionNumber/2+1 ; n++)
        {
            Xrepetission = (SideToothRepetitions + 1 )+((n-1)*(SideToothRepetitions)) ;
            Serial.print("Xrepetission for B Board= ");
            Serial.println(Xrepetission);

            Temp[1][Xrepetission-SideToothRepetitions] = NewToothThickness + CurrentMovment;
            if (SidetoothRemind !=0) 
            {
                Temp[1][Xrepetission-SideToothRepetitions-1] = SidetoothRemind ;
            } 
            else 
            {
                Temp[1][Xrepetission-SideToothRepetitions-1] = CurrentMovment;
            }
        }

        counterB=Xrepetission-SideToothRepetitions;
        Serial.print("counterB = ");
        Serial.println(counterB);
        Serial.print("counterC = ");
        Serial.println(counterC);
        
        Serial.println("\n\n Temp 1, B " );
        for (int n = 1; n < counterB+z; n++)
        {
            Serial.println(Temp[1][n]);
        }
        Serial.println("\n\n" );

        //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        // Start to copy and fill up board A and B 
        //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

        if (SideBoardRepetissionNumber %2 == 0)
        {    
            // A Borad =  fill up with A counter + C counter
            // B Board =  fill up with B counter + D Counter

            // Borad A 

            for (int n = 1; n < counterA; n++) 
            {
                PossionDifferentAB[0][n] = Temp[0][n];
            }

                for (int n = 1; n < counterC ; n++)  // 
            {
                PossionDifferentAB[0][counterA-1+ n] = Temp[2][n+1]; //starting from n+1 becaus SideBoardRepetissionNumber %2 == 
                                                                    // and the priviuos staps incloding the n=1 value  
            }
            
            Serial.println("Temp 2 Center " );
            for (int n = 1; n < counterC; n++) 
                                                
            {
                Serial.println(Temp[2][n]);
            }
            Serial.println("\n\n" );


            Serial.println("Board A on side " );
            for (int n = 1; n < counterA-1 + counterC -1; n++)  // counterA-1 becaus we dont need the last parmeter , we have it on the first Temp 2 array 
                                                                // counterC -1 we need to remoce 1 stepas becosed it incloed on last temp0 array 
            {
                Serial.println(PossionDifferentAB[0][n]);
            }
            Serial.println("\n\n" );

            // Board B

            
            for (int n = 1; n < counterB; n++) // numebr of the tooth / 2  = giv you 1 side of reppetition
            {
                PossionDifferentAB[1][n] = Temp[1][n];
            }
            Serial.println("Temp 3 counterD B" );
            for (int n = 1; n < counterD; n++)
            {
                Serial.println(Temp[3][n]);
                PossionDifferentAB[1][counterB] = Temp[3][n];
            }
            Serial.println("\n\n" );

            Serial.println("Board B on side " );
            for (int n = 1; n < counterB + counterD-1 ; n++)
            {
                
                Serial.println(PossionDifferentAB[1][n]);
            }
            Serial.println("\n\n" );

        }
        else //(SideBoardRepetissionNumber %2 =! 0)
        {
            
            Serial.println("SideBoardRepetissionNumber %2 =! 0" );
            // Start to copy and fill up board A and B 
            // A Borad =  fill up with A counter + D counter
            // B Board =  fill up with B counter + C Counter
            
            //Borad A
            Serial.println("Temp 3 = counterD Board A" );
            for (int n = 1; n < counterD; n++) 
                                                
            {
                Serial.println(Temp[3][n]);
            }
            Serial.println("\n\n" );

            for (int n = 1; n <= counterA-counterD; n++) 
            {
                PossionDifferentAB[0][n] = Temp[0][n];
            }

                for (int n = 1; n < counterD ; n++)   
            {
                PossionDifferentAB[0][counterA-counterD+1] = Temp[3][n];; 
                                                                        
            }
            
            Serial.println("Board A one side counterA-counterD + 1" );
            for (int n = 1; n <= counterA-counterD+1 ; n++)    
                                                                
            {
                Serial.println(PossionDifferentAB[0][n]);
            }
            Serial.println("\n\n" );

        

            // Borad B

            Serial.println("Board B Temp 1 = counterB" );
            for (int n = 1; n <= counterB; n++)
            {
                Serial.println(Temp[1][n]);
                PossionDifferentAB[1][n] = Temp[1][n];
                
            }
            Serial.println("\n\n" );

            Serial.println("Board B, Temp 3 = counterC" );

            for (int n = 1; n < counterC; n++) 
                                                
            {
                Serial.println(Temp[2][n]);
            }
            Serial.println("\n\n" );

            for (int n = 1; n < counterC ; n++)  // 
            {
                
                PossionDifferentAB[1][counterB + n] = Temp[2][n+1]; //starting from n+1 becaus SideBoardRepetissionNumber %2 == 
                                                                    // and the priviuos staps incloding the n=1 value  
            }

            Serial.println("\n\n" ); 
            Serial.println("Board B one side = counterB + counterC -1" );
            //fill up 
                            
            for (int n = 1; n < counterB + counterC -1 ; n++)
            {
                    
                Serial.println(PossionDifferentAB[1][n]);
            }
            Serial.println("\n\n" );

        }


        Serial.print("\n\nMirorr Copy A \n\n");
        Serial.print("counterA =");
        Serial.println(counterA);
        Serial.print("counterB =");
        Serial.println(counterB);
        Serial.print("counterC =");
        Serial.println(counterC);
        Serial.print("counterD =");
        Serial.println(counterD);
        Serial.print("counterA + counterD =");
        Serial.println(counterA + counterD);
        Serial.print("counterA + counterC =");
        Serial.println(counterA + counterC);
        Serial.print("SideToothRepetitions =");
        Serial.println(SideToothRepetitions);
        //Miror Copy Start 
        int i = 1;

        if (SideBoardRepetissionNumber %2 == 0)
        {
            //#############################################################################
            //Miror Copy A SideBoardRepetissionNumber %2 == 0
            //#############################################################################

            Serial.print("counterA+counterC-SideToothRepetitions + i = ");
            Serial.println(counterA+counterC-SideToothRepetitions + i);

            for (int n = counterA-1; n > 1 ; n--) 
            {	
                PossionDifferentAB[0][counterA+counterC-SideToothRepetitions + i] = Temp[0][n];
                i=i+1;
            }
            Serial.println("Board A Full Array  " );
            for (int n = 1; n < 2*counterA + counterC ; n++)
            {
                
                Serial.println(PossionDifferentAB[0][n]);
            }
            //#############################################################################
            //Miror Copy B SideBoardRepetissionNumber %2 == 0
            //#############################################################################
            i = 1;
            Serial.print("\n\nMirorr Copy B \n\n");
            Serial.print("counterB =");
            Serial.println(counterB);
            Serial.print("counterD =");
            Serial.println(counterD);
            Serial.print("counterB+i =");
            Serial.println(counterB+i);

            for (int n = counterB-1; n >= 1 ; n--) 
            {
                PossionDifferentAB[1][counterB+i] = Temp[1][n]; // counterB+1 to start the next free array field 
                i=i+1;
            }
            
            Serial.println("Board B Full Array  " );
            for (int n = 1; n < 2*counterB + counterD ; n++)
            {
                
                Serial.println(PossionDifferentAB[1][n]);
            }
        }
        else //SideBoardRepetissionNumber %2 != 0
        {
            i=1;
            //#############################################################################
            Serial.print("Miror Copy A SideBoardRepetissionNumber %2 != 0");
            //#############################################################################
            Serial.print("counterA+counterD-SideToothRepetitions + i = ");
            Serial.println(counterA+counterD-SideToothRepetitions + i);

            for (int n = counterA-counterD; n > 1 ; n--) 
            {	
                PossionDifferentAB[0][counterA+counterD-SideToothRepetitions + i + z] = Temp[0][n];
                i=i+1;
            }
            Serial.println("Board A Full Array  " );
            for (int n = 1; n < 2*counterA - counterD ; n++)
            {
                Serial.println(PossionDifferentAB[0][n]);
            }

            i=1;
            //#############################################################################
            Serial.print("Miror Copy B SideBoardRepetissionNumber %2 != 0");
            //############################################################################
            for (int n = counterB; n >= 1 ; n--) 
            {
                PossionDifferentAB[1][counterB+counterC - 2 + i ] = Temp[1][n]; // counterB+1 to start the next free array field 
                i=i+1;
            }

            Serial.println("Board B Full Array  " );
            for (int n = 1; n < 2*counterB + counterC ; n++)
            {
                Serial.println(PossionDifferentAB[1][n]);
            }
        }

           
    }
    break;

    case 6: // Bridle Joint
    {
			Serial.print("\n\n\n@@@@@@@@@@@@@@@@@@@@@@@@@ startcut 6: // Bridle Joint @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");

        Serial.print("SelectPlanNumber = ");
        Serial.println(SelectPlanNumber);
        CurrentMovment = saw_configs[installed_saw].thickness / (UnitSawDefinition[saw_configs[installed_saw].units]);

        float CentralCut = PlanX / (UnitSawDefinition[saw_configs[installed_saw].units]);
        float TotalBoardLength = PlanY / (UnitSawDefinition[saw_configs[installed_saw].units]);
        float SideCut = (TotalBoardLength - CentralCut) / 2;

        number_of_repetitionsA = (CentralCut / CurrentMovment);
        number_of_repetitionsB = SideCut / CurrentMovment;
        
        Serial.print("number_of_repetitionsA =");
        Serial.println(number_of_repetitionsA);
        Serial.print("number_of_repetitionsB =");
        Serial.println(number_of_repetitionsB);

        int remaindStepsAint = number_of_repetitionsA;
        int remaindStepsBint = number_of_repetitionsB;

        remaindStepsA =( number_of_repetitionsA - remaindStepsAint) * CurrentMovment;
        remaindStepsB =( number_of_repetitionsB - remaindStepsBint) * CurrentMovment;

        number_of_repetitionsA = remaindStepsAint;
        number_of_repetitionsB = remaindStepsBint;

        Serial.print("remaindStepsAint =");
        Serial.println(remaindStepsAint);
        Serial.print("remaindStepsBint =");
        Serial.println(remaindStepsBint);

        Serial.print("CurrentMovment =");
        Serial.println(CurrentMovment);

        Serial.print("number_of_repetitionsA =");
        Serial.println(number_of_repetitionsA);
        Serial.print("number_of_repetitionsB =");
        Serial.println(number_of_repetitionsB);
        Serial.print("remaindStepsA =");
        Serial.println(remaindStepsA);
        Serial.print("remaindStepsB =");
        Serial.println(remaindStepsB);
        Serial.print("SideCut =");
        Serial.println(SideCut);
        Serial.print("TotalBoardLength =");
        Serial.println(TotalBoardLength);
        Serial.print("CentralCut =");
        Serial.println(CentralCut);
        Serial.print("saw_configs[installed_saw].pressure/10*pressureReference = ");
        Serial.println(saw_configs[installed_saw].pressure/10*pressureReference);

        PosissionMovment = 0; // configure the counter differently only for case 6

        if (remaindStepsA == 0)
        {

            number_of_repetitionsA = number_of_repetitionsA - 1;
            number_of_repetitionsB = number_of_repetitionsB - 1;
            //Serial.print("number_of_repetitionsA =");
            //Serial.println(number_of_repetitionsA);
            //Serial.print("number_of_repetitionsB =");
            //Serial.println(number_of_repetitionsB);
            remaindStepsA = CurrentMovment;
            //remaindStepsB = CurrentMovment;
        }

        saw_configs[installed_saw].HomeBmovmentOffset = 0;

        // for boardtype == 0)

        for (int n = 0; n <= number_of_repetitionsA; n++)
        {
            if (n == 0)
            {
                PossionDifferentAB[0][n] = ((TotalBoardLength / 2 - CentralCut / 2) + CurrentMovment);

                Serial.print("TotalBoardLength / 2 - CentralCut / 2) + CurrentMovment = ");
                Serial.println(PossionDifferentAB[0][n]);
            }

            else if (n == number_of_repetitionsA)
            {

                PossionDifferentAB[0][n] = remaindStepsA;
                Serial.print("remaindStepsA =");
                Serial.println(remaindStepsA);
                Serial.print("(PossionDifferentAB[0][n]=");
                Serial.println(PossionDifferentAB[0][n]);
            }
            else
            {
                PossionDifferentAB[0][n] = CurrentMovment - (saw_configs[installed_saw].pressure/10*pressureReference);
            }
        }

        for (int n = 0; n < arraySize - 1; n++)
        {
            Serial.println(PossionDifferentAB[0][n]);
        }

        //for .boardtype == 1)

        for (int n = 0; n <= number_of_repetitionsB * 2; n++)
        {
            //start to print form 0 insted to 1 to see the 0 filed
            PossionDifferentAB[1][n] = CurrentMovment;
        }

        for (int n = 0; n <= number_of_repetitionsB * 2; n++)
        {

            if ((n == number_of_repetitionsB ) && (remaindStepsB ==0))
            {
                
                Serial.print("(n == number_of_repetitionsB ) && (remaindStepsB ==0)");
                PossionDifferentAB[1][n] = CentralCut + CurrentMovment;
                Serial.print("CentralCut=");
                Serial.println(CentralCut);
                Serial.print("PossionDifferentAB[1][n] = ");
                Serial.println(PossionDifferentAB[1][n]);
            }
            if ((n == number_of_repetitionsB) && (remaindStepsB !=0))
            {
                
				n=n+1;
                PossionDifferentAB[1][n] = CentralCut + CurrentMovment;
				PossionDifferentAB[1][n-1] = remaindStepsB;
                Serial.print("remaindStepsB =");
                Serial.println(remaindStepsB);
                Serial.print("PossionDifferentAB[1][n] = ");
                Serial.println(PossionDifferentAB[1][n]);
            }
            if ((n == number_of_repetitionsB * 2) && (remaindStepsB !=0))
            {
				
                PossionDifferentAB[1][n+1] = remaindStepsB;
                Serial.print("remaindStepsB =");
                Serial.println(remaindStepsB);
                Serial.print("PossionDifferentAB[1][n] = ");
                Serial.println(PossionDifferentAB[1][n]);

            }
        }

        for (int n = 0; n < arraySize - 1; n++)
        {
            //start to print form 0 insted to 1 to see the 0 filed
            Serial.println(PossionDifferentAB[1][n]);
        }
    }
    break;

    case 7: //DoveTail Flat Joint
    {
            
            Serial.print("\n\n\n@@@@@@@@@@@@@@@@@@@@@@@@@ startcut 7: // shape @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
            Serial.print("SelectPlanNumber = ");
            Serial.println(SelectPlanNumber);
		
            double BitDegree = (saw_configs[1].thickness_d* 71) / 4068; //invert from rad
            float CurrentMovmentHigh = saw_configs[1].thickness_D;
            double TangesResult = tan(BitDegree);
            float CurrentMovmentLow = CurrentMovmentHigh - (saw_configs[1].thickness_h*2*TangesResult);
            float baseResio = CurrentMovmentHigh / CurrentMovmentLow;
           
            Serial.print("BitDegree = ");
            Serial.println(BitDegree,4);
            Serial.print("TangesResult = ");
            Serial.println(TangesResult,4);
            Serial.print("tan(BitDegree) = ");
             Serial.println(tan(BitDegree));

            Serial.print("saw_configs[1].thickness_h*tan(BitDegree)*2 = ");
            Serial.println(saw_configs[1].thickness_h*tan(BitDegree));
            Serial.print("CurrentMovmentHigh = ");
            Serial.println(CurrentMovmentHigh);
            Serial.print("CurrentMovmentLow = ");
            Serial.println(CurrentMovmentLow);
            Serial.print("baseResio = ");
            Serial.println(baseResio);

			float TotalBoardLength = PlanY ;/// (UnitSawDefinition[saw_configs[installed_saw].units]);
			float UserToothSizeHighA = PlanX; /// (UnitSawDefinition[saw_configs[installed_saw].units]);
            float UserToothSizeLowA = UserToothSizeHighA/baseResio;
            float BoardWithoutHalfBitA =  TotalBoardLength - UserToothSizeHighA ;
                



            Serial.print("TotalBoardLength = ");
            Serial.println(TotalBoardLength);
            Serial.print("UserToothSizeHighA = ");
            Serial.println(UserToothSizeHighA);
            Serial.print("UserToothSizeLowA = ");
            Serial.println(UserToothSizeLowA);
            Serial.print("BoardWithoutHalfBitA = ");
            Serial.println(BoardWithoutHalfBitA);
// calcute the board parameter 




			// A Board 

            // return the   toothRemind  + number Tooth Repetition is needed based on the user Tooth length
            // toothThiknesCutRepetition(saw Tooth length, user Tooth length);
            toothThiknesCutRepetition(UserToothSizeHighA , CurrentMovmentHigh);
            int UserToothSizeHighA_RepetitionsOrigA = cutStructure[0];
            int UserToothSizeHighA_RepetitionsA = cutStructure[1];
            float UserToothSizeHighA_RemindA = cutStructure[2] * (UnitSawDefinition[saw_configs[installed_saw].units]);
            
            Serial.print("UserToothSizeHighA_RepetitionsOrigA = ");
			Serial.println(UserToothSizeHighA_RepetitionsOrigA);
			Serial.print("UserToothSizeHighA_RepetitionsA = ");
			Serial.println(UserToothSizeHighA_RepetitionsA);				
			Serial.print("UserToothSizeHighA_RemindA = ");
			Serial.println(UserToothSizeHighA_RemindA);


            toothThiknesCutRepetition(BoardWithoutHalfBitA, UserToothSizeHighA);
            int BoardDovetailToothRepetitionsOrigA = cutStructure[0];
            int BoardDovetailToothRepetitionsA = cutStructure[1];
            float BoardDovetailToothRemindA = cutStructure[2] * (UnitSawDefinition[saw_configs[installed_saw].units]);

            Serial.print("BoardDovetailToothRepetitionsOrigA = ");
			Serial.println(BoardDovetailToothRepetitionsOrigA);
			Serial.print("BoardDovetailToothRepetitionsA = ");
			Serial.println(BoardDovetailToothRepetitionsA);				
			Serial.print("BoardDovetailToothRemindA = ");
			Serial.println(BoardDovetailToothRemindA);


            float SpaceRemainForBCut = 0 ;
            if (BoardDovetailToothRepetitionsA % 2 == 0)
			{
				 SpaceRemainForBCut = (BoardDovetailToothRepetitionsOrigA/2*UserToothSizeHighA)+BoardDovetailToothRemindA; // Even 
			}
			else
			{
				 SpaceRemainForBCut = ((BoardDovetailToothRepetitionsOrigA+1)/2*UserToothSizeHighA)+BoardDovetailToothRemindA; // Odd
			}
            
            float SpaceCutBetweenToothA = SpaceRemainForBCut /((BoardDovetailToothRepetitionsOrigA/2)+1);
            float UserToothSizeHighB =   SpaceCutBetweenToothA + UserToothSizeHighA - UserToothSizeLowA;   

			Serial.print("\n\n\n SpaceCutBetweenToothA = ");
			Serial.println(SpaceCutBetweenToothA);
            Serial.print("UserToothSizeHighB = ");
			Serial.println(UserToothSizeHighB);

            toothThiknesCutRepetition(UserToothSizeHighB, CurrentMovmentHigh);
            int UserToothSizeHighB_RepetitionsOrigB = cutStructure[0];
            int UserToothSizeHighB_RepetitionsB = cutStructure[1];
            float UserToothSizeHighB_RemindB = cutStructure[2] * (UnitSawDefinition[saw_configs[installed_saw].units]);

            Serial.print("UserToothSizeHighB_RepetitionsOrigB = ");
			Serial.println(UserToothSizeHighB_RepetitionsOrigB);
			Serial.print("UserToothSizeHighB_RepetitionsB = ");
			Serial.println(UserToothSizeHighB_RepetitionsB);
            Serial.print("UserToothSizeHighB_RemindB = ");
			Serial.println(UserToothSizeHighB_RemindB);
            Serial.print("\n\n\n");

            int RepatationCutA = 0 ;
            int RepatationCutB = 0 ;
            if (BoardDovetailToothRepetitionsA % 2 == 0)
			{
				 RepatationCutA = (BoardDovetailToothRepetitionsOrigA +1)/2; // Even 
                 RepatationCutB = BoardDovetailToothRepetitionsA/2; // Even 
			}
			else
			{
				 RepatationCutA = (BoardDovetailToothRepetitionsOrigA +1)/2; // Odd 
                 RepatationCutB = BoardDovetailToothRepetitionsA/2; // Odd 
                 
            }
            


            
		// used parameter 
        CurrentMovmentHigh = CurrentMovmentHigh / (UnitSawDefinition[saw_configs[installed_saw].units]);
        CurrentMovmentLow = CurrentMovmentLow / (UnitSawDefinition[saw_configs[installed_saw].units]);
        
        UserToothSizeHighA = UserToothSizeHighA / (UnitSawDefinition[saw_configs[installed_saw].units]);
        UserToothSizeLowA = UserToothSizeLowA / (UnitSawDefinition[saw_configs[installed_saw].units]);  
        
        UserToothSizeHighB = UserToothSizeHighB / (UnitSawDefinition[saw_configs[installed_saw].units]);
        UserToothSizeHighB_RemindB = UserToothSizeHighB_RemindB / (UnitSawDefinition[saw_configs[installed_saw].units]);
        UserToothSizeHighA_RemindA = UserToothSizeHighA_RemindA / (UnitSawDefinition[saw_configs[installed_saw].units]);
        BoardWithoutHalfBitA = BoardWithoutHalfBitA / (UnitSawDefinition[saw_configs[installed_saw].units]);
        
        
        Serial.print("\n\n\n used parameter =");
        Serial.print("CurrentMovmentHigh = ");
        Serial.println(CurrentMovmentHigh);
        Serial.print("\nCurrentMovmentLow = ");
        Serial.println(CurrentMovmentLow);
        
        Serial.print("\n\nUserToothSizeHighA = ");
        Serial.println(UserToothSizeHighA);
        Serial.print("\nUserToothSizeLowA = ");
        Serial.println(UserToothSizeLowA);
        Serial.print("\nUserToothSizeHighA_RemindA = ");
        Serial.println(UserToothSizeHighA_RemindA);

        Serial.print("\nUserToothSizeHighB = ");
        Serial.println(UserToothSizeHighB);
        Serial.print("\nUserToothSizeHighB_RemindB = ");
        Serial.println(UserToothSizeHighB_RemindB);

        Serial.print("\nUserToothSizeHighA_RepetitionsA = ");
        Serial.println(UserToothSizeHighA_RepetitionsA);
        Serial.print("\nUserToothSizeHighB_RepetitionsB = ");
        Serial.println(UserToothSizeHighB_RepetitionsB);
        Serial.print("\nCurrentMovmentHigh = ");
        Serial.println(CurrentMovmentHigh);
        Serial.print("\nCurrentMovmentLow = ");
        Serial.println(CurrentMovmentLow);

        Serial.print("\n RepatationCutA = ");
        Serial.println(RepatationCutA);
        Serial.print("\n RepatationCutB = ");
        Serial.println(RepatationCutB);
        
        Serial.print("\n\n");
        for (int i = 0 ; i <= 3 ; i++) 
        {
				for (int n = 0 ; n < arraySize-2 ; n++) 
            {
						Temp[i][n] = CurrentMovment;
            }
        }
        for (int i = 0 ; i <= 1 ; i++) 
			{ 
				for (int n = 0 ; n < arraySize-2 ; n++) 
					{ 
						PossionDifferentAB[i][n] = 1;
					}
			}


        Serial.println("****************************");
        Serial.println("A Board Dovetail ");
        Serial.println("****************************");

        float arrayCount = 0; // Count the progress of the steps
        //Array step filed up
        //PossionDifferentAB[0][1] = ToothThickness + CurrentMovment;
        PossionDifferentAB[0][1] = UserToothSizeHighA/2;

        for (int n = 1; n <= RepatationCutA; n++)
        {

            Xrepetission = (UserToothSizeHighA_RepetitionsA + 1) + ((n - 1) * (UserToothSizeHighA_RepetitionsA));

            Serial.print("Xrepetission = ");
            Serial.println(Xrepetission);
            Serial.print("arrayCount = ");
            Serial.println(arrayCount);
            PossionDifferentAB[0][Xrepetission] = UserToothSizeHighA + CurrentMovmentHigh;
            PossionDifferentAB[1][Xrepetission - UserToothSizeHighA_RepetitionsA] = UserToothSizeHighB + CurrentMovmentHigh;
            if (UserToothSizeHighA_RemindA != 0)
            {
                PossionDifferentAB[0][Xrepetission - 1] = UserToothSizeHighA_RemindA;
                PossionDifferentAB[1][Xrepetission - UserToothSizeHighA_RepetitionsA -1] = UserToothSizeHighB_RemindB;
            }
            else
            {
                PossionDifferentAB[0][Xrepetission - 1] = CurrentMovmentHigh;
                PossionDifferentAB[1][Xrepetission - UserToothSizeHighB_RepetitionsB - 1] = CurrentMovmentLow;
            }
            for (int i = 1; i <= Xrepetission; i++)
            {
                arrayCount = arrayCount + PossionDifferentAB[0][i];
            }
            Serial.print("arrayCount = ");
            Serial.println(arrayCount);

            Serial.print("n = ");
            Serial.println(n);

            if (BoardWithoutHalfBitA < arrayCount)
            {
                Serial.print("\n\n\n exit \n\n");
                n = arraySize + 1;
                //exit ;
            }
            arrayCount = 0; // reset and start to conut up to new Xrepetission number
        }
      
        // array printing
        Serial.println("*********PossionDifferentAB[0][n]********");
        for (int n = 0; n <= Xrepetission; n++)
        {
            Serial.println(PossionDifferentAB[0][n]);
        }

        Serial.println("*********PossionDifferentAB[1][n]********");
        for (int n = 0; n <= Xrepetission; n++)
        {
            Serial.println(PossionDifferentAB[1][n]);
        }


    }
    
    break;

    case 8:
    {
        installed_saw = 0;
        // Symmetrical Finger Joint
        Serial.print("\n\n\n@@@@@@@@@@@@@@@@@@@@@@@@@ 8 TBD   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n\n");
        Serial.print("SelectPlanNumber = ");
        Serial.println(SelectPlanNumber);
    }
    break;

    case 9:
    {
        installed_saw = 0;
        // Symmetrical Finger Joint
        Serial.print("\n\n\n@@@@@@@@@@@@@@@@@@@@@@@@@ 9 Half blind   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n\n");
        Serial.print("SelectPlanNumber = ");
        Serial.println(SelectPlanNumber);
    }
    break;

    case 10:
    {
        installed_saw = 0;
        // Symmetrical Finger Joint
        Serial.print("\n\n\n@@@@@@@@@@@@@@@@@@@@@@@@@ 10 Triple Joint   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n\n");
        Serial.print("SelectPlanNumber = ");
        Serial.println(SelectPlanNumber);
    }
    break;

    default:
		SelectPlanNumber = 1 ;
    }
}

void load_calib()
{
    File file = LittleFS.open("/calib.json", "r");

    if (!file)
    {
        Serial.println("calib.json does not exist, creating one with default values");

        installed_saw = 0;

        // Define default saw here
        saw_configs[0].index = 0;
        saw_configs[0].thickness = 3.5; //TODO: need to change to valid Bit
        saw_configs[0].steps = 0.0;
        saw_configs[0].boardtype = 0;
        saw_configs[0].units = SAW_UNITS_MM;
        saw_configs[0].pressure = 0;
        saw_configs[0].HomeBmovmentOffset = 0;

        saw_configs[1].index = 1;
        saw_configs[1].thickness_d = 14; //TODO: need to change to valid Bit
        saw_configs[1].thickness_h = 10;  //TODO: need to change to valid Bit
        saw_configs[1].thickness_D = 12.3;  //TODO: need to change to valid Bit
        saw_configs[1].boardtype = 0;
        saw_configs[1].steps = 0.0;
        saw_configs[1].units = SAW_UNITS_MM;
        saw_configs[1].pressure = 0;
        saw_configs[1].HomeBmovmentOffset = 0;

        save_calib();
    }
    else
    {
        String json = file.readString();
        Serial.println(json);
        file.close();
        parse_json_calib(json);
    }
}

void save_calib()
{
    Serial.println("save_calib() started ");
    String json;
    build_json_calib(&json);

    File file = LittleFS.open("/calib.json", "w");
    if (!file)
    {
        Serial.println("Error opening file for writing");
        home_releas_func(); //TODO: check if needed
        Serial.println("Reset..");
        ESP.restart();
        return;
    }

    file.println(json);
    file.close();

    printJsonStringToSerial(json);
    //Serial.println(json);
    //bIsSawBusy  = false;
    //checksawbusy() ;
    Serial.println(" save calib() END ");
}

void printJsonStringToSerial(String json)
{
    StaticJsonDocument<1000> jsonBuffer;
    deserializeJson(jsonBuffer, json);
    serializeJsonPretty(jsonBuffer, Serial);
}

void build_json_calib(String *calibjson)
{
    *calibjson = "{\"max_num_saws\":\"" + String(MAX_NUM_SAWS) + "\", ";
    *calibjson += "\"installed_saw\":\"" + String(installed_saw) + "\", ";

    for (int i = 0; i < MAX_NUM_SAWS; i++)
    {
        *calibjson += "\"saw_" + String(i) + "_index\":\"" + String(saw_configs[i].index) + "\", ";
        if (i == 0)
        {
            *calibjson += "\"saw_" + String(i) + "_thickness\":\"" + String(saw_configs[i].thickness) + "\", ";
        }
        else
        {
            *calibjson += "\"saw_" + String(i) + "_thickness_d\":\"" + String(saw_configs[i].thickness_d) + "\", ";
            *calibjson += "\"saw_" + String(i) + "_thickness_h\":\"" + String(saw_configs[i].thickness_h) + "\", ";
            *calibjson += "\"saw_" + String(i) + "_thickness_D\":\"" + String(saw_configs[i].thickness_D) + "\", ";
        }
        *calibjson += "\"saw_" + String(i) + "_steps\":\"" + String(saw_configs[i].steps) + "\", ";
        *calibjson += "\"saw_" + String(i) + "_boardtype\":\"" + saw_configs[i].boardtype + "\", ";
        *calibjson += "\"saw_" + String(i) + "_units\":\"" + saw_configs[i].units + "\", ";
        *calibjson += "\"saw_" + String(i) + "_pressure\":\"" + String(saw_configs[i].pressure) + "\", ";
        *calibjson += "\"saw_" + String(i) + "_HomeBmovmentOffset\":\"" + String(saw_configs[i].HomeBmovmentOffset) + "\", ";
    }
    *calibjson += "\"last_param\":\"1\"}";
}

void parse_json_calib(String calibjson)
{
    // Serial.println("*********************************");
    // Serial.println(calibjson);
    // Serial.println("*********************************");
    StaticJsonDocument<1000> jsonBuffer;
    deserializeJson(jsonBuffer, calibjson);

    String max_num_saws = jsonBuffer["max_num_saws"];
    String installed_saw_str = jsonBuffer["installed_saw"];
    installed_saw = installed_saw_str.toInt();

    String saw_0_index = jsonBuffer["saw_0_index"];
    saw_configs[0].index = saw_0_index.toInt();

    String saw_1_index = jsonBuffer["saw_1_index"];
    saw_configs[1].index = saw_1_index.toInt();

    String saw_0_thickness = jsonBuffer["saw_0_thickness"];
    saw_configs[0].thickness = saw_0_thickness.toFloat();

    String saw_1_thickness_d = jsonBuffer["saw_1_thickness_d"];
    saw_configs[1].thickness_d = saw_1_thickness_d.toFloat();

    String saw_1_thickness_h = jsonBuffer["saw_1_thickness_h"];
    saw_configs[1].thickness_h = saw_1_thickness_h.toFloat();

    String saw_1_thickness_D = jsonBuffer["saw_1_thickness_D"];
    saw_configs[1].thickness_D = saw_1_thickness_D.toFloat();

    String saw_0_steps = jsonBuffer["saw_0_steps"];
    saw_configs[0].steps = saw_0_steps.toFloat();

    String saw_1_steps = jsonBuffer["saw_1_steps"];
    saw_configs[1].steps = saw_1_steps.toFloat();

    //String saw_0_type = jsonBuffer["saw_0_type"];
    //saw_configs[0].type = saw_0_type.toInt();

    //String saw_1_type= jsonBuffer["saw_1_type"];
    //saw_configs[1].type = saw_1_type.toInt();

    String saw_0_units = jsonBuffer["saw_0_units"];
    saw_configs[0].units = saw_0_units.toInt();

    String saw_1_units = jsonBuffer["saw_1_units"];
    saw_configs[1].units = saw_1_units.toInt();

    //   String saw_0_description = jsonBuffer["saw_0_description"];
    //   saw_configs[0].description = saw_0_description;

    //   String saw_1_description = jsonBuffer["saw_1_description"];
    //   saw_configs[1].description = saw_1_description;

    String saw_0_boardtype = jsonBuffer["saw_0_boardtype"];
    saw_configs[0].boardtype = saw_0_boardtype.toInt();

    String saw_1_boardtype = jsonBuffer["saw_1_boardtype"];
    saw_configs[1].boardtype = saw_1_boardtype.toInt();

    String saw_0_pressure = jsonBuffer["saw_0_pressure"];
    saw_configs[0].pressure = saw_0_pressure.toInt();

    String saw_1_pressure = jsonBuffer["saw_1_pressure"];
    saw_configs[1].pressure = saw_1_pressure.toInt();
    Serial.println(saw_0_pressure);
    Serial.println(saw_1_pressure);
    //   Serial.println(saw_configs[0].pressure);
    //   Serial.println(saw_configs[1].pressure);

    String saw_0_HomeBmovmentOffset = jsonBuffer["saw_0_HomeBmovmentOffset"];
    saw_configs[0].HomeBmovmentOffset = saw_0_HomeBmovmentOffset.toInt();

    String saw_1_HomeBmovmentOffset = jsonBuffer["saw_1_HomeBmovmentOffset"];
    saw_configs[1].HomeBmovmentOffset = saw_1_HomeBmovmentOffset.toInt();
}

void get_json_calib()
{
    String calibjson;

    build_json_calib(&calibjson);

    server.send(200, "application/json", calibjson);
}

void save_json_calib()
{
    parse_json_calib(server.arg("plain"));

    Serial.println("Saving new calibration data");

    save_calib();
    server.send(200, "application/text", "Data Saved");
}

void checksawbusy()
{
    String response_json;

    if (bIsSawBusy)
    {
        response_json = "{\"is_saw_busy\":\"true\"}";
    }
    else
    {
        response_json = "{\"is_saw_busy\":\"false\"}";
    }

    server.send(200, "application/json", response_json);

    Serial.println(response_json);
}

void serverStarting()
{
    Serial.print("\nESP Server Loading...");

#ifdef WIFI_ACCESS_POINT
    wifi_station_set_auto_connect(false);
    wifi_station_disconnect();
    bool wifi_AP_valid = false;
    while (!wifi_AP_valid)
    {
        Serial.print(".");
        if (!WiFi.mode(WIFI_AP)) // in  AP mode
        {
            Serial.println("\nError: Failed to set mode!");
        }
        else if (!WiFi.softAPConfig(ip, gateway, subnet))
        {
            Serial.println("\nError: Failed to set AP configuration!");
        }
        else if (!WiFi.softAP(ssid, password))
        {
            Serial.println("\nError: Failed to setup AP!");
        }
        else
        {
            wifi_AP_valid = true;
        }
    }
    Serial.print("\nAP SSID: ");
    Serial.println(ssid);
    Serial.print("AP PASSWORD: ");
    Serial.println(password);
    Serial.print("Server Started at IP: ");
    Serial.println(WiFi.softAPIP().toString());

#else
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Attempting to connect to WiFI \r\n");
    Serial.print(ssid);

    unsigned char iCounter = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");

        iCounter++;
        if (iCounter > 20)
        {
            Serial.print("\r\n");
            iCounter = 0;
        }
    }
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

#endif
    server.serveStatic("/css", LittleFS, "/css");
    server.serveStatic("/css/style.css", LittleFS, "/css/style.css", "max-age=5000");
    server.serveStatic("/img/imgMainLogo.svg", LittleFS, "/img/imgMainLogo.svg", "max-age=5000");
    server.serveStatic("/img/plan1oob.svg", LittleFS, "/img/plan1oob.svg", "max-age=5000");
    server.serveStatic("/img/plan2asymmetrical.png", LittleFS, "/img/plan2asymmetrical.png", "max-age=5000");
    server.serveStatic("/img/plan3symmetrical.png", LittleFS, "/img/plan3symmetrical.png", "max-age=5000");
    server.serveStatic("/img/plan4incremental.png", LittleFS, "/img/plan4incremental.png", "max-age=5000");
    server.serveStatic("/img/plan5custom.png", LittleFS, "/img/plan5custom.png", "max-age=5000");
    server.serveStatic("/img/plan6bridlejoint.png", LittleFS, "/img/plan6bridlejoint.png", "max-age=5000");
    server.serveStatic("/img", LittleFS, "/img");
    server.serveStatic("/", LittleFS, "/index.htm");
    server.serveStatic("/calib.htm", LittleFS, "/calib.htm");
    server.serveStatic("/calib.json", LittleFS, "/calib.json");
    server.serveStatic("/cut.htm", LittleFS, "/cut.htm");
    server.serveStatic("/select.htm", LittleFS, "/select.htm");
    server.serveStatic("/calib.js", LittleFS, "/calib.js");
    server.serveStatic("/select.js", LittleFS, "/select.js");
    server.serveStatic("/cut.js", LittleFS, "/cut.js");
    server.serveStatic("/information.htm", LittleFS, "/information.htm");
    server.on("/getcalib", get_json_calib);
    server.on("/savecalib", save_json_calib);
    server.on("/speed", speedSelect);
    server.on("/direction", directionSelect);
    server.on("/sethome", setHome);
    server.on("/boardtype", boardtypeSelect);
    server.on("/forward", forwardCut);
    server.on("/reverse", reverseCut);
    server.on("/home", home);
    server.on("/checksawbusy", checksawbusy);
    server.on("/startcut", startcut);
    server.begin(); // start server
}

void blinkLed(int blinkNumner)
{
    for (int t = 0; t <= blinkNumner; t++)
    {
        digitalWrite(OnBoardLed, HIGH);
        delay(150);
        digitalWrite(OnBoardLed, LOW);
        delay(150);
    }
    digitalWrite(OnBoardLed, HIGH);
}

void fileLog(String txt)
{
    File fileDebug = LittleFS.open("/Log.txt", "w");

    if (!fileDebug)
    {
        Serial.println("Error opening file for writing");
        return;
    }
    if (fileDebug.println(txt))
    {
        Serial.println("file Ok");
    }
    else
    {
        Serial.println("file failed");
    }
    fileDebug.close();
    Serial.println("/Log.txt open");
}

void fileLogAppend(char txt)
{
    File fileAdd = LittleFS.open("/Log.txt", "a");

    if (!fileAdd)
    {
        Serial.println("Error opening file for writing");
        return;
    }
    if (fileAdd.println(txt))
    {
        Serial.println("file append");
    }
    else
    {
        Serial.println("file failed");
    }
    fileAdd.close();
}
