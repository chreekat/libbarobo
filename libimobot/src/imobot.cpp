#include <stdio.h>
#include "imobot.h"
#include "stdint.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _CH_
#pragma package <chi2cio>
#include <i2c-api.h>
#else
#include "libi2c/i2c-api.h"
#endif

#ifdef __cplusplus
}
#endif

#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

int BR_init(iMobot_t* iMobot)
{
  const char *m_i2cDevName = "/dev/i2c-3";
  int i;
  uint8_t hibyte, lobyte;

  // Try to open the i2c device

  if (( iMobot->i2cDev = open( m_i2cDevName, O_RDWR )) < 0 )
  {
    return( -1 );
  }
  
  /* Get the positions */
  for( i = 0; i < 4; i++) {
    I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
    I2cReadByte(iMobot->i2cDev, I2C_REG_MOTORPOS(i), &hibyte);
    I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
    I2cReadByte(iMobot->i2cDev, I2C_REG_MOTORPOS(i)+1, &lobyte);
    iMobot->enc[i] = (hibyte << 8) + lobyte;
  }
  /* Set default speeds for the motors */
  for(i = 0; i < 4; i++) {
    BR_setMotorSpeed(iMobot, i, 30);
  }
  return 0;
}

int BR_initListenerBluetooth(iMobot_t* iMobot, int channel)
{
  struct sockaddr_rc loc_addr = { 0 };
  int err;

  // allocate socket
  iMobot->socket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
  if (iMobot->socket < 0) {
    fprintf(stderr, "Error allocating socket.\n");
    exit(-1);
  }

  // bind socket to port 1 of the first available 
  // local bluetooth adapter
  loc_addr.rc_family = AF_BLUETOOTH;
  loc_addr.rc_bdaddr.b[0] = 0;
  loc_addr.rc_bdaddr.b[1] = 0;
  loc_addr.rc_bdaddr.b[2] = 0;
  loc_addr.rc_bdaddr.b[3] = 0;
  loc_addr.rc_bdaddr.b[4] = 0;
  loc_addr.rc_bdaddr.b[5] = 0;
  loc_addr.rc_channel = (uint8_t) channel;
  err = bind(iMobot->socket, (struct sockaddr *)&loc_addr, sizeof(loc_addr));
  if(err < 0) {
    fprintf(stderr, "error binding. %s:%d\n", strerror(errno), errno);
    exit(-1);
  }

  // put socket into listening mode
  err = listen(iMobot->socket, 1);
  if(err < 0) {
    fprintf(stderr, "error listen. %s \n", strerror(err));
    exit(-1);
  }

  return 0;
}

int BR_pose(iMobot_t* iMobot, double angles[4], const char motorMask)
{
  /* Send the desired angles to the iMobot */
  /* Need to convert it to 2 bytes and send to motor*/
  int i;
  uint8_t data[2];
  short enc[4];
  for(i = 0; i < 4; i++) {
    /* Internally, the motor angles are stored as shorts, where each unit
     * represents 0.1 degrees. Thus, we must multiply this angle value by ten
     * before sending it to the motor controller. */
    enc[i] = (short)(angles[i]*10);
    if(((1<<i) & motorMask) == 0) {
      continue;
    }
    memcpy(&data, &enc[i], 2);
    /* We should send the high byte first */
    I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
    I2cWriteByte(iMobot->i2cDev, I2C_REG_MOTORPOS(i), data[1]);
    I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
    I2cWriteByte(iMobot->i2cDev, I2C_REG_MOTORPOS(i)+1, data[0]);
    iMobot->enc[i] = enc[i];
  }
  
  return 0;
}

int BR_poseJoint(iMobot_t* iMobot, unsigned short id, double angle)
{
  double _enc[4];
  if(id > 3) {
    return -1;
  }
  _enc[id] = angle;
  return BR_pose(iMobot, _enc, (1<<id));
}

int BR_move(iMobot_t* iMobot, double angles[4], const char motorMask)
{
  int i;
  for(i = 0; i < 4; i++) {
    if(((1<<i) & motorMask) == 0) {
      continue;
    }
    if( BR_moveJoint(iMobot, i, angles[i]) ) {
      return -1;
    }
  }
  return 0;
}

int BR_moveJoint(iMobot_t* iMobot, unsigned short id, double angle)
{
  double cur_angle;
  /* Get the joint's current angle */
  if(BR_getJointAngle(iMobot, id, &cur_angle)) {
    return -1;
  }
  return BR_poseJoint(iMobot, id, cur_angle + angle);
}

int BR_stop(iMobot_t* iMobot)
{ 
  /* Immediately set motor speeds to zero */
  int i;
  I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
  for(i = 0; i < 4; i++) {
    I2cWriteByte(iMobot->i2cDev, I2C_REG_MOTORSPEED(i), 0);
  }
  return 0;
}

