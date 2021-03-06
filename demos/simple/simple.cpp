/* Filename: simple.cpp
 * Rotate the faceplates by 90 degrees */
#include <mobot.h>
#include <linkbot.h>
#include <stdlib.h>

#define NUMBOTS 3

int main()
{
  int i;
  CLinkbotI mobot[NUMBOTS];
  CLinkbotIGroup group;
  for(i = 0; i < NUMBOTS; i++) {
    mobot[i].connect();
    group.addRobot(mobot[i]);
  }
  group.move(45, 0, 45);

  return 0;
}
