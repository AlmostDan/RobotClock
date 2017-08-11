/**

uArm Pro version of paper clock written by Dominik Franek

Based on uArm Metal by Alex Thiel

See Alexes original instructions and uArm Metal code links at 
https://forum.ufactory.cc/t/tutorial-for-beginners-and-intermediate-make-your-own-uarm-clockbot/126

DIGIT BLOCKS IN ARRAY, human view, top(0) is towards the arm base
    0
   ---   
1|    2| 
  3---   
4|    5|  
   --- 
    6
**/

#define START_TIME 7 // current time minute

// To be used with serial monitor
#define DEBUG_PRINT 0

#define SPEED 5000 //mm/min

#define ZERO_LEVEL 5 // zero Z = height for the arm


// Dimensions of the block the digits are made of.
#define BLOCK_WIDTH 2.5
#define BLOCK_LENGTH 7.5
#define BLOCK_GAP 0.5 // 0 - blocks will touch by corners. Not recommended. Higher value is a gap in both X and Y coordinates.
#define DIGIT_WIDTH (BLOCK_LENGTH + 2 * BLOCK_WIDTH + 2 * BLOCK_GAP)
#define DIGIT_HEIGHT (2 * BLOCK_LENGTH + 3 * BLOCK_WIDTH + 4 * BLOCK_GAP)

// Positions of the digits bottom left corner, viewed from the arm base.
#define DIGIT_0_X 100 
#define DIGIT_0_Y -100 
#define DIGIT_1_X 100 
#define DIGIT_1_Y 100 - BLOCK_WIDTH

float height = 0, stretch = 0, rotation = 0, wrist = 0;
int time[2] = {1,7};        //  Holds the first two digits of the current time
long millisSinceUpdate = 0;
boolean runClock = false;   //  If true, then the refreshClock() function will run.

//  Location of the paper bank
//int pickup[4] = {15, -53, 54, -69};   //{Stretch, Height, Rotation, wrist}  s:h:r:w:
int pickup[4] = {75, -100, 0, -45};   //{X, Y, wrist}  s:h:r:w:
int clock[2][7][4];
 
// Calculates wrist angle based on tile position and whether it is horizontal or vertical.
float posToWrist(float x, float y, bool horizontal) 
{
  float angle = atan2(y, x) * 180 / 3.14159265;
  if (horizontal)
    angle += y < 0 ? 90 : -90; 
  return angle;
}

// Creates basic coordinates with (0,0) at the bottom left corner of it when viewed from the robot base, top right when looking at the digit, top right of the bar no. 0
void buildDigit(int index) 
{
  // shortcuts
  float w = BLOCK_WIDTH;
  float w2 = BLOCK_WIDTH / 2;
  float l = BLOCK_LENGTH;
  float l2 = BLOCK_LENGTH / 2;
  float g = BLOCK_GAP;
  float x,y;

  // {X, Y, wrist}, Z is always ZERO_LEVEL, wrist will be calculated after transposition.
  clock[index][0][0] = w + g + l2;  // horizontal bar closest to the arm base. l2, w2 is to get bar center coords. X
  clock[index][0][1] = w2; // Y
  clock[index][1][0] = w + 2*g + l + w2;
  clock[index][1][1] = w + g + l2;
  clock[index][2][0] = w2;
  clock[index][2][1] = w + g + l2;
  clock[index][3][0] = w + g + l2;
  clock[index][3][1] = w + 2*g + l + w2;
  clock[index][4][0] = w + 2*g + l + w2;
  clock[index][4][1] = 2*w + 3*g + l + l2;
  clock[index][5][0] = w2;
  clock[index][5][1] = 2*w + 3*g + l + l2;
  clock[index][6][0] = w + g + l2;
  clock[index][6][1] = 2*w + 4*g + 2*l + w2;
}

// move digit
void positionDigit(int index, float x, float y)
{
  for (int i = 0; i < 7; ++i)
  {
    clock[index][i][0] += x;
    clock[index][i][1] += y;
  }
}

// calculate wrist angles based on block positions and orientations
void recalculateWrist(int index)
{
  clock[index][0][2] = posToWrist(clock[index][0][0], clock[index][0][1], true);
  clock[index][1][2] = posToWrist(clock[index][1][0], clock[index][1][1], false);
  clock[index][2][2] = posToWrist(clock[index][2][0], clock[index][2][1], false);
  clock[index][3][2] = posToWrist(clock[index][3][0], clock[index][3][1], true);
  clock[index][4][2] = posToWrist(clock[index][4][0], clock[index][4][1], false);
  clock[index][5][2] = posToWrist(clock[index][5][0], clock[index][5][1], false);
  clock[index][6][2] = posToWrist(clock[index][6][0], clock[index][6][1], true);
}


void prepareDigits() {
  buildDigit(0);
  positionDigit(0, DIGIT_0_X, DIGIT_0_Y);
  recalculateWrist(0);
  buildDigit(1);
  positionDigit(1, DIGIT_1_X, DIGIT_1_Y );
  recalculateWrist(1);
}

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
  Serial.begin(115200);                  //  Set up serial
  
  if (DEBUG_PRINT)
    Serial.println(F("Beginning uArmWrite"));

  if (START_TIME >= 0 && START_TIME < 60) {
    time[0] = floor(START_TIME / 10);
    time[1] = START_TIME % 10;
    millisSinceUpdate = millis();
    runClock = true;
  }

  Serial.println("M2400 S0"); // set mode to default
}


void loop(){
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

  if (DEBUG_PRINT)
    Serial.print("Building number: " + String(time[0]) + " " + String(time[1]));
  
  if(buildNumber(time[0], 1)){
    //  Once the first digit is complete, it will move on to the next if statement, where it proceeds to work on the next digit.
  }       
  else if(buildNumber(time[1], 0)){
    moveTo(100, 50, 0,0);
  }
  if (DEBUG_PRINT)
    Serial.print("\n");
}

boolean buildNumber(int num, int digit){
  //  This function either removes or places a segment each time it is run. If run x times, it will eventually build "num" on "digit"
  
  //  Pick a segment that needs to be Removed and place it, then exit the program
  for(int i = 0; i < 7; i++){  //  Check all segments
    if(!numbers[num][i] & currentSegments[digit][i]){  //  If there should NOT be a segment present, and there happens to be one, then take it away.
      if (DEBUG_PRINT)
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
      if (DEBUG_PRINT)
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
  pumpOn();
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

  pumpOn();
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
  
  pumpOff();  //  Release, then keep pushing down to help "normalize" the stack
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
  pumpOff();
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

// ----- uArm interfaces

// sucker
void pumpOn(void) {
  Serial.println("M2231 V1");
}

void pumpOff(void) {
  Serial.println("M2231 V0");
}

//  movement
void moveTo(int s, int h, int r, int w){  //  Moves robot to a position, and records the variables. 
  height =   h + ZERO_LEVEL; 
  stretch =  s;
  rotation = r;
  wrist =    w;
  
  Serial.print("G2201 S");
  Serial.print(stretch);
  Serial.print(" R");
  Serial.print(rotation);
  Serial.print(" H");
  Serial.print(height);
  Serial.print(" F");
  Serial.println(SPEED);

  Serial.print("G2202 N3 V");
  Serial.println(wrist);
}

