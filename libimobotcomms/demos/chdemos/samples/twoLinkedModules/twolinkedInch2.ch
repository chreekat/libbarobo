/* Filename: twoLinkedInch2.ch
   Control two modules and make them stand simultaneously.
   The joint4 of the first robot should be connected to the joint1 
   of the second robot. 
           1st                         2nd
  |---------|--------|        |---------|--------|     
 1|    2     |   3   | 4 <=> 1|    2    |   3    | 4
  |---------|--------|        |---------|--------|   
   */

#include <mobot.h>
int i;
CMobot robot1;
CMobot robot2;

/* Connect robot variables to the robot modules. */
robot1.connect();
robot2.connect();

/* Set the robot to "home" position, where all joint angles are 0 degrees. */

robot1.moveToZeroNB();
robot2.moveToZeroNB();
robot1.moveWait();
robot2.moveWait();

/* stand up */
robot1.moveToNB(0, 90, 90,  0);
robot2.moveToNB(0,  -90,  -90, 0);
robot1.moveWait();
robot2.moveWait();
/* inworm right */
for( i = 0; i < 3; i++){
    robot1.moveTo(0,  30,  90, 0);
    robot2.moveTo(0, -90, -30,  0);
    robot1.moveTo(0,  90,  90, 0);
    robot2.moveTo(0, -90, -90,  0);
}
/* inworm left */
for( i = 0; i < 3; i++){
    robot2.moveTo(0, -90, -30,  0);
    robot1.moveTo(0,  30,  90, 0);
    robot2.moveTo(0, -90, -90,  0);
    robot1.moveTo(0,  90,  90, 0);
}