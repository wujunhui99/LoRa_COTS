/*!
 * \file      main.c
 *
 * \brief     FHSS 
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                 _______  _______          
 *                (  ____ \/ ___   )|\     /|
 *                | (    \/\/   )  || )   ( |
 *                | (_____     /   )| |   | |
 *                (_____  )   /   / | |   | |
 *                    ) |  /   /  | |   | |
 *                /\____) | /   (_/\| (___) |
 *                \_______)(_______/(_______)
 *                  (C)2023-2026 SZU
 *
 * \endcode
 *
 * \author    
 */
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
#include <time.h>

#include "sx1272Regs-LoRa.h"
// #include "sx1272Regs-LoRa.h"

#define RX_TIMEOUT_VALUE                            1000
#define APP_DATA_MAX_SIZE           230
static uint8_t AppData[APP_DATA_MAX_SIZE];
#define BASE_FREQ 915000000
// #define BASE_BW 500000
#define FHSS_TOTAL_BW 50000000 // Total hopping bandwidth
#define FHSS_STEP 500000
// #define FHSS_STEP 62500
#define FREQ_NUM (FHSS_TOTAL_BW / FHSS_STEP)
// #define STRAT_FREQ (BASE_FREQ - ((FHSS_TOTAL_BW/FHSS_STEP)/2-1)*FHSS_STEP - FHSS_STEP/2)
#define STRAT_FREQ (BASE_FREQ - FHSS_TOTAL_BW/2 + FHSS_STEP/2)
// #define FREQ_NUM 30

unsigned int Freq = BASE_FREQ;
// unsigned int BW = 2; // BW=250kHz
// unsigned int SF = 7; // SF=9

int step = 125000;
int offset = 0;
bool sig = 1;
uint8_t cnt = 0;

unsigned int FhssFreq[FREQ_NUM];
unsigned int FhssBW[FREQ_NUM];
unsigned int FhssSF[FREQ_NUM];
unsigned int curr_freq_idx = 0;

/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;
    
/*!
 * \brief Function to be executed on Radio Tx Done event
 */
void OnTxDone( void );

/*!
 * \brief Function to be executed on Radio Rx Done event
 */
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );

/*!
 * \brief Function executed on Radio Tx Timeout event
 */
void OnTxTimeout( void );

/*!
 * \brief Function executed on Radio Rx Timeout event
 */
void OnRxTimeout( void );

/*!
 * \brief Function executed on Radio Rx Error event
 */
void OnRxError( void );

/**
  * @brief  The application entry point.
  * @retval int
  */


void OnTxDone( void )
{
    printf("(%d)TX done\n", RtcGetTimerValue());
        
}


// // node state
// // LSB and MSB are implementation-specific
typedef struct sState
{
    uint16_t id; // node id
    uint8_t is_on : 1; // flag: send pkt (is_on == 1) or not (is_on == 0)
    uint8_t sf : 1; // spreading factor [0:SF6, 1:SF7, 2:SF8, 3:SF9, 4:SF10, 5:SF11, 6:SF12]
    uint8_t cr : 1; // coderate [0: 4/5, 1: 4/6, 2: 4/7, 3: 4/8], +1 when passed to SetTxConfig
    uint8_t bw : 2; // bandwidth [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
    uint8_t pw_index; // power index [0:20, 1:16, 2:14, 3:12, 4:10, 5:7, 6:5, 7:2] (dBm)
    uint16_t dc; // tx dutycycle (>= 3s)
    uint8_t freq_index; // freq index: 96 channels
    uint8_t pkt_size; // packet size (>= 8 bytes)
    uint32_t cnt; // packet counter
}sState; // 12 bytes

sState state =
{
    .id = 1,
    .bw = 2,
    .cr = 0,
    .sf = 7,
    .is_on = 1,
    .dc = 5,
    .pw_index = 0,
    .freq_index = 0,
    .pkt_size = 2,
    .cnt = 0
};