int BR_moveWait(iMobot_t* iMobot)
{
  int i;
  for(i = 0; i < 4; i++) {
    BR_waitMotor(iMobot, i);
  }
  return 0;
}

int BR_setMotorDirections(iMobot_t* iMobot, unsigned short dir[4], const char motorMask)
{
  int i;
  uint8_t data[2];
  for(i = 0; i < 4; i++) {
    if(((1<<i) & motorMask) == 0) {
      continue;
    }
    memcpy(&data, &dir[i], 2);
    I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
    I2cWriteByte(iMobot->i2cDev, I2C_REG_MOTORDIR(i), data[0]);
  }
  return 0;
}

int BR_setMotorDirection(iMobot_t* iMobot, int id, int direction)
{
  int i;
  uint8_t data[2];
  memcpy(&data, &direction, 2);
  I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
  I2cWriteByte(iMobot->i2cDev, I2C_REG_MOTORDIR(id), data[0]);
  return 0;
}

int BR_getMotorDirection(iMobot_t* iMobot, int id, int *dir)
{
  uint8_t byte;
  I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
  I2cReadByte(iMobot->i2cDev, I2C_REG_MOTORDIR(id), &byte);
  *dir = byte;
  return 0;
}

int BR_setMotorSpeed(iMobot_t* iMobot, int id, int speed)
{
  int i;
  uint8_t data[2];
  memcpy(&data, &speed, 2);
  I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
  I2cWriteByte(iMobot->i2cDev, I2C_REG_MOTORSPEED(id), data[0]);
  return 0;
}

int BR_getMotorSpeed(iMobot_t* iMobot, int id, int *speed)
{
  uint8_t byte;
  I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
  if(I2cReadByte(iMobot->i2cDev, I2C_REG_MOTORSPEED(id), &byte)) {
    return -1;
  } else {
    *speed = byte;
    return 0;
  }
}

int BR_getMotorState(iMobot_t* iMobot, int id, int *state)
{
  uint8_t byte;
  I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
  if(I2cReadByte(iMobot->i2cDev, I2C_REG_MOTORSTATE(id), &byte)) {
    return -1;
  } else {
    *state = byte;
    return 0;
  }
}

int BR_setMotorPosition(iMobot_t* iMobot, int id, double angle)
{
  /* Send the desired angles to the iMobot */
  /* Need to convert it to 2 bytes and send to motor*/
  int i;
  uint8_t data[2];
  /* Multiply by 10 to convert to 10*degrees, which are the units used by the
   * daughterboard. */
  short position = (short) angle*10;
  memcpy(&data, &position, 2);
  /* We should send the high byte first */
  I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
  I2cWriteByte(iMobot->i2cDev, I2C_REG_MOTORPOS(i), data[1]);
  I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
  I2cWriteByte(iMobot->i2cDev, I2C_REG_MOTORPOS(i)+1, data[0]);
  iMobot->enc[id] = position;

  return 0;
}

int BR_getMotorPosition(iMobot_t* iMobot, int id, double *angle)
{
  uint8_t hibyte, lobyte;
  short s_angle;
  I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
  I2cReadByte(iMobot->i2cDev, I2C_REG_MOTORPOS(id), &hibyte);
  I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
  I2cReadByte(iMobot->i2cDev, I2C_REG_MOTORPOS(id)+1, &lobyte);
  s_angle = (hibyte << 8) + lobyte;
  *angle = (double)s_angle/10.0;
  return 0;
}

int BR_isBusy(iMobot_t* iMobot)
{
  int i;
  uint8_t byte;
  I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
  for(i = 0; i < 4; i++) {
    if(I2cReadByte(iMobot->i2cDev, I2C_REG_MOTORSTATE(i), &byte)) {
      return -1;
    } else {
      if(byte) {
        return 1;
      }
    }
  }
  return 0;
}

int BR_getJointAngle(iMobot_t* iMobot, int id,  double *angle)
{
  uint8_t hibyte, lobyte;
  short s_angle;
  I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
  I2cReadByte(iMobot->i2cDev, I2C_REG_MOTORPOS(id), &hibyte);
  I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
  I2cReadByte(iMobot->i2cDev, I2C_REG_MOTORPOS(id)+1, &lobyte);
  s_angle = (hibyte << 8) + lobyte;
  *angle = (double)s_angle/10.0;
  return 0;
}

int BR_getJointAngles(iMobot_t* iMobot, double angle[4])
{
  int i;
  double _angle;
  for(i = 0; i < 4; i++) {
    BR_getJointAngle(iMobot, i, &_angle);
    angle[i] = _angle;
  }
  return 0;
}

