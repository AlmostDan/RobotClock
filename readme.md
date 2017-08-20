## Introduction

This code impletens a digital minute clock performed by uArm Pro with suction cup extension, Arduino and couple of paper blocks with a storage containers.

It is based on code for uArm Metal by Alex Thiel. For the original code and image instructions see https://forum.ufactory.cc/t/tutorial-for-beginners-and-intermediate-make-your-own-uarm-clockbot/126. Note that the instruction slightly differ for this version, see below.

The code is fairly general and easily modifiable for different dimensions, digits, etc.

## Hardware setup

1) Equip uArm pro with smaller version of the suction cup. The air tube connection is to be pointed towards left side of the arm, when viewed from the front. It is the same side as where the box is positioned and it is important, because it limits the arm's wrist rotation and it would not work otherwise. 
2) Connect Arduino via serial connection to the uArm. This process is the same as connection of OpenMV cam described in the uArm Pro user guide. To rephrase it: Between Arduino and uArm, connect 5V, GND; TX to RX and vice versa by cables. Note that it blocks the Arduino USB connection, so it needs to be disconnected in order to reflash the Arduino. The uArm, by default, does not use this connection in favour for the USB serial. This needs to be switched. Send command "M2500" to the uArm via the USB. This can be done from the Arduino Serial monitor from a computer. The clock code waits for this connection to activate and then it starts to operate.

## Paper blocks

The clock uses paper blocks to write the digits and a box to store and "calibrate" the blocks, which get to move out of position over time.
The papers are 60x20mm. Different size can be used, just change size defines in the code. But the original size, 75x25mm is too large for the Pro and digit ends are out of reach. If the papers "stick" together when picked up by the arm, it is because the sucked air flows through the paper. Put pices of tape on the papers to make them air proof. 
For the box, see the original instructions by Alex for the basic idea. I recommend a bit different architecture. Base of same size as the papers, 60x20, and make it wider towards the top, cca 65x25. That way the papers easily fall in and arrange themselves in place.
For the box placement, just see where the arm reaches at first and position the box middle under it. It should be vertical and parallel to the robot central axis.

## Code setup

The only constant that needs to be provided is the TIME, set it to the minute when the program is activated in order to show real time.