uint8_t white[] = {
                0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE1, 0xC2, 0x85, 0x0B, 0x17, 0x2F, 0x5E, 0xBC, 0x78, 0xF1, 0xE3, 
                0xC6, 0x8D, 0x1A, 0x34, 0x68, 0xD0, 0xA0, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x11, 0x23, 0x47, 
                0x8E, 0x1C, 0x38, 0x71, 0xE2, 0xC4, 0x89, 0x12, 0x25, 0x4B, 0x97, 0x2E, 0x5C, 0xB8, 0x70, 0xE0, 
                0xC0, 0x81, 0x03, 0x06, 0x0C, 0x19, 0x32, 0x64, 0xC9, 0x92, 0x24, 0x49, 0x93, 0x26, 0x4D, 0x9B, 
                0x37, 0x6E, 0xDC, 0xB9, 0x72, 0xE4, 0xC8, 0x90, 0x20, 0x41, 0x82, 0x05, 0x0A, 0x15, 0x2B, 0x56, 
                0xAD, 0x5B, 0xB6, 0x6D, 0xDA, 0xB5, 0x6B, 0xD6, 0xAC, 0x59, 0xB2, 0x65, 0xCB, 0x96, 0x2C, 0x58, 
                0xB0, 0x61, 0xC3, 0x87, 0x0F, 0x1F, 0x3E, 0x7D, 0xFB, 0xF6, 0xED, 0xDB, 0xB7, 0x6F, 0xDE, 0xBD, 
                0x7A, 0xF5, 0xEB, 0xD7, 0xAE, 0x5D, 0xBA, 0x74, 0xE8, 0xD1, 0xA2, 0x44, 0x88, 0x10, 0x21, 0x43, 
                0x86, 0x0D, 0x1B, 0x36, 0x6C, 0xD8, 0xB1, 0x63, 0xC7, 0x8F, 0x1E, 0x3C, 0x79, 0xF3, 0xE7, 0xCE, 
                0x9C, 0x39, 0x73, 0xE6, 0xCC, 0x98, 0x31, 0x62, 0xC5, 0x8B, 0x16, 0x2D, 0x5A, 0xB4, 0x69, 0xD2, 
                0xA4, 0x48, 0x91, 0x22, 0x45, 0x8A, 0x14, 0x29, 0x52, 0xA5, 0x4A, 0x95, 0x2A, 0x54, 0xA9, 0x53, 
                0xA7, 0x4E, 0x9D, 0x3B, 0x77, 0xEE, 0xDD, 0xBB, 0x76, 0xEC, 0xD9, 0xB3, 0x67, 0xCF, 0x9E, 0x3D, 
                0x7B, 0xF7, 0xEF, 0xDF, 0xBF, 0x7E, 0xFD, 0xFA, 0xF4, 0xE9, 0xD3, 0xA6, 0x4C, 0x99, 0x33, 0x66, 
                0xCD, 0x9A, 0x35, 0x6A, 0xD4, 0xA8, 0x51, 0xA3, 0x46, 0x8C, 0x18, 0x30, 0x60, 0xC1, 0x83, 0x07, 
                0x0E, 0x1D, 0x3A, 0x75, 0xEA, 0xD5, 0xAA, 0x55, 0xAB, 0x57, 0xAF, 0x5F, 0xBE, 0x7C, 0xF9, 0xF2, 
                0xE5, 0xCA, 0x94, 0x28, 0x50, 0xA1, 0x42, 0x84, 0x09, 0x13, 0x27, 0x4F, 0x9F, 0x3F, 0x7F};
static TimerEvent_t SendDataTimer;
TimerEvent_t fhssTimer;
void OnSendDataTimerEvent(void)
{
    printf("(%d)TimerOn\n", RtcGetTimerValue());
    TimerStop(&SendDataTimer);
    state.is_on = 1;
}
int j = 1;
void fhssTimerEvent(void)
{
    printf("start timer!!\n");
   
}

static void LowPowerHandler(void)
{
    Radio.Sleep();
    //MCU rest
    // TimerLowPowerHandler();
}

static void OnRadioTxDone(void)
{
      
    curr_freq_idx = 0;
    Freq = BASE_FREQ;
    // SF = 7, BW = 0;
    Radio.SetChannel(BASE_FREQ);
    // Radio.SetTxConfig(MODEM_LORA, 10, 0, state.bw,
    //                             state.sf + 6, state.cr + 1,8, 1,
    //                     0, 1, 1, false, 10000);

    // offset = 0;
    printf("(%d)TxDone\n", RtcGetTimerValue());
    // LowPowerHandler();
    // TimerStart(&SendDataTimer);
}

static void OnRadioRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
   
 }



static void OnRadioRxError(void)
{
    // Radio.Sleep();
}

static void OnRadioTxTimeout(void)
{
    // printf("Tx timeout\n");
    // Radio.Sleep();  
}

static void OnRadioRxTimeout(void)
{
    // Radio.Sleep();
}


static void OnFhssChangeChannel( uint8_t currentChannel )
{
    Freq = FhssFreq[curr_freq_idx];
    curr_freq_idx = (curr_freq_idx + 1) % FREQ_NUM;
    // printf("curr_freq_idx %d, fhss channel %d \n",curr_freq_idx, Freq);
    Radio.SetChannel(Freq);

    // // SF and BW are changed
    // Freq = FhssFreq[curr_freq_idx];
    // BW = FhssBW[curr_freq_idx];
    // SF = FhssSF[curr_freq_idx];
    // curr_freq_idx = (curr_freq_idx + 1) % FREQ_NUM;
    
    // Radio.SetChannel(Freq);

    // Radio.Write( REG_LR_MODEMCONFIG1,
    //                      ( Radio.Read( REG_LR_MODEMCONFIG1 ) &
    //                        RFLR_MODEMCONFIG1_BW_MASK &
    //                        RFLR_MODEMCONFIG1_CODINGRATE_MASK &
    //                        RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK &
    //                        RFLR_MODEMCONFIG1_RXPAYLOADCRC_MASK &
    //                        RFLR_MODEMCONFIG1_LOWDATARATEOPTIMIZE_MASK ) |
    //                        ( BW << 6 ) | ( (state.cr + 1) << 3 ) |
    //                        ( 1 << 2 ) | ( 0 << 1 ) |
    //                        (0x00) );

    // Radio.Write( REG_LR_MODEMCONFIG2,
    //                     ( Radio.Read( REG_LR_MODEMCONFIG2 ) &
    //                       RFLR_MODEMCONFIG2_SF_MASK ) |
    //                       ( SF << 4 ) );
    // Radio.Write( REG_LR_DETECTOPTIMIZE,
    //                          ( Radio.Read( REG_LR_DETECTOPTIMIZE ) &
    //                          RFLR_DETECTIONOPTIMIZE_MASK ) |
    //                          RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12 );
    // Radio.Write( REG_LR_DETECTIONTHRESHOLD,
    //                 RFLR_DETECTIONTHRESH_SF7_TO_SF12 );

    // printf("curr_freq_idx %d, fhss channel %d, fhss BW %d , fhss SF %d\n",curr_freq_idx, Freq, BW, SF);

}

