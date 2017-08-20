/**

uArm Pro version of paper clock written by Dominik Franek

Based on uArm Metal version by Alex Thiel

See Alexes original instructions and uArm Metal code links at 
https://forum.ufactory.cc/t/tutorial-for-beginners-and-intermediate-make-your-own-uarm-clockbot/126

For this version, follow the instructions, but use blocks of size 60x20 instead of 75x25.
Als use appropriate sized box. Box I use is 60x20 at the base and widens towards the top,
where it is 65x25, making the blocks fall in place naturally.

Digit blocks array indices, human view orientation, top - 0 - is towards the arm base
    0
   ---
1|    2|
  3---
4|    5|
   ---
    6

**/

// -------------------------------------------------------------- DEFINES

#define START_TIME 42  // current time minute

// To be used with serial monitor
#define DEBUG_PRINT 0

#define SPEED 10000 // general move around speed, mm/min
#define APPROACH_SPEED 3000 // speed of approach to the blocks, mm/min

#define MOVE_TIME 0 //5000 // time to perform move order
#define APPROACH_TIME 0 //3000 // time to descend to/ascend from a block
#define PUMP_TIME 500 // time to wait after pump state change
#define WRIST_TIME 500 // time for wrist turn
#define SPEED_CONSTANT 200000 // time to perform arm move is calculated from this constant 

#define ZERO_LEVEL 0 // zero Z = height for the arm
#define MOVE_LEVEL (20 + ZERO_LEVEL) // height above 0 in which the arm moves around

#define WRIST_CORRECTION 10 // Use this to correct wrong wrist callibration.

// Dimensions of the block the digits are made of. This is for uArm Pro, original for Metal is 25x75
#define BLOCK_WIDTH 20
#define BLOCK_LENGTH 60
#define BLOCK_GAP 3 // 0 - blocks will touch by corners. Not recommended. Higher value is a gap in both X and Y coordinates.
#define DIGIT_WIDTH (BLOCK_LENGTH + 2 * BLOCK_WIDTH + 2 * BLOCK_GAP)
#define DIGIT_HEIGHT (2 * BLOCK_LENGTH + 3 * BLOCK_WIDTH + 4 * BLOCK_GAP)

// Positions of the digits bottom left corner, viewed from the arm base. 0 is for minutes, 1 for tens of minuts.
#define DIGIT_0_X 120
#define DIGIT_0_Y (130 - DIGIT_WIDTH)
#define DIGIT_1_X 120
#define DIGIT_1_Y -130

// block storage box
#define BOX_X 150
#define BOX_Y -180
#define BOX_TOP (15 + ZERO_LEVEL) // height of the box

// position of a block. Z is assumed ZERO_LEVEL.
typedef struct Pos
{
  float x;
  float y;
  bool vertical; // is block vertical or horizontal
  int wrist; // angle of the wrist to catch it
};

typedef struct Point3D
{
  float x;
  float y;
  float z;
};


int time[2] = {1,7};        //  Holds the first two digits of the current time. 0 is for tens, 1 for units.
long millisSinceUpdate = 0;

Point3D lastPos; // store last known position to calculate 

//  Location of the paper bank
Pos pickup = {BOX_X, BOX_Y, true, 0};   //{X, Y, vertical, wrist}, wrist is calculated in setup
Pos clock[2][7];

// -------------------------------------------------------------- SUPPORT FUNCTIONS
 
// Calculates wrist angle based on tile position
float posToWrist(float x, float y) 
{
  float angle = atan2(y, x) * 180 / 3.14159265;
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
  Serial.println(angle + WRIST_CORRECTION);
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
  float distance = sqrt(pow(lastPos.x - x, 2) + pow(lastPos.y - y, 2) + pow(lastPos.z - z, 2));
  int delayTime = distance * SPEED_CONSTANT / speed;
  lastPos = {x, y, z};
  if (DEBUG_PRINT)
  {
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.print("; Time: ");
    Serial.println(delayTime);
  }
  delay(delayTime);
}

// position arm and turn wrist in one command
void moveArm(float x, float y, float z, int wrist, int speed = SPEED)
{
  moveTo(x, y, z, speed);
  turnWrist(wrist);
}  

