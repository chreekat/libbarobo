/* Discriptioin:
 * In this scene, two linked mobot push the fourWheelDrive.
 *           1st                         2nd
 *   |---------|--------|       |---------|--------|     
 * 1 |    2    |   3    | 4 X 1 |    2    |   3    | 4
 *   |---------|--------|       |---------|--------| 
 *The mobot3 and mobot4 are assembled as a fourWheelDrive.   
 */
 
#include <mobot.h>
int i;
CMobot mobot1;
CMobot mobot2;
CMobot mobot3;
CMobot mobot4;

/* Connect mobot variables to the mobot modules. */
mobot1.connect();
mobot2.connect();
mobot3.connect();
mobot4.connect();

/* Set the mobot to "home" position, where all joint angles are 0 degrees. */
mobot1.moveToZeroNB();
mobot2.moveToZeroNB();
mobot3.moveToZeroNB();
mobot4.moveToZeroNB();
mobot1.moveWait();
mobot2.moveWait();
mobot3.moveWait();
mobot4.moveWait();

mobot3.setJointSpeedRatios(0.05, 0.05, 0.05, 0.05);
mobot4.setJointSpeedRatios(0.05, 0.05, 0.05, 0.05);
/* stand up */
mobot1.moveToNB(0, 90, 90,  0);
mobot2.moveToNB(0,  -90,  -90, 0);
mobot1.moveWait();
mobot2.moveWait();

// the fourWheelDrive roll backward
mobot3.motionRollBackwardNB(360);
mobot4.motionRollBackwardNB(360);

// the twoLinked one inworm right
for( i = 0; i < 10; i++){
    mobot2.moveTo(0, -90, -30,  0);
    mobot1.moveTo(0,  30,  90, 0);
    mobot2.moveTo(0, -90, -90,  0);
    mobot1.moveTo(0,  90,  90, 0);
}
mobot3.motionWait();
mobot4.motionWait();
// inworm left 
/*
for( i = 0; i < 3; i++){
    mobot1.moveTo(0,  30,  90, 0);
    mobot2.moveTo(0, -90, -30,  0);
    mobot1.moveTo(0,  90,  90, 0);
    mobot2.moveTo(0, -90, -90,  0);
}
*/
