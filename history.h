#ifndef HISTORY_H
#define HISTORY_H

#define H_NUMBER 11
#define H_LENGTH 256

struct inputHistory
{
  int currentcmd;
  int lastusedcmd;
  int len;
  char history[H_NUMBER][H_LENGTH];
};


#endif