// -------------------------------------------------------------- DIGIT COORDINATES

/* Creates basic digit coordinates with (0,0) at the bottom left corner of it 
 *  when viewed from the robot base, top right when looking at the digit, top right of the bar no. 0
 */
void buildDigit(int index) 
{
  // shortcuts
  float w = BLOCK_WIDTH;
  float w2 = BLOCK_WIDTH / 2;
  float l = BLOCK_LENGTH;
  float l2 = BLOCK_LENGTH / 2;
  float g = BLOCK_GAP;
  float x,y;

  // {X, Y, vertical, wrist}, Z is always ZERO_LEVEL, wrist will be calculated after transposition.
  clock[index][0].x = w2;  // horizontal block closest to the arm base. l2, w2 is to get bar center coords. X
  clock[index][0].y = w + g + l2; // Y
  clock[index][0].vertical = false;
  clock[index][1].x = w + g + l2; // etc., by block numbering above.
  clock[index][1].y = w2;
  clock[index][1].vertical = true;
  clock[index][2].x = w + g + l2;
  clock[index][2].y = w + 2*g + l + w2;
  clock[index][2].vertical = true;
  clock[index][3].x = w + 2*g + l + w2;
  clock[index][3].y = w + g + l2;
  clock[index][3].vertical = false;
  clock[index][4].x = 2*w + 3*g + l + l2;
  clock[index][4].y = w2;
  clock[index][4].vertical = true;
  clock[index][5].x = 2*w + 3*g + l + l2;
  clock[index][5].y = w + 2*g + l + w2;
  clock[index][5].vertical = true;
  clock[index][6].x = 2*w + 4*g + 2*l + w2;
  clock[index][6].y = w + g + l2;
  clock[index][6].vertical = false;
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
void recalculateDigitWrist(int index)
{
  for (int i = 0; i < 7; ++i)
  {
    clock[index][i].wrist = posToWrist(clock[index][i].x, clock[index][i].y);
  }
}


void prepareDigits()
{
  buildDigit(0);
  positionDigit(0, DIGIT_0_X, DIGIT_0_Y);
  recalculateDigitWrist(0);
  buildDigit(1);
  positionDigit(1, DIGIT_1_X, DIGIT_1_Y );
  recalculateDigitWrist(1);
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
                                 {  true,  true,  true,  true, false,  true,  true}};  //  #9                               


// -------------------------------------------------------------- PICKUP FUNCTIONS

//  This is a generic pickup command. Enter the location in {stretch, height, rotation, wrist} format.
void pickupPaper(Pos pos)
{
  moveTo(pos.x, pos.y, MOVE_LEVEL); // move above the block
  delay(MOVE_TIME);
  turnWrist(pos.wrist);
  delay(WRIST_TIME);
  
  moveTo(pos.x, pos.y, ZERO_LEVEL, APPROACH_SPEED); // drop down onto the block
  delay(APPROACH_TIME);

  pumpOn();
  delay(PUMP_TIME);
  
  moveTo(pos.x, pos.y, MOVE_LEVEL, APPROACH_SPEED); // move up
  delay(APPROACH_TIME);
  // ready to accept next command
}

/* Get paper from the paper stack.
 *  Reason for the vertical rotation is that we need to orientate the paper block for the final position already, 
 *  because of limitations given by the air tube.
 *  @param vertical at which orientation we want to pick up the block. 
 */
void pickupPaperStack(bool vertical = true)
{
  int wrist = pickup.wrist;
  if (!vertical)
  {
    wrist += 90;
  }
  pickupPaper({pickup.x, pickup.y, true, wrist});
}

// -------------------------------------------------------------- PLACING FUNCTIONS

/* This function specializes in dropping papers off onto the stack, even if they may be slightly misaligned after moving so much.
 *  @param vertical Orientation of the block held. Vertical block orientation is default. For horizontal, wrist must be turned by 90deg. 
 */
void placeStack(bool vertical = true)
{

  // Move towards stack
  moveTo( pickup.x, pickup.y, MOVE_LEVEL);
  delay(MOVE_TIME);
  
  int wrist = pickup.wrist;
  if (!vertical)
  {
    wrist += 90;
  }
  turnWrist(wrist);
  delay(WRIST_TIME);
  
  // move right above the stack
  moveTo( pickup.x, pickup.y, BOX_TOP, APPROACH_SPEED);
  delay(APPROACH_TIME);
  
  pumpOff();  //  Drop the paper
  delay(PUMP_TIME);
 
  //  Do one final push in the center, to make sure paper falls in place
  moveTo( pickup.x, pickup.y, BOX_TOP, APPROACH_SPEED);
  delay(APPROACH_TIME / 2);
  moveTo( pickup.x, pickup.y, ZERO_LEVEL, APPROACH_SPEED);
  delay(APPROACH_TIME / 2);
  
  //  Back away from paper and end function
  moveTo( pickup.x, pickup.y, MOVE_LEVEL, APPROACH_SPEED);
  delay(APPROACH_TIME);
}

/*  Place paper on one of the segments of the clock
 *  int spot is the particular segment of the number you are working with. c is the clock, or the actual digit
 */
void placePaper(Pos pos)
{
  moveTo(pos.x, pos.y, MOVE_LEVEL);
  delay(MOVE_TIME);
  turnWrist(pos.wrist);
  delay(WRIST_TIME);
  moveTo(pos.x, pos.y, ZERO_LEVEL, APPROACH_SPEED);
  delay(APPROACH_TIME);
  
  pumpOff();
  delay(PUMP_TIME);
  
  moveTo(pos.x, pos.y, MOVE_LEVEL, APPROACH_SPEED);
  delay(APPROACH_TIME);
}

// -------------------------------------------------------------- WRITING ALGORITHM

boolean buildNumber(int num, int digit)
{
  //  This function either removes or places a segment each time it is run. If run x times, it will eventually build "num" on "digit"
  
  //  Pick a segment that needs to be Removed and place it, then exit the program
  for(int i = 0; i < 7; i++)  //  Check all segments
  {  
    if (!numbers[num][i] & currentSegments[digit][i])  //  If there should NOT be a segment present, and there happens to be one, then take it away.
    {
      if (DEBUG_PRINT)
        Serial.println("\t Removing Segment" + String(i) + " on digit " + String(digit)); 
      pickupPaper(clock[digit][i]);
      placeStack(clock[digit][i].vertical);
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
        Serial.println("\t Placing Segment " + String(i) + " on digit " + String(digit)); 
      pickupPaperStack(clock[digit][i].vertical);
      placePaper(clock[digit][i]);
      currentSegments[digit][i] = true; 
      return true;  //  Returns true if it did an action
    }
  }
  return false;  //  Returns false since it never performed any removals/placements (e.g. the number is complete
}

//  Clock functions (keeping track of segments, building the numbers, etc)
void refreshClock()
{
  //  Adds or removes one segment every time it is run. This is so that if the time changes while building a number,
  //  it will immediately switch to building the next number when it is called. 
    
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
    //moveTo(120, 0, MOVE_LEVEL); // some default pos
  }
  if (DEBUG_PRINT)
    Serial.print("\n");
}

// -------------------------------------------------------------- INIT                                
void setup()
{
  Serial.begin(115200);                  //  Set up serial

  pickup.wrist = posToWrist(pickup.x, pickup.y);

  // Arm will not move here, but time for first movement to the block box will be calculated from this.
  // So this just needs to be at least as far from the box as the arm really is.
  lastPos = {100, 100, 100};

  // init digit coordinates
  prepareDigits();

  int startTime;
  // check if the starting time makes sense. If not, nothing will be happeing.
  if (START_TIME >= 0 && START_TIME < 60) 
  {
    startTime = START_TIME;
  }
  else
  {
    startTime = 0;  
  }
  

  time[0] = floor(startTime / 10);
  time[1] = startTime % 10;
  millisSinceUpdate = millis();

  // wait for arm connection
  while (!Serial.available()) 
  {
    delay(100);
  }
  delay(1000);
  
  Serial.println("M2400 S0"); // set mode to default
}

// -------------------------------------------------------------- MAIN LOOP

void loop()
{
   //  Add or remove segments on the clock to get them as close as possible to the correct value of the time
  refreshClock();
} 




