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

// -------------------------------------------------------------- DEFINES

#define START_TIME 7 // current time minute

// To be used with serial monitor
#define DEBUG_PRINT 0

#define SPEED 5000 // general move around speed, mm/min
#define APPROACH_SPEED 1000 // speed of approach to the blocks, mm/min

#define ZERO_LEVEL 5 // zero Z = height for the arm
#define MOVE_LEVEL (50 + ZERO_LEVEL) // height above 0 in which the arm moves around

#define WRIST_OFFSET 0 // Use this to correct wrong wrist callibration.

// Dimensions of the block the digits are made of.
#define BLOCK_WIDTH 25
#define BLOCK_LENGTH 75
#define BLOCK_GAP 5 // 0 - blocks will touch by corners. Not recommended. Higher value is a gap in both X and Y coordinates.
#define DIGIT_WIDTH (BLOCK_LENGTH + 2 * BLOCK_WIDTH + 2 * BLOCK_GAP)
#define DIGIT_HEIGHT (2 * BLOCK_LENGTH + 3 * BLOCK_WIDTH + 4 * BLOCK_GAP)

// Positions of the digits bottom left corner, viewed from the arm base.
#define DIGIT_0_X 100 
#define DIGIT_0_Y -100 
#define DIGIT_1_X 100 
#define DIGIT_1_Y (100 - BLOCK_WIDTH)

// block storage box
#define BOX_X 100
#define BOX_Y -150
#define BOX_TOP (15 + ZERO_LEVEL) // height of the box

// position of a block. Z is assumed ZERO_LEVEL.
typedef struct Pos
{
  float x;
  float y;
  int wrist; // angle of the wrist
};  

float height = 0, stretch = 0, rotation = 0, wrist = 0;
int time[2] = {1,7};        //  Holds the first two digits of the current time
long millisSinceUpdate = 0;
boolean runClock = false;   //  If true, then the refreshClock() function will run.

//  Location of the paper bank
Pos pickup = {BOX_X, BOX_Y, 0};   //{X, Y, wrist}, wrist is calculated in setup
Pos clock[2][7];

// -------------------------------------------------------------- SUPPORT FUNCTIONS
 
// Calculates wrist angle based on tile position and whether it is horizontal or vertical.
float posToWrist(float x, float y, bool horizontal) 
{
  float angle = atan2(-y, x) * 180 / 3.14159265;
  if (horizontal)
    angle += y < 0 ? -90 : 90; 
  return angle + 90; // facing forward is, by arm design, 90, instead of the expected 0.
}

// -------------------------------------------------------------- UARM INTERFACE

// sucker
void pumpOn(void) {
  Serial.println("M2231 V1");
}

void pumpOff(void) {
  Serial.println("M2231 V0");
}

void turnWrist(int angle)
{
  Serial.print("G2202 N3 V");
  Serial.println(wrist + WRIST_OFFSET);
}

void moveTo(float x, float y, float z, int speed = SPEED)
{
  Serial.print("G0 X");
  Serial.print(x);
  Serial.print(" Y");
  Serial.print(y);
  Serial.print(" Z");
  Serial.print(z);
  Serial.print(" F");
  Serial.println(speed);
}

// position arm and turn wrist in one command
void moveArm(float x, float y, float z, int wrist, int speed = SPEED)
{
  moveTo(x, y, z, speed);
  turnWrist(wrist);
}  

// -------------------------------------------------------------- DIGIT COORDINATES

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
  clock[index][0].x = w + g + l2;  // horizontal block closest to the arm base. l2, w2 is to get bar center coords. X
  clock[index][0].y = w2; // Y
  clock[index][1].x = w + 2*g + l + w2; // etc., by block numbering above.
  clock[index][1].y = w + g + l2;
  clock[index][2].x = w2;
  clock[index][2].y = w + g + l2;
  clock[index][3].x = w + g + l2;
  clock[index][3].y = w + 2*g + l + w2;
  clock[index][4].x = w + 2*g + l + w2;
  clock[index][4].y = 2*w + 3*g + l + l2;
  clock[index][5].x = w2;
  clock[index][5].y = 2*w + 3*g + l + l2;
  clock[index][6].x = w + g + l2;
  clock[index][6].y = 2*w + 4*g + 2*l + w2;
}

// move digit
void positionDigit(int index, float x, float y)
{
  for (int i = 0; i < 7; ++i)
  {
    clock[index][i].x += x;
    clock[index][i].y += y;
  }
}

// calculate wrist angles based on block positions and orientations
void recalculateWrist(int index)
{
  clock[index][0].wrist = posToWrist(clock[index][0].x, clock[index][0].y, true);
  clock[index][1].wrist = posToWrist(clock[index][1].x, clock[index][1].y, false);
  clock[index][2].wrist = posToWrist(clock[index][2].x, clock[index][2].y, false);
  clock[index][3].wrist = posToWrist(clock[index][3].x, clock[index][3].y, true);
  clock[index][4].wrist = posToWrist(clock[index][4].x, clock[index][4].y, false);
  clock[index][5].wrist = posToWrist(clock[index][5].x, clock[index][5].y, false);
  clock[index][6].wrist = posToWrist(clock[index][6].x, clock[index][6].y, true);
}


