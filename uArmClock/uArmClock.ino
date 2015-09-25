/**
Created by Alex Thiel

LOCATION BLOCKS IN ARRAY
    0
   ---   
1|    2| 
  3---   
4|    5|  
   --- 
    6
**/

#include <UF_uArm_Metal_new.h>
#include <VarSpeedServo.h>
#include <EEPROM.h>
#include <Wire.h>
 
UF_uArm uarm;               //  initialize the uArm library 

double height = 0, stretch = 0, rotation = 0, wrist = 0;
int time[2] = {1,7};       //  Holds the first two digits of the current time
long millisSinceUpdate = 0;
boolean runClock = false;  //  If true, then the refreshClock() function will run.

//  Location of where to pickup the papers
int pickup[4] = {15, -53, 54, -69};   //{Stretch, Height, Rotation, wrist}  s:h:r:w:
 
//  Location of every paper in the clock {rotation, stretch, height}
int clock[2][7][4] = {{{  0, -49, -47,  -54}, //  CLOCK 1
                       { 29, -55, -22,    8},  
                       { 72, -61, -54,   40}, 
                       { 90, -60, -35,  -71},
                       {120, -72, -20,    7},
                       {155, -64, -42,   27},
                       {193, -70, -28,  -77}
                      },                      //  CLOCK 2
                      {{  0, -49,  18,  57},
                       { 72, -60,  25, -37},
                       { 33, -51,  -7,  -8},
                       { 85, -62,   5,  63},
                       {156, -68,  11, -26},
                       {125, -65, -10,  -7},
                       {192, -76,  -2,  70}}
                     };

//  Keeps track of the locations of the papers currently on the board. 
//  True means that there is a paper present at that location. The first array is for clock 0, and the second for clock 1
int currentSegments[2][7] = {{false, false, false, false, false, false, false},{false, false, false, false, false, false, false}};

//  This is a database of how digital numbers are constructed. A true means that the segment is there for that number, while false means it must not be there.   
const boolean numbers[10][7] =  {{  true,  true,  true, false,  true,  true,  true},   //  #0   Location of segments in array:     
                                 { false, false,  true, false, false,  true, false},   //  #1        0  ---   
                                 {  true, false,  true,  true,  true, false,  true},   //  #2        1 |   | 2 
                                 {  true, false,  true,  true, false,  true,  true},   //  #3        3  ---   
                                 { false,  true,  true,  true, false,  true, false},   //  #4        4 |   | 5  
                                 {  true,  true, false,  true, false,  true,  true},   //  #5        6  --- 
                                 {  true,  true, false,  true,  true,  true,  true},   //  #6        
                                 {  true, false,  true, false, false,  true, false},   //  #7
                                 {  true,  true,  true,  true,  true,  true,  true},   //  #8
                                 {  true,  true,  true,  true, false,  true, false}};  //  #9                               
                               
void setup(){
  Wire.begin();          // Join i2c bus (address optional for master)
  
  Serial.begin(9600);    // Start serial port at 9600 bps
  Serial.setTimeout(10); //  Super important. Makes Serial.parseInt() not wait a long time after an integer has ended.
  
  uarm.init();           // initialize the uArm position
  uarm.setServoSpeed(SERVO_R,   255);  // 0=full speed, 1-255 slower to faster
  uarm.setServoSpeed(SERVO_L,   255);  // 0=full speed, 1-255 slower to faster
  uarm.setServoSpeed(SERVO_ROT, 255);  // 0=full speed, 1-255 slower to faster
  
   
  Serial.println(F("Beginning uArmWrite"));
}


void loop(){
  
  uarm.calibration();
  uarm.gripperDetach();  //  delay release valve, this function must be in the main loop

  //  HANDLE SERIAL INPUT 
  while(Serial.available() > 0){  //  USE SERIAL TO SEND A STRING OF TWO NUMBERS, WHICH SETS THE CLOCK TO THE CURRENT TIME. 
    String currentTime = Serial.readString();

    if(currentTime.length() == 2){  //  Make sure it's a valid string
      time[0] = currentTime.substring(0,1).toInt();
      time[1] = currentTime.substring(1,2).toInt();
      millisSinceUpdate = millis();
      runClock = true;   
    }else{
      Serial.println(F("ERROR: Invalid time length. Number must be two digits"));
    }
  }
  
  //  Add or remove segments on the clock to get them as close as possible to the correct value of the time
  refreshClock();
} 




