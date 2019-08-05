/*
 * Movie Projecter Version: LED
 * Hardware: Vangelis Lympouridis, PhD
 * Firmware: Seth Persigehl, Factorio Player
 * 
 * Goal: Replace the mechanical shutter system with an
 *       electronic equivilent, using an LED. Timings, 
 *       duty cycle, and number of pulses are adjustable 
 *       via the serial port.
 * The hardware wheel has 12 cogs
 * DC60S3 Solid state relay, switching 24VDC
 * 
 */


#define OPTOSENSORPIN A9  // an arbitrary threshold level that's in the range of the analog input
#define POWERLEDPIN 13    // pin that the power LED is attached to
#define BOARDLEDPIN 22    // pin connected to the small onboard LED
#define FRAMEDURATIONARRAYSIZE 12 // Number of frame durations to store in memory, currently used for averaging together. May be used to calculate abnormalities in the mechanical gears

bool PrintExtraDebug = true;
//The following are global for reads, but should only be set by DetectFrameStartAndEnd
bool pulseHighLastLoop;
bool pulseHighThisLoop;
bool newFrameDetected;
bool newPulseFaded;
bool frameDone;
int optoThreshold = 512;
int optoSensorValue;
unsigned long frameStartTimeMillis;

unsigned long frameEnd;
unsigned long minimumDuration;
unsigned long averageFrameDuration;
unsigned long lastFrameDuration;
unsigned long frameDurationArray[FRAMEDURATIONARRAYSIZE];
unsigned int frameDurationIndex;
unsigned int numberOfLightPulsesPerFrame = 3;
unsigned long actionsTimingsArray[10]; // Needs to be large enough for all needed timings, at least 2*numberOfLightPulsesPerFrame
unsigned int actionsTimingsIndex;

// The timing of the LED pulses can be adjusted using the array below.
// Function codings:
// 0 = Lights off
// 1 = Lights on
// 2 = Done with frame
//unsigned int actionsArray[10] = {0,1,0,1,0,1,2,0,0,0}; // What command do we want to call at the function time?
//unsigned int actionsArray[10] = {0,1,0,1,0,0,0,0,0,0}; //The first 120fps video, on(ish) for three off for 2. Very choppy
//unsigned int actionsArray[10] = {0,1,1,1,0,0,0,0,0,0}; //The second 120fps video, on 3.5, off 1.5 rolling. Choppy
//unsigned int actionsArray[10] = {0,1,1,1,1,0,0,0,0,0}; // The third 120fps video, on 4 off 1. Hard flashing.
  unsigned int actionsArray[10] = {0,1,0,0,1,0,0,0,0,0}; // This provided the best visible results. Flashes at start, then off, then flash at end.

//// Setup and Loop ///////////////////////////////////////////////////////////////////////////////
void setup()
{
  Serial.begin(115200);
  pinMode(OPTOSENSORPIN, INPUT);
  pinMode(POWERLEDPIN, OUTPUT);
  pinMode(BOARDLEDPIN, OUTPUT);
  Serial.println("Setup ended. Starting first loop...");
}


void loop()
{
  DetectFrameStartAndEnd();
  NewWaveDetectedInstantResponse();
  RecordFrameTimings();
  CheckAndRunTimedEvents();
  CommunicateWithUser();
}


//// Primary Functions ///////////////////////////////////////////////////////////////////////////////
void DetectFrameStartAndEnd(){
  pulseHighLastLoop = pulseHighThisLoop;
  optoSensorValue = analogRead(OPTOSENSORPIN);
  if (optoSensorValue > optoThreshold)pulseHighThisLoop = true;
  else pulseHighThisLoop = false;
  
  if (!pulseHighLastLoop && pulseHighThisLoop)newFrameDetected = true;
  else newFrameDetected = false;

  if (pulseHighLastLoop && !pulseHighThisLoop)newPulseFaded = true;
  else newPulseFaded = false;
}


void RecordFrameTimings(){
  if (newFrameDetected){  
      frameDone = false;
      frameDurationArray[frameDurationIndex] = millis() - frameStartTimeMillis;
      frameDurationIndex++;
      if (frameDurationIndex >= FRAMEDURATIONARRAYSIZE) frameDurationIndex = 0;
      frameStartTimeMillis = millis();
      //TODO pulseTimingsArray[ //Maybe not needed
      CalculateFrameTimings();
    }
}


