#pragma once

#include "../global/global.h"

#define WIRE3_DATA_DIR BIT30
#define WIRE3_CLK_DIR BIT31

#define WIRE3_DATA BIT22
#define WIRE3_CLK BIT23

#define WIRE3_CLK_CNT 2

const static unsigned char DAC_SCALER_ADR = 0x00;

static unsigned char bit_cnt;

typedef struct pga PGA;

void pga_set (unsigned char, unsigned int, unsigned int);

void wire3_init (void);
void wire3_write (unsigned char);
void wire3_handler (void);
void wire3_end_write (void);