//  Clock functions (keeping track of segments, building the numbers, etc)
void refreshClock(){
  //  Adds or removes one segment every time it is run. This is so that if the time changes while building a number,
  //  it will immediately switch to building the next number when it is called. 
  
  if(!runClock){ return;} //  If the function has not been commanded to start from the serial, then 'time' will always be -1
  
  //  If it has been one minute, update the clock to match
  if(millis()- millisSinceUpdate >= 45000){
    //  Add one minute to the clock, and reset digits to 0 when appropriate
    time[1] += 1;
    if(time[1] >= 10){ 
      time[1] = 0;
      time[0] += 1;
    }
    if(time[0] >= 6){ time[0] = 0;}
    millisSinceUpdate = millis();  //  Reset the millis() counter
  }
  
  Serial.print("Building number: " + String(time[0]) + " " + String(time[1]));
  
  if(buildNumber(time[0], 1)){
    //  Once the first digit is complete, it will move on to the next if statement, where it proceeds to work on the next digit.
  }       
  else if(buildNumber(time[1], 0)){
    moveTo(100, 50, 0,0);
  }
  Serial.print("\n");
}

boolean buildNumber(int num, int digit){
  //  This function either removes or places a segment each time it is run. If run x times, it will eventually build "num" on "digit"
  
  //  Pick a segment that needs to be Removed and place it, then exit the program
  for(int i = 0; i < 7; i++){  //  Check all segments
    if(!numbers[num][i] & currentSegments[digit][i]){  //  If there should NOT be a segment present, and there happens to be one, then take it away.
      Serial.print("\t Removing Segment" + String(i) + " on digit " + String(digit)); 
      pickupPaper(clock[digit][i]);
      placeStack();
      currentSegments[digit][i] = false;
      return true;       //  Returns true if it did an action
    }
  }
  
  //  Pick a segment that needs to be placed and place it, then exit the program
  for(int i = 0; i < 7; i++){  //  Check all segments
    if(numbers[num][i] & !currentSegments[digit][i]){  //  If there should be a segment present in this spot, and if there isn't already one, then place one.
      Serial.print("\t Placing Segment " + String(i) + " on digit " + String(digit)); 
      pickupPaperStack();
      placePaper(clock[digit][i]);
      currentSegments[digit][i] = true; 
      return true;  //  Returns true if it did an action
    }
  }
  return false;  //  Returns false since it never performed any removals/placements (e.g. the number is complete
}




//  PICKUP FUNCTIONS
void pickupPaperStack(){
  //Get paper from the paper stack. This method is different from "pickupPaper()" because it is more specialized in consistent pickups from one location - the stack.
  //moveTo(100, 50, 0,0);  //  Always start from the same location to help minimize error
  //delay(250);

  moveTo( pickup[0] + 12, pickup[1] + 30, pickup[2], pickup[3]);
  delay(1000);
  moveTo( pickup[0] + 8,  pickup[1] + 20, pickup[2], pickup[3]);
  delay(500);
  moveTo( pickup[0] + 4,  pickup[1] + 10, pickup[2], pickup[3]);
  delay(500);
  moveTo( pickup[0],      pickup[1],      pickup[2], pickup[3]); //Finish moving down onto the paper
  delay(600);  
  uarm.gripperCatch();
  delay(1000);
  moveTo( pickup[0] + 4,  pickup[1] + 10, pickup[2], pickup[3]); //Move slowly so that you don't create suction and another paper flies out
  delay(500);
  moveTo(stretch, height + 40, rotation, wrist); //  Move out of the way and start picking up the block
  delay(500);
  //moveTo(100, 50, 0, 0);  //  Go back to reset point
}

void pickupPaper(int location[]){
  //  This is a generic pickup command. Enter the location in {stretch, height, rotation, wrist} format.
  //  Robot will end up in the reset position moveTo(100, 50, 0, 0)
  
  moveTo(100, 50, 0,0);  //  Always start from the same location to help minimize error
  delay(250);
  
  moveTo( location[0] + 8, location[1] + 20, location[2], location[3]);
  delay(1000);
  moveTo( location[0] + 4, location[1] + 5, location[2], location[3]);
  delay(500);
  moveTo( location[0],     location[1] - 5,  location[2], location[3]); //Finish moving down onto the paper
  delay(600);

  uarm.gripperCatch();
  delay(1000);
  moveTo(stretch + 4, height + 10, rotation, wrist); //Move slowly so that you don't create suction and another paper flies out
  delay(500);
  moveTo(stretch,     height + 40, rotation, wrist); //  Move out of the way and start picking up the block
  delay(500);
  //moveTo(100, 50, 0,0);  //  Go back to reset point
}




