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
#define BASE_FREQ 950000000
#define FHSS_TOTAL_BW 40000000 // Total hopping bandwidth
#define FHSS_STEP 500000
#define FREQ_NUM (FHSS_TOTAL_BW / FHSS_STEP)
// #define STRAT_FREQ (BASE_FREQ - ((FHSS_TOTAL_BW/FHSS_STEP)/2-1)*FHSS_STEP - FHSS_STEP/2)
#define STRAT_FREQ (BASE_FREQ - FHSS_TOTAL_BW/2 + FHSS_STEP/2)
#define END_FREQ (BASE_FREQ + FHSS_TOTAL_BW/2 - FHSS_STEP/2)

unsigned int Freq = BASE_FREQ;

int step = 125000;
int offset = 0;
bool sig = 1;
uint8_t cnt = 0;

unsigned int FhssFreq[FREQ_NUM];
unsigned int FhssBW[FREQ_NUM];
unsigned int FhssSF[FREQ_NUM];
unsigned int curr_freq_idx = 0;

typedef enum
{
    LOWPOWER,
    RX,
    RX_TIMEOUT,
    RX_ERROR,
    TX,
    TX_TIMEOUT,
    START_CAD,
}States_t;

// // node tConfig
// // LSB and MSB are implementation-specific
typedef struct RadioConfig
{
    uint16_t id; // node id
    uint8_t is_on; // flag: send pkt (is_on == 1) or not (is_on == 0)
    uint8_t sf; // spreading factor [0:SF6, 1:SF7, 2:SF8, 3:SF9, 4:SF10, 5:SF11, 6:SF12]
    uint8_t cr; // coderate [0: 4/5, 1: 4/6, 2: 4/7, 3: 4/8], +1 when passed to SetTxConfig
    uint8_t bw; // bandwidth [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
    uint16_t dc; // tx dutycycle (>= 3s)
    uint32_t cnt; // packet counter
    int8_t txpower; // Tx Power(dB)
    uint16_t preambleLen; // preamble len
    bool fixLen;   // fixLen=1: no header; fixLen=0: with header
    bool crcOn;   // crc
    bool freqHopOn;  // chirp hopping
    uint8_t hopPeriod; // how many chirps in a hopping channel
    bool iqInverted; // true: downchirp to upchirp
}RadioConfig; // 12 bytes

RadioConfig txconfig =
{
    .id = 1,
    .bw = 2,
    .cr = 0,
    .sf = 7,
    .is_on = 1,
    .dc = 5,
    .cnt = 1,
    .txpower = 25,
    .preambleLen = 8,
    .fixLen = 0,
    .crcOn = 1,
    .freqHopOn = 0,
    .hopPeriod = 1,
    .iqInverted = 0
};

uint8_t data[] = "beacon";

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

static TimerEvent_t SendDataTimer;
TimerEvent_t fhssTimer;
void OnSendDataTimerEvent(void)
{
    printf("(%d)TimerOn\n", RtcGetTimerValue());
    TimerStop(&SendDataTimer);
    txconfig.is_on = 1;
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
    Radio.SetChannel(BASE_FREQ);
    // Radio.SetTxConfig(MODEM_LORA, 10, 0, txconfig.bw,
    //                             txconfig.sf + 6, txconfig.cr + 1,8, 1,
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
    printf("curr_freq_idx %d, fhss channel %d \n",curr_freq_idx, Freq);
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
    //                        ( BW << 6 ) | ( (txconfig.cr + 1) << 3 ) |
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
    // memcpy(AppData, &txconfig, 12);      // store node id in AppData
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
   
    // printf("FHSS Test  \n");

    // OnRadioTxDone();

    // txconfig.is_on = 1;

    
    // InitPktTimer();

    Radio.SetChannel(BASE_FREQ); // +17.5kHz
    curr_freq_idx = 0;
    
    // 
    // Problem: Only SF7BW125k for preamble can be send when frequency hopping is on
    // void    ( *SetTxConfig )( RadioModems_t modem, int8_t power, uint32_t fdev,
    //                 uint32_t bandwidth, uint32_t datarate,
    //                 uint8_t coderate, uint16_t preambleLen,
    //                 bool fixLen, bool crcOn, bool freqHopOn,
    //                 uint8_t hopPeriod, bool iqInverted, uint32_t timeout );
    Radio.SetTxConfig(MODEM_LORA, txconfig.txpower, 0, txconfig.bw,
                                txconfig.sf, txconfig.cr + 1,txconfig.preambleLen, txconfig.fixLen,
                        txconfig.crcOn, txconfig.freqHopOn, txconfig.hopPeriod, false, 10000);
   
    printf("the sf is %d\n",txconfig.sf);
    printf("the bw is %d\n",txconfig.bw);

    for (int i = 0; i < txconfig.cnt; i++)
    {
        Radio.Send(data, sizeof(data));
        printf("send beacon !!!!\n");
    }
}