/*
 * mcp3208.c:
 * This file is a fork of the mcp3004 library for WiringPi 
 ***********************************************************************
 */

#include <wiringPi.h>
#include <wiringPiSPI.h>

#include "mcp3208.h"

static int myAnalogRead (struct wiringPiNodeStruct *node, int pin)
{
  unsigned char spiData [3] ;
  unsigned char chanBits ;
  int chan = pin - node->pinBase ;

  chanBits = 0b10000000 | (chan << 4) ;

  spiData [0] = 1 ;		// Start bit
  spiData [1] = chanBits ;
  spiData [2] = 0 ;

  wiringPiSPIDataRW (node->fd, spiData, 3) ;

  return ((spiData [1] << 8) | spiData [2]) & 0xFFF ;
}

int mcp3208Setup (const int pinBase, int spiChannel)
{
  struct wiringPiNodeStruct *node ;

  if (wiringPiSPISetup (spiChannel, 1000000) < 0)
    return -1 ;

  node = wiringPiNewNode (pinBase, 8) ;

  node->fd         = spiChannel ;
  node->analogRead = myAnalogRead ;

  return 0 ;
}
