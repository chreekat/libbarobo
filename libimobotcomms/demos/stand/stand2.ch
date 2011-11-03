#include <mobot.h>

CMobot robot;

/* Connect to the paired MoBot */
robot.connect();

/* Set robot motors to speed of 0.50 */
int i;
for(i = MOBOT_JOINT1; i < MOBOT_NUM_JOINTS; i++) {
  robot.setJointSpeed(i, 0.50);
}
/* Set the robot to "home" position, where all joint angles are 0 degrees. */
robot.moveToZero();
robot.moveWait();

/* Move the robot into a fetal position */
robot.moveJointTo(MOBOT_JOINT2, -85);
robot.moveJointTo(MOBOT_JOINT3, 80);
robot.moveWait();

/* Rotate the bottom faceplate by 45 degrees */
robot.moveJointTo(MOBOT_JOINT4, 45);
robot.moveWait();

/* Lift the body up */
robot.moveJointTo(MOBOT_JOINT2, 20);
robot.moveWait();

/* Pan the robot around for 3 seconds */
robot.setJointSpeed(MOBOT_JOINT4, 0.0);
robot.setJointDirection(MOBOT_JOINT4, MOBOT_JOINT_DIR_FORWARD);
robot.setJointSpeed(MOBOT_JOINT4, 0.30);
sleep(3);
robot.setJointSpeed(MOBOT_JOINT4, 0.0);
