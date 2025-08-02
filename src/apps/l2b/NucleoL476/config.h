#include "board.h"
#include "delay.h"
#include "gpio.h"
#include "gps.h"
#include "utilities.h"
#include "rtc-board.h"
#include "radio.h"
#include "timer.h"
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef CONFIG_H
#define CONFIG_H

#define BASE_FREQ 868000000
unsigned int Freq = BASE_FREQ;
int step = 125000;
int offset = 0;
bool sig = 1;
uint8_t cnt = 0;

uint8_t sf = 10;
uint8_t pw_index = 0;
uint8_t bw = 0;
uint8_t cr = 0;
int seq1[20] = {3, -1, 9, -9, -6, -14, -9, 0, -12, 14, 8, 0, -4, -6, 10, -15, -14, 6, 8, -12};
int seq2[20] = {14, -7, -6, 7, 2, -12, -11, 0, -5, -8, 0, 9, 4, 6, -7, 13, 11, 14, -7, -1};
int seq3[20] = {15, 5, 6, 5, -8, -16, -7, -16, 12, -5, 7, 11, 12, -6, 13, 3, -5, 12, -7, 14};
#endif // CONFIG_H