void CalculateFrameTimings(){
  CalculateAverageFrameDuration();

  int numTimedEvents = numberOfLightPulsesPerFrame * 2;
  unsigned int durationOfEachStateChange = averageFrameDuration / numTimedEvents;
  
  for (int i = 0 ; i < numTimedEvents ; i++){
    actionsTimingsArray[i] = frameStartTimeMillis + (durationOfEachStateChange * i);
    // Note if there are extra actions in the array, they will not be set
  }
  // Set the next event to the max value for unsigned long, about 50 days after power-on
  // 4,294,967,295 is the max value, but remember not to use commas!.
  actionsTimingsArray[numTimedEvents + 1] = 4294967295; 
  
   // Without prempting our timings using NewWaveDetectedInstantResponse(), we'd 
   // normally reset our timings index to 0 to then run through turing the lights
   // on and off. Since that's already done by NewWaveDetectedInstantResponse(), 
   // we'll comment out the following statement:
   //
   // actionsTimingsIndex = 0; 
   // 
   // TODO: Allow for offset to timings and first frame, in case it is 
   //       undesireable to start the functions running based on the 
   //       first moment the new frame is detected.
}


void CalculateAverageFrameDuration(){
  averageFrameDuration = AverageArray (frameDurationArray, FRAMEDURATIONARRAYSIZE);
}

void NewWaveDetectedInstantResponse(){
  // Act as soon as possible to get the lights on. Sort out the details later.
  if (newFrameDetected) {
    SetOutputs (false);
    actionsTimingsIndex = 1; //Originally 1, let's start earlier at index 0
    if (PrintExtraDebug) Serial.println(F("NWDIR: START"));
    // We move the index to the next timing, even though it's not calculated yet,
    // because we already turned the lights on (the action set for timing index 0).
  }
}


void SetOutputs(bool b){
  digitalWrite(POWERLEDPIN, b);
  digitalWrite(BOARDLEDPIN, b);
}


void CheckAndRunTimedEvents(){
  if (!frameDone && millis() >= actionsTimingsArray[actionsTimingsIndex]){
    // Execute a function call based on actionsArray
    // Unset actions will call 0, because they're not set
    if (actionsArray[actionsTimingsIndex] == 0){
      //Lights Off
      SetOutputs (false);
      if (PrintExtraDebug) Serial.println(F("TE:FUNC0"));
    }
    else if (actionsArray[actionsTimingsIndex] == 1){
      //Lights On
      SetOutputs (true);
      if (PrintExtraDebug) Serial.println(F("TE:FUNC1"));
    }
    else if (actionsArray[actionsTimingsIndex] == 2){
      //Done with the frame
      frameDone = true;
      if (PrintExtraDebug) Serial.println(F("TE:FUNC2"));
    }
    else {
      if (PrintExtraDebug) Serial.println(F("TE:ERROR Tried to call an unkown function number"));
    }
    
    actionsTimingsIndex ++;
    //TODO: if (actionsTimingsIndex > LENGTHOFACTIONSTIMINGSINDEX)frameDone = true;
  }
}


void CommunicateWithUser(){
  //TODO: SERIAL COMMS
  //if (newPulseFaded && PrintExtraDebug) PrintDebugBlob();
  PrintDebugBlob();
}


//// Generic Tool Functions ///////////////////////////////////////////////////////////////////////
unsigned long AverageArray (unsigned long * array, int len)  // assuming array is int.
{
  unsigned long sum = 0L ;  // sum will be larger than an item, long for safety.
  for (int i = 0 ; i < len ; i++)
    sum += array [i] ;
  return  ( sum) / len ;
}

//// Debug Functions ///////////////////////////////////////////////////////////////////////////////

void PrintDebugBlob(){
  Serial.print("Millis: ");
  Serial.print(millis());
  Serial.print(" A9:");
  Serial.println(analogRead(OPTOSENSORPIN));


  Serial.println(F("============================================"));
  Serial.print(F("Millis: ")); Serial.println(millis());
  Serial.print(F("new Frame Detected: ")); Serial.println(newFrameDetected);
  Serial.print(F("new Frame Faded:    ")); Serial.println(newPulseFaded);
  Serial.print(F("Frame Done:         ")); Serial.println(frameDone);
  
  Serial.print(F("frameDurationArray: "));
  for (int i = 0 ; i < FRAMEDURATIONARRAYSIZE ; i++){
    if (i > 0) Serial.print(F(", "));
    Serial.print( frameDurationArray[i]);
  }
  Serial.println();
  Serial.print(F("frameDurationIndex: ")); Serial.print(frameDurationIndex);
  Serial.println();
  Serial.print(F("actionsTimingsArray: "));
  for (int i = 0 ; i < 10 ; i++){
    if (i > 0) Serial.print(F(", "));
    Serial.print( actionsTimingsArray[i]);
  }
  Serial.println();
  Serial.print(F("actionsTimingsIndex: ")); Serial.println(actionsTimingsIndex);
  Serial.print(F("actionsTimings Function: ")); Serial.println(actionsArray[actionsTimingsIndex]);
  Serial.println();
  
  Serial.println(F("============================================"));
}