void prepareDigits() 
{
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


// -------------------------------------------------------------- PICKUP FUNCTIONS

//Get paper from the paper stack. This method is different from "pickupPaper()" because it is more specialized in consistent pickups from one location - the stack.
void pickupPaperStack()
{
  pickupPaper(pickup);
}

//  This is a generic pickup command. Enter the location in {stretch, height, rotation, wrist} format.
void pickupPaper(Pos pos)
{
  // TODO maybe insert itermediate point on the way from the box to the numbers
  moveTo(pos.x, pos.y, MOVE_LEVEL); // move above the block
  delay(1000);
  turnWrist(pos.wrist);
  
  moveTo(pos.x, pos.y, ZERO_LEVEL, APPROACH_SPEED); // drop down onto the block
  delay(500);

  pumpOn();

  delay(500);
  moveTo(pos.x, pos.y, MOVE_LEVEL, APPROACH_SPEED); // move up
  delay(500);
  // ready to accept next command
}




// -------------------------------------------------------------- PLACING FUNCTIONS

//  This function specializes in dropping papers off onto the stack, even if they may be slightly misaligned after moving so much.
void placeStack()
{

  // Move towards stack
  moveTo( pickup.x, pickup.y, MOVE_LEVEL);
  delay(1000);
  // move right above the stack
  moveTo( pickup.x, pickup.y, BOX_TOP, APPROACH_SPEED);
  delay(500);
  
  pumpOff();  //  Drop the paper
  delay(500);

  //  Start a "pushing" sequence, which helps push irregular drops back into place, and does nothing bad for successful drops
  // TODO write this procedure
  //  Start far-to-close push
  moveTo( pickup.x, pickup.y, BOX_TOP, APPROACH_SPEED);
  delay(500);    
        //  Start close-to-far push NOT DONE
  moveTo( pickup.x, pickup.y, BOX_TOP, APPROACH_SPEED);
  delay(500);
      //  Start left-to-middle push
  moveTo( pickup.x, pickup.y, BOX_TOP, APPROACH_SPEED);
  delay(500);
      //  Start right-to-middle push
  moveTo( pickup.x, pickup.y, BOX_TOP, APPROACH_SPEED);
  delay(500);
 
  //  Do one final push in the center
  moveTo( pickup.x, pickup.y, BOX_TOP, APPROACH_SPEED);
  delay(500);
  moveTo( pickup.x, pickup.y, ZERO_LEVEL, APPROACH_SPEED);
  delay(500);
  
  //  Back away from paper and end function
  moveTo( pickup.x, pickup.y, MOVE_LEVEL, APPROACH_SPEED);
  delay(1000);
}

/*  Place paper on one of the segments of the clock
 *  int spot is the particular segment of the number you are working with. c is the clock, or the actual digit
 */
void placePaper(Pos pos)
{
  moveTo(pos.x, pos.y, MOVE_LEVEL);
  delay(1000);
  turnWrist(pos.wrist);
  delay(500);
  moveTo(pos.x, pos.y, ZERO_LEVEL, APPROACH_SPEED);
  delay(1000);
  
  pumpOff();
  delay(500);
  
  moveTo(pos.x, pos.y, MOVE_LEVEL, APPROACH_SPEED);
  delay(1000);
}

// -------------------------------------------------------------- WRITING ALGORITHM

//  Clock functions (keeping track of segments, building the numbers, etc)
void refreshClock()
{
  //  Adds or removes one segment every time it is run. This is so that if the time changes while building a number,
  //  it will immediately switch to building the next number when it is called. 
  
  if (!runClock)
    return; //  If the function has not been commanded to start from the serial, then 'time' will always be -1
  
  //  If it has been one minute, update the clock to match
  if (millis()- millisSinceUpdate >= 45000)
  {
    //  Add one minute to the clock, and reset digits to 0 when appropriate
    time[1] += 1;
    if (time[1] >= 10){ 
      time[1] = 0;
      time[0] += 1;
    }
    if (time[0] >= 6){ time[0] = 0;}
    millisSinceUpdate = millis();  //  Reset the millis() counter
  }

  if (DEBUG_PRINT)
    Serial.print("Building number: " + String(time[0]) + " " + String(time[1]));
  
  if (buildNumber(time[0], 1))
  {
    //  Once the first digit is complete, it will move on to the next if statement, where it proceeds to work on the next digit.
  }       
  else if (buildNumber(time[1], 0))
  {
    moveTo(100, 0, MOVE_LEVEL); // some default pos
  }
  if (DEBUG_PRINT)
    Serial.print("\n");
}

boolean buildNumber(int num, int digit)
{
  //  This function either removes or places a segment each time it is run. If run x times, it will eventually build "num" on "digit"
  
  //  Pick a segment that needs to be Removed and place it, then exit the program
  for(int i = 0; i < 7; i++)  //  Check all segments
  {  
    if (!numbers[num][i] & currentSegments[digit][i])  //  If there should NOT be a segment present, and there happens to be one, then take it away.
    {
      if (DEBUG_PRINT)
        Serial.print("\t Removing Segment" + String(i) + " on digit " + String(digit)); 
      pickupPaper(clock[digit][i]);
      placeStack();
      currentSegments[digit][i] = false;
      return true;       //  Returns true if it did an action
    }
  }
  
  //  Pick a segment that needs to be placed and place it, then exit the program
  for(int i = 0; i < 7; i++)  //  Check all segments
  {  
    if (numbers[num][i] & !currentSegments[digit][i])  //  If there should be a segment present in this spot, and if there isn't already one, then place one.
    {
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


// -------------------------------------------------------------- INIT                                
void setup()
{
  Serial.begin(115200);                  //  Set up serial

  pickup.wrist = posToWrist(pickup.x, pickup.y, false);
  
  if (DEBUG_PRINT)
    Serial.println(F("Beginning uArmWrite"));

  if (START_TIME >= 0 && START_TIME < 60) 
  {
    time[0] = floor(START_TIME / 10);
    time[1] = START_TIME % 10;
    millisSinceUpdate = millis();
    runClock = true;
  }

  Serial.println("M2400 S0"); // set mode to default
}

// -------------------------------------------------------------- MAIN LOOP

void loop()
{
   //  Add or remove segments on the clock to get them as close as possible to the correct value of the time
  refreshClock();
} 