static void PreparePacket(void)
{
    // static int32_t latitude = 0, longitude = 0;
    // static uint16_t temperature = 0, humidity = 0;
    // memcpy(AppData, &state, 12);      // store node id in AppData
    // GpsGetLatestGpsPositionBinary(&latitude, &longitude);
    // // SHT2xGetTempHumi(&temperature, &humidity);

    // AppData[12] = latitude & 0xFF;
    // AppData[13] = ( latitude >> 8 ) & 0xFF;
    // AppData[14] = ( latitude >> 16 ) & 0xFF;
    // AppData[15] = longitude & 0xFF;
    // AppData[16] = ( longitude >> 8 ) & 0xFF;
    // AppData[17] = ( longitude >> 16 ) & 0xFF;
    // AppData[18] = temperature & 0xFF;
    // AppData[19] = ( temperature >> 8 ) & 0xFF;
    // AppData[20] = humidity & 0xFF;
    // AppData[21] = ( humidity >> 8 ) & 0xFF;
}

static void InitRadio(void)
{
    // Initialize Radio driver
    RadioEvents.TxDone = OnRadioTxDone;
    RadioEvents.RxDone = OnRadioRxDone;
    RadioEvents.RxError = OnRadioRxError;
    RadioEvents.TxTimeout = OnRadioTxTimeout;
    RadioEvents.RxTimeout = OnRadioRxTimeout;
    RadioEvents.FhssChangeChannel = OnFhssChangeChannel;
    // RadioEvents.RadioOnDioIrq = RadioOnDioIrq;
    Radio.Init(&RadioEvents);

    // Random seed initialization
    srand1(Radio.Random());

    // bool PublicNetwork = true;
    // Radio.SetPublicNetwork(PublicNetwork);
    Radio.Sleep( );
}

// /**
//  * Main application entry point.
//  */

static void SetFhssFreq() {
    // unsigned int start_freq = BASE_FREQ - 750000;
    // printf("STRAT_FREQ is %d\n", STRAT_FREQ);
    for (int i = 0; i < FREQ_NUM; i++) {
        FhssFreq[i] = STRAT_FREQ + i * FHSS_STEP;
        // printf("the freq is %d\n",FhssFreq[i]);
        // printf("FhssFreq[%d]=%d, ", i, FhssFreq[i]);
    }

    // // SF7 and SF9 alternation 
    // unsigned int freq_len = floor(FREQ_NUM / 3);
    // for (int i = 0; i < 2*freq_len; i++) {
    //     FhssBW[i] = 0;
    //     FhssSF[i] = 7;
    //     if (i % 2 == 0) {
    //         FhssFreq[i] = BASE_FREQ - BASE_BW/2;
    //     } else {
    //         FhssFreq[i] = BASE_FREQ + BASE_BW/2;
    //     }
        
    // }
    // for (int i = 2*freq_len; i < 3*freq_len; i++) {
    //     FhssFreq[i] = BASE_FREQ;
    //     FhssBW[i] = 1;
    //     FhssSF[i] = 9;
    // }
}



int main(void)
{
    BoardInitMcu();
    BoardInitPeriph();
    SetFhssFreq();

    InitRadio();
   
    printf("FHSS Test  \n");

    // OnRadioTxDone();

    // state.is_on = 1;

    
    // InitPktTimer();

    Radio.SetChannel(BASE_FREQ); // +17.5kHz
    curr_freq_idx = 0;
    
    // Problem: Only SF7BW125k for preamble can be send when frequency hopping is on
    Radio.SetTxConfig(MODEM_LORA, 15, 0, state.bw,
                                state.sf + 6, state.cr + 1,8, 1,
                        0, 1, 1, false, 10000);
   
    printf("the sf is %d\n",state.sf+6);
    printf("the bw is %d\n",state.bw);

    while (1)
    {
        Radio.Send(white    , 200);
        DelayMs(80);
        printf(" send !!!!\n");
       

    }
}