int BR_getJointEnc(iMobot_t* iMobot, int id, short* enc)
{
  uint8_t hibyte, lobyte;
  I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
  I2cReadByte(iMobot->i2cDev, I2C_REG_MOTORPOS(id), &hibyte);
  I2cSetSlaveAddress(iMobot->i2cDev, I2C_HC_ADDR, 0);
  I2cReadByte(iMobot->i2cDev, I2C_REG_MOTORPOS(id)+1, &lobyte);
  *enc = (hibyte << 8) + lobyte;
  return 0;
}

int BR_listenerMainLoop(iMobot_t* iMobot)
{
  struct sockaddr_rc rem_addr = { 0 };
  char buf[1024] = { 0 };
  int client, bytes_read, err;
  socklen_t opt = sizeof(rem_addr);
  // accept connections
  while(1) {
    client = accept(iMobot->socket, (struct sockaddr *)&rem_addr, &opt);
    if (client < 0) {
      fprintf(stderr, "Error accepting connection\n");
      exit(-1);
    }

    ba2str( &rem_addr.rc_bdaddr, buf );
    fprintf(stderr, "accepted connection from %s\n", buf);
    memset(buf, 0, sizeof(buf));

    // read data from the client
    err = 0;
    while (err == 0) {
      bytes_read = read(client, buf, sizeof(buf));
      if( bytes_read > 0 ) {
        printf("received [%s]\n", buf);
        err = BR_slaveProcessCommand(iMobot, client, bytes_read, buf);
      } else {
        err = -1;
      }
    }
    if(err == -1) {
      continue;
    }

    // close connection
    close(client);
  }
  close(iMobot->socket);
  return 0;
}

int BR_waitMotor(iMobot_t* iMobot, int id)
{
  int state;
  if(BR_getMotorState(iMobot, id, &state)) {
    return -1;
  }
  while(state != 0) {
    usleep(100000);
    if(BR_getMotorState(iMobot, id, &state)) {
      return -1;
    }
  }
  return 0;
}

int BR_poseZero(iMobot_t* iMobot)
{
  int i;
  for(i = 0; i < 4; i++) {
    if(BR_poseJoint(iMobot, i, 0)) {
      return -1;
    }
  }
  return 0;
}

#define MATCHSTR(str) \
(strncmp(str, buf, strlen(str))==0)
int BR_slaveProcessCommand(iMobot_t* iMobot, int socket, int bytesRead, const char* buf)
{
  /* Possible Commands:
   * DEMO -- Run through a preprogrammed demo routine
   * SET_MOTOR_DIRECTION <id> <uint8_t dir>
   * GET_MOTOR_DIRECTION -- returns uint8_t
   * SET_MOTOR_SPEED    <id> <uint8_t speed>
   * GET_MOTOR_SPEED    <id> -- returns uint8_t
   * SET_MOTOR_POSITION <id> <double>
   * GET_MOTOR_POSITION <id> -- returns double 
   * GET_MOTOR_STATE    <id> -- returns uint8_t
   * STOP -- Stop all motors
   * GET_IMOBOT_STATUS -- Should return the string "IMOBOT READY"
   */
  int id;
  int32_t int32;
  char mybuf[80];
  unsigned short myshort;
  double mydouble;
#if DEBUG
  printf("Slave recv: %s\n", buf);
#endif

  /* **** *
   * DEMO *
   * **** */
  if(MATCHSTR("DEMO")) {
#if 0
    write(socket, "OK", 3);
    for(i = 0; i < 8; i++) {
      demo_move_forward(iMobot);
    }
    for(i = 0; i < 8; i++) {
      demo_move_right(iMobot);
    }
    for(i = 0; i < 8; i++) {
      demo_move_left(iMobot);
    }
    for(i = 0; i < 4; i++) {
      demo_move_backward(iMobot);
    }
    demo_turn_left(iMobot, 8);
    demo_sweep_left(iMobot, 8);
    demo_arch(iMobot);
    demo_sweep_left(iMobot, 8);
    demo_stand(iMobot);
    demo_turn_left(iMobot, 8);
    demo_stand_rotate(iMobot);
    demo_turn_left(iMobot, 8);
#endif
  } else if (MATCHSTR("SET_MOTOR_DIRECTION"))
  {
    sscanf(buf, "%*s %d %d", &id, &int32);
    if(BR_setMotorDirection(iMobot, id, int32)) {
      write(socket, "ERROR", 6);
    } else {
      write(socket, "OK", 6);
    }
  } else if (MATCHSTR("GET_MOTOR_DIRECTION"))
  {
    sscanf(buf, "%*s %d", &id);
    if(BR_getMotorDirection(iMobot, id, &int32)) {
      write(socket, "ERROR", 6);
    } else {
      sprintf(mybuf, "%d", int32);
      write(socket, mybuf, strlen(mybuf)+1);
    }
  } else if (MATCHSTR("SET_MOTOR_SPEED"))
  {
    sscanf(buf, "%*s %d %d", &id, &int32);
    if(BR_setMotorSpeed(iMobot, id, int32)) {
      write(socket, "ERROR", 6);
    } else {
      write(socket, "OK", 3);
    }
  } else if (MATCHSTR("GET_MOTOR_SPEED"))
  {
    sscanf(buf, "%*s %d", &id);
    if(BR_getMotorSpeed(iMobot, id, &int32)) {
      write(socket, "ERROR", 6);
    } else {
      sprintf(mybuf, "%d", int32);
      write(socket, mybuf, strlen(mybuf)+1);
    }
  } else if (MATCHSTR("SET_MOTOR_POSITION"))
  {
    sscanf(buf, "%*s %d %lf", &id, &mydouble);
    if(BR_poseJoint(iMobot, id, mydouble)) {
      write(socket, "ERROR", 6);
    } else {
      write(socket, "OK", 3);
    }
  } else if (MATCHSTR("GET_MOTOR_POSITION"))
  {
    sscanf(buf, "%*s %d", &id);
    if(BR_getMotorPosition(iMobot, id, &mydouble)) {
      write(socket, "ERROR", 6);
    } else {
      sprintf(mybuf, "%lf", mydouble);
      write(socket, mybuf, strlen(mybuf)+1);
    }
  } else if (MATCHSTR("STOP")) 
  {
    for(id = 0; id < 4; id++) {
      BR_setMotorSpeed(iMobot, id, 0);
    }
    write(socket, "OK", 3);
  }  else if (MATCHSTR("GET_MOTOR_STATE")) 
  {
    sscanf(buf, "%*s %d", &id);
    if(BR_getMotorState(iMobot, id, &int32)) {
      write(socket, "ERROR", 6);
    } else {
      sprintf(mybuf, "%d", int32);
      write(socket, mybuf, strlen(mybuf)+1);
    }
  } else if (MATCHSTR("GET_IMOBOT_STATUS"))
  {
    write(socket, "IMOBOT READY", strlen("IMOBOT READY") + 1);
  } else {
    fprintf(stderr, "Received unknown command from master: %s\n", buf);
    write(socket, "ERROR", 6);
  }
  return 0;
}
#undef MATCHSTR

