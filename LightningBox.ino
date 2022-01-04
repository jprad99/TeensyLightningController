#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioPlaySdWav           playSdWav1;     //xy=208,279
AudioAnalyzeToneDetect   tone2;          //xy=460,424
AudioAnalyzeToneDetect   tone3;          //xy=461,458
AudioAnalyzeToneDetect   tone4;          //xy=461,495
AudioAnalyzeToneDetect   tone1;          //xy=462,387
AudioAnalyzeToneDetect   tone5;          //xy=462,531
AudioOutputAnalogStereo  dacs1;          //xy=464,279
AudioAnalyzeToneDetect   tone6;          //xy=463,571
AudioConnection          patchCord1(playSdWav1, 0, dacs1, 0);
AudioConnection          patchCord2(playSdWav1, 0, tone1, 0);
AudioConnection          patchCord3(playSdWav1, 0, tone2, 0);
AudioConnection          patchCord4(playSdWav1, 0, tone3, 0);
AudioConnection          patchCord5(playSdWav1, 0, tone4, 0);
AudioConnection          patchCord6(playSdWav1, 0, tone5, 0);
AudioConnection          patchCord7(playSdWav1, 0, tone6, 0);
AudioConnection          patchCord8(playSdWav1, 1, dacs1, 1);
// GUItool: end automatically generated code

// Use these with the Teensy 3.5 & 3.6 SD card
#define SDCARD_CS_PIN    BUILTIN_SDCARD
#define SDCARD_MOSI_PIN  11  // not actually used
#define SDCARD_SCK_PIN   13  // not actually used
//#define DEBUG

//define things
#define timer0_duration 30
#define NUM_DIMMERS 6
#define NUM_DIMMERS_USED 6



//set relay pins!
int zeroCrossPin = 2;
int fadeValues[NUM_DIMMERS];

const int syncPin = 2;

const int dim1 = 3;
const int dim2 = 4;
const int dim3 = 5;
const int dim4 = 6;
const int dim5 = 7;
const int dim6 = 8;



char audioFiles[100];
int numberFiles;

int DIMMERS[NUM_DIMMERS] = {dim1,dim2,dim3,dim4,dim5,dim6};

elapsedMicros sinceInterrupt;
unsigned long thePeriod = 0;

volatile unsigned int timerFire_cnt = 0;

IntervalTimer timer0;

void setup() {
  Serial.begin(9600);

//-----------------------TRIAC Control---------------------
  pinMode(zeroCrossPin, INPUT);
  attachInterrupt(zeroCrossPin, zero_crosss_int, RISING);

  timer0.begin(timerFire, timer0_duration);

  for(int i=0; i <NUM_DIMMERS_USED;i++){
    pinMode(DIMMERS[i], OUTPUT);        // Set the AC Load as output

  }
//---------------------------------------------------------
  
  AudioMemory(12);
  Serial.println("Done!");

  //set frequencies to be analyzed
  tone1.frequency(40);
  tone2.frequency(65);
  tone3.frequency(90);
  tone4.frequency(115);
  tone5.frequency(150);
  tone6.frequency(190);


  //Connect to the SD card
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  
}

double mapf(double x, double in_min, double in_max, double out_min, double out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void printDirectory(File dir, int numTabs) {

  for (int i = 0; i<100; i++) {

    File entry =  dir.openNextFile();

    if (! entry) {

      // no more files

      break;

    }

    for (uint8_t i = 0; i < numTabs; i++) {

      Serial.print('\t');

    }

    Serial.print(entry.name());

    audioFiles[i] = entry.name();
    numberFiles = i;

    if (entry.isDirectory()) {

      Serial.println("/");

      printDirectory(entry, numTabs + 1);

    } else {

      // files have sizes, directories do not

      Serial.print("\t\t");

      Serial.println(entry.size(), DEC);

    }

    entry.close();

  }
}

void playFile(const char *filename) {
  //Serial.print("Playing file: ");
  //Serial.println(filename);

  // Start playing the file.  This sketch continues to
  // run while the file plays.
  playSdWav1.play(filename);

  // A brief delay for the library read WAV info
  while (playSdWav1.isPlaying() == false){
    //wait for it to start!
  }
  // Run Lightning!
  while (playSdWav1.isPlaying()) {
    double ch1, ch2, ch3, ch4, ch5, ch6, val, min_level;
    ch1 = tone1.read();
    ch2 = tone2.read();
    ch3 = tone3.read();
    ch4 = tone4.read();
    ch5 = tone5.read();
    ch6 = tone6.read();
    //Serial.println(AudioMemoryUsageMax());
    //Serial.println(AudioProcessorUsageMax());
    min_level = 1;
    double thunder[] = {ch1, ch2, ch3, ch4, ch5, ch6};
    for (byte i = 0; i < (sizeof(thunder) / sizeof(thunder[0])); i++) {
      val = thunder[i]*100;
      //Serial.print(String(val)+", ");
      if (val > min_level){
        min_level = val;
      } 
    }
    //Serial.println(min_level);
    
    for (byte i = 0; i < (sizeof(thunder) / sizeof(thunder[0])); i++) { 
      fadeValues[i] = mapf(thunder[i]*100,0,min_level,8333,0);
      Serial.print(fadeValues[i]);
    }
    Serial.println();
    //delay(50);
  }
  //This is where a reset could occur after?
}

void rotate(int arr[], int n)
{
  int x = arr[n - 1], i;
  for (i = n - 1; i > 0; i--)
    arr[i] = arr[i - 1];
  arr[0] = x;
}

void zero_crosss_int(){
  // Ignore spuriously short interrupts
  if (sinceInterrupt < 2000){
#ifdef DEBUG
    Serial.println("sinceInterrupt < 2000 ");
#endif

    return;
  }

  thePeriod = sinceInterrupt; //ca. 10012 at 50Hz location, 8300 at 60 hz in montreal

  sinceInterrupt = 0;

#ifdef DEBUG
  Serial.print("timerFire_cnt ");
  Serial.println(timerFire_cnt);
  timerFire_cnt = 0;
#endif
  // Serial.print("----------thePeriod----------");
  // Serial.println(thePeriod);

  for(int i=0; i<NUM_DIMMERS_USED;i++){
    digitalWrite(DIMMERS[i], LOW);
  }
}


void timerFire(void){
  for(int i=0; i<NUM_DIMMERS_USED;i++){
    if(sinceInterrupt >= fadeValues[i] ){
      // if(temp_sinceInterrupt >= fadeValues[i] ){
      //for first part of 1/2 sinus curve triac is off, then we turn triac on by setting pin high
      //triac automatically turns on next zero crossing
      //but not arduino pin
      digitalWrite(DIMMERS[i], HIGH);
    }
  }
}


void loop() {
  rotate(DIMMERS, 1);
  //Serial.println("Still Running: " + String(millis()));
  //int file2play = random(0,numberFiles);
  playFile("0001.WAV");
}