// PLACING FUNCTIONS
void placeStack(){
  //  This function specializes in dropping papers off onto the stack, even if they may be slightly misaligned after moving so much.
  
  moveTo(100, 50, 0, pickup[3]);  //  Always start from the same location
  delay(250);
  

  // Move towards stack and drop in slowly and carefully
  moveTo( pickup[0] + 12, pickup[1] + 30, pickup[2], pickup[3]);
  delay(1000);
  moveTo( pickup[0] + 8,  pickup[1] + 20, pickup[2], pickup[3]);
  delay(300);
  moveTo( pickup[0] + 4,  pickup[1] + 10, pickup[2], pickup[3]);
  delay(300);
  
  uarm.gripperRelease();  //  Release, then keep pushing down to help "normalize" the stack
  delay(500);

  //  Start a "pushing" sequence, which helps push irregular drops back into place, and does nothing bad for successful drops
      //  Start far-to-close push
  moveTo( pickup[0]    ,  pickup[1] + 20,      pickup[2], pickup[3]);  //  Start from higher up before starting the manuver
  delay(200);
  moveTo(pickup[0] + 43,  pickup[1] + 30,  pickup[2] + 3, pickup[3]);  //  Go to a drop-in position, it is so that you don't hit any papers along the way to getting to the pushing position
  delay(200);
  moveTo(pickup[0] + 41,  pickup[1] + 14,  pickup[2] + 3, pickup[3]);  //  Get to pushing position
  delay(300);
  moveTo(pickup[0] + 22,  pickup[1] + 17,  pickup[2] + 1, pickup[3]);  //  Go up a little
  delay(300);    
        //  Start close-to-far push NOT DONE
  moveTo( pickup[0]    ,  pickup[1] + 20,      pickup[2], pickup[3]);  //  Start from higher up before moving into pushing position
  delay(200);
  moveTo(pickup[0] - 15,  pickup[1] + 37,  pickup[2] - 4, pickup[3]);  //  Get to pushing position
  delay(300);
  moveTo(pickup[0] - 15,  pickup[1] + 21,  pickup[2] - 3, pickup[3]);
  delay(300);
      //  Start left-to-middle push
  moveTo(     pickup[0],  pickup[1] + 20,      pickup[2], pickup[3]);  //  Start from higher up before moving into pushing position
  delay(200);
  moveTo( pickup[0] - 6,  pickup[1] + 17, pickup[2] + 10, pickup[3]);  //  Get to pushing position
  delay(300);
  moveTo(     pickup[0],   pickup[1] + 10,     pickup[2], pickup[3]);  //  Go up a little
  delay(300);
      //  Start right-to-middle push
  moveTo( pickup[0]    ,  pickup[1] + 20,      pickup[2], pickup[3]);  //  Start from higher up before moving into pushing position
  delay(200);
  moveTo(pickup[0] + 20,  pickup[1] + 11, pickup[2] - 10, pickup[3]);  //  Get to pushing position
  delay(300);
  moveTo(     pickup[0],  pickup[1] + 10,      pickup[2], pickup[3]);  //  Go up a little
  delay(300);
 
     //  Do one final push in the center
  moveTo( pickup[0],      pickup[1],      pickup[2], pickup[3]); //  Finish moving down onto the paper
  delay(200);  
  
  //  Back away from paper and end function
  moveTo( pickup[0] + 4,  pickup[1] + 10, pickup[2], pickup[3]); //  Move slowly so that you don't create suction and another paper flies out
  delay(400);
  moveTo(stretch, height + 40, rotation, 0); //  Move out of the way and start picking up the block
  delay(300);
  //moveTo(100, 50, 0,0);  //  Go back to reset point

}

void placePaper(int location[]){
  //  Place paper on one of the segments of the clock
  //  int spot is the particular segment of the number you are working with. c is the clock, or the actual digit
  moveTo(100, 50, 0,0);  //  Always start from the same location
  delay(250);
  moveTo(location[0], location[1] + 40, location[2], location[3]);
  delay(750);
  moveTo(stretch, height - 40, rotation, wrist);
  delay(500);
  uarm.gripperRelease();
  delay(600);
  
  //  Raise slowly so as to not disturb the papers new position, then return to home position
  moveTo(stretch + 1, height + 5, rotation, wrist);
  delay(500);
  moveTo(stretch + 1, height + 5, rotation, wrist);
  delay(500);
  moveTo(stretch + 1, height + 5, rotation, wrist);
  delay(500);
  moveTo(stretch + 5, height + 15, rotation, wrist);
  delay(500);
  //moveTo(100,50,0,0);
}




//  OTHER
void moveTo(int s, int h, int r, int w){  //  Moves robot to a position, and records the variables. 
  height = h; 
  //uarm.setPosition(stretch, height, rotation, wrist);
  //delay(250);        //By having the move split in two, theres little chance that the robot will knock anything over
  stretch = s;
  rotation = r;
  wrist = w;
  uarm.setPosition(stretch, height, rotation, wrist);
}