int BR_terminate(iMobot_t* iMobot)
{
  return close(iMobot->i2cDev);
}

/* C++ class function wrappers */
CiMobot::CiMobot() {
  BR_init(&_iMobot);
}

CiMobot::~CiMobot() {
}

int CiMobot::initListenerBluetooth(int channel)
{
  return BR_initListenerBluetooth(&_iMobot, channel);
}

int CiMobot::poseJoint(int id, double angle)
{
  return BR_poseJoint(&_iMobot, id, angle);
}

int CiMobot::moveJoint(int id, double angle)
{
  return BR_moveJoint(&_iMobot, id, angle);
}

int CiMobot::stop()
{
  return BR_stop(&_iMobot);
}

int CiMobot::setMotorDirection(int id, int direction)
{
  return BR_setMotorDirection(&_iMobot, id, direction);
}

int CiMobot::moveWait()
{
  return BR_moveWait(&_iMobot);
}

int CiMobot::isBusy()
{
  return BR_isBusy(&_iMobot);
}

int CiMobot::getJointAngle(int id, double &angle)
{
  return BR_getJointAngle(&_iMobot, id, &angle);
}

int CiMobot::getJointAngles(double angle[4])
{
  return BR_getJointAngles(&_iMobot, angle);
}

int CiMobot::setMotorPosition(int id, double angle)
{
  return BR_setMotorPosition(&_iMobot, id, angle);
}

int CiMobot::getMotorPosition(int id, double &angle)
{
  return BR_getMotorPosition(&_iMobot, id, &angle);
}

int CiMobot::setMotorSpeed(int id, int speed)
{
  return BR_setMotorSpeed(&_iMobot, id, speed);
}

int CiMobot::getMotorSpeed(int id, int &speed)
{
  return BR_getMotorSpeed(&_iMobot, id, &speed);
}

int CiMobot::getMotorState(int id, int &state)
{
  return BR_getMotorState(&_iMobot, id, &state);
}

int CiMobot::waitMotor(int id)
{
  return BR_waitMotor(&_iMobot, id);
}

int CiMobot::poseZero()
{
  return BR_poseZero(&_iMobot);
}

int CiMobot::listenerMainLoop()
{
  return BR_listenerMainLoop(&_iMobot);
}

int CiMobot::terminate()
{
  return BR_terminate(&_iMobot);
}
