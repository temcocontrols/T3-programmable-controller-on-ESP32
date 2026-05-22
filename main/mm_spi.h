#ifndef MM_SPI_H
#define MM_SPI_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"

//#include "DrvREG32.h"
//#include "SpecialMacro.h"

#include "hy3131_sw.h"



//#include "DrvADC.h"
//#include "SPI_3131_Read.h"
/*----------------------------------------------------------------------------*/
/* DEFINITIONS                                                                */
/*----------------------------------------------------------------------------*/
#define  BIT0 0x01
#define  BIT1 0x02
#define  BIT2 0x04
#define  BIT3 0x08
#define  BIT4 0x10
#define  BIT5 0x20
#define  BIT6 0x40
#define  BIT7 0x80

#define C_func_KEY			(1 << 0)	//PT1.1
#define C_range_KEY         (1 << 1)	//PT1.0
#define C_Cal_KEY			0xFF
//FunCode
#define  E_DCVolt  0
#define  E_ACVolt  1
#define  E_DCmVolt 2
#define  E_ACmVolt 3
#define  E_OHMCV   4
#define  E_OHMCC   5
#define  E_OHM     6
#define  E_DCuA    7
#define  E_DCmA    8
#define  E_DCA     9
#define  E_ACuA    10
#define  E_ACmA    11
#define  E_ACA     12
#define  E_Temp    13
#define  E_CapCV   14
#define  E_CapCC   15
#define  E_Freq    16
#define  E_Diode   17
#define  E_Continuity 18
#define  E_CAP     19

//NowFunc_temp
#define  Func_DCV	0
#define  Func_ACV	1
#define  Func_DCmV  2
#define  Func_ACmV  3
#define  Func_OHM   4
#define  Func_uA    5	//Func_DCuA,Func_ACuA
#define  Func_mA    6	//Func_DCmA,Func_ACmA
#define  Func_A     7	//Func_DCA,Func_ACA
#define  Func_Hz    8
#define  Func_Cap   9
#define  Func_C_D   10

//Flash Data Calibration parameters
#define	Cal_DC60mV	0xAFC00	
#define	Cal_DC600mV	0xAFC04
#define	Cal_DC6V	0xAFC08
#define	Cal_DC60V	0xAFC0C
#define	Cal_DC600V	0xAFC10
#define	Cal_DC1000V	0xAFC14
#define	Cal_AC60mV	0xAFC18
#define	Cal_AC600mV	0xAFC1C
#define	Cal_AC6V	0xAFC20
#define	Cal_AC60V	0xAFC24
#define	Cal_AC600V	0xAFC28
#define	Cal_AC1000V	0xAFC2C
#define	cal_60R		0xAFC30
#define	cal_600R	0xAFC34
#define	cal_6KR		0xAFC38
#define	cal_60KR	0xAFC3C
#define	cal_600KR	0xAFC40
#define	cal_6MR		0xAFC44
#define	cal_60MR	0xAFC48
#define	cal_DC60uA	0xAFC4C
#define	cal_DC600uA	0xAFC50
#define	cal_DC60mA	0xAFC54
#define	cal_DC600mA	0xAFC58
#define	cal_DC6A	0xAFC5C
#define	cal_DC10A	0xAFC60
#define	cal_AC60uA	0xAFC64
#define	cal_AC600uA	0xAFC68
#define	cal_AC60mA	0xAFC6C
#define	cal_AC600mA	0xAFC70
#define	cal_AC6A	0xAFC74
#define	cal_AC10A	0xAFC78
#define cal_60nF	0xAFC7C
#define cal_600nF	0xAFC80
#define cal_6uF		0xAFC84
#define cal_60uF	0xAFC88
#define cal_600uF	0xAFC8C
#define cal_6mF		0xAFC90

// > 6000,閸氭垳绗傞幓娑欑崲 6000
#define	C_upLimit	6000
// < 570, 閸氭垳绗呯捄铏崲 6000*0.095=570
#define C_downLimit	570

#define C_beepon_OHM 500	//50 ohm
#define C_beepOff_OHM 1100	//110 ohm


// TMA : 31.25ms
#define C_125ms	4
#define C_250ms	8
#define C_500ms	16
#define	C_1S	32 //1000ms / 31.25ms = 32
#define	Cap_1S	5 
#define C_2S	64 
#define C_3S	128 
/*Software-SPI Define*********************************************/
#define  HY3131_PORT E_PT2
#define  HY3131_CS   BIT0
#define  HY3131_SCLK BIT1
#define  HY3131_MISO BIT2
#define  HY3131_MOSI BIT3
#define  SCS_PIN  0
#define  SCLK_PIN 1
#define  MISO_PIN 2
#define  MOSI_PIN 3
/****************************************************************/
/*Hardware-SPI Define********************************************/
//#define  Flash_PORT E_PT2
//#define  Flash_CS   BIT0
//#define  Flash_SCLK BIT1
//#define  Flash_MISO BIT2
//#define  Flash_MOSI BIT3
//#define  SCS_PIN 0

//#define SPI_4WIRE
#define SPI_3WIRE
/****************************************************************/
//#define  UART_PORT E_PT9
//#define  UART_TXD  BIT4
//#define  UART_RXD  BIT5
//#define  UartBufferSize 128

#define KEY_PORT E_PT1
#define KEYIN0 BIT0		//PT1.0
#define KEYIN1 BIT1		//PT1.1
#define KEYIN2 BIT2		//PT1.2
#define KEYIN3 BIT3		//PT1.3
#define KEYIN0_PIN 0
#define KEYIN1_PIN 1
#define KEYIN2_PIN 2

#define  Func_PORT	E_PT3
#define	 Func_P4	BIT5
#define	 Func_P3	BIT4
#define	 Func_P2	BIT3
#define	 Func_P1	BIT2

#define  AD1
//#define  AD2
//#define  LPF

//#define  AC
#define  VA //Apparent Power

#define  Gain1
//#define  Gain10

#define  PEAKMAX
//#define  PEAKMIN

//#define  CTA_Initial 0xE000
#define  CTA_Initial_Low 0xC000	// <1M	
#define  CTA_Initial 0xF000	// >1M		
#define  CTA_Preset_Low  0x1000000-(CTA_Initial_Low<<8)
#define  CTA_Preset  0x1000000-(CTA_Initial<<8)
#define  Period
//#define  Duty

//#define  FIN_ACPO
#define  FIN_CNTI

//#define  CC_MODE
#define  CV_MODE

// HY3131 interrupt source bit masks (参考HY3131手册)
#define DCIRQ   0x04    // AD1 interrupt
#define ACIRQ   0x02    // AD2 interrupt
#define OHMIRQ  0x08    // LPF interrupt
#define FreqIRQ 0x01    // CT interrupt

/*----------------------------------------------------------------------------*/
/* STRUCTURES

//----------------------------------------------------------------------------*/
// typedef   signed          char int8_t;
// typedef   signed short     int int16_t;
// typedef   signed           int int32_t;
// typedef unsigned          char uint8_t;
// typedef unsigned short     int uint16_t;
// typedef unsigned           int uint32_t;

typedef union _MCUSTATUS
{
  char  _byte;
  struct
  {
    unsigned b_ADCdone:1;
    unsigned b_TMAdone:1;
    unsigned b_TMBdone:1;
    unsigned b_TMCdone:1;
    unsigned b_RTCdone:1;
    unsigned b_UART_TxDone:1;
    unsigned b_UART_RxDone:1;
    unsigned b_UartConn:1;
  };
} MCUSTATUS;

typedef union _DMMSTATUS
{
  char  _byte;
  struct
  {
    unsigned b_RangeChange:1;
    unsigned b_FunChange:1;
	unsigned b_ScanTime:1;
	unsigned b_Cal_Mode:1;
	unsigned b_Cal_STATUS:1;
    unsigned RESV:3;
  };
} DMMSTATUS;

typedef struct
{
  	unsigned char b_longKeyEn:1;
  	unsigned char b_longKey:1;
  	unsigned char b_AutoEn:1;
  	unsigned char RESV1:5;

} KEY_STATUS; 

typedef struct
{
	unsigned char b_tick:1;
	unsigned char b_tma1SArrive:1;
    unsigned char b_delayClock:1;
	unsigned char b_freqTimeOut:1;
	unsigned char RESV2:4;	
} TIME_BASE; 

typedef union
{
	long adcVal;
	struct
	{
		char adcL;
		char adcH;
		char adcU;
		char adcUU;
	};
    struct
    {
        unsigned char rmsL;
        unsigned char rmsH;
        unsigned char rmsU;
        unsigned char rmsUU;
    };
}TYPE_ADC_DATA;



volatile typedef union
{
	uint8_t u8_flag[5];
	struct
	{
		//u8_flag[0]
		uint8_t b_minus : 1;
		uint8_t b_Auto_Status : 1;
		uint8_t b_beep_on : 1;
		uint8_t b_Freqlarge1M : 1;
		uint8_t b_over2Sec_Freq : 1;
		uint8_t b_ol : 1;
		uint8_t b_125mS : 1;
		uint8_t b_tick : 1;

		//u8_flag[1]
		uint8_t b_adcOk : 1;
		uint8_t b_125ms_tick : 1;
		uint8_t b_calMode : 1;
		uint8_t b_get600mVOffset : 1;
		uint8_t b_ad1Ok : 1;
		uint8_t b_adcRefreshEn : 1;
		uint8_t b_IC_RESET : 1;

		//u8_flag[2]
		uint8_t b_LCD_busy : 1;
		uint8_t b_beepOff_On : 1;
		uint8_t b_scanKeyEn : 1;
		uint8_t b_Funupdate : 1;
		uint8_t b_adc1ok : 1;
		uint8_t b_unStable_state : 1;
		uint8_t b_Cap_ADC_En : 1;

		//u8_flag[3]
		uint8_t b_chkAD1 	: 1;
		uint8_t b_highcapOk : 1;
		uint8_t b_clock 	: 1;
		uint8_t b_cal2Normal : 1; //閺嶁剝婧�濡�崇础, ??濮濓絽鐖�?闁插繑膩瀵�?
		uint8_t b_flEn 		: 1;
		uint8_t b_longKey 	: 1;
		uint8_t b_lvdH 		: 1;
		uint8_t b_Emty 		: 1;

		//u8_flag[4]
		uint8_t b_lvdL 					: 1;
		uint8_t b_cont50ohm 			: 1;
		uint8_t b_autoRange 			: 1;
		uint8_t b_showCountEn 			: 1;
		uint8_t b_powerOnKey 			: 1;
		uint8_t b_autoChk_res_dcv_acv 	: 2;
		uint8_t b_resResumeLastRangeEn 	: 1;

		//u8_flag[5]
		uint8_t b_dcvResumeLastRangeEn 	: 1;
		uint8_t b_dispAutoEn : 1;
		uint8_t b_freqTimeOut : 1;
		uint8_t b_ohmIgnoreDispCnt : 2;
		uint8_t b_lastFuncNCV : 1;
	};
}TYPE_flag;



// typedef enum
// {
	// A_DC6V_RANGE = 0,
	// M_DC6V_RANGE= 1,
	// M_DC60V_RANGE, 
	// M_DC600V_RANGE,
	// M_DC1000V_RANGE,

	// D_R600_RANGE,
	// D_R6K_RANGE,
	// D_R60K_RANGE,
	// D_R600K_RANGE,
	// D_R6M_RANGE,
	// D_R60M_RANGE,

// }TYPE_RANGE;

// typedef enum
// {
	// A_AC6V_RANGE=0,
	// M_AC6V_RANGE,
	// M_AC60V_RANGE, 
	// M_AC600V_RANGE,
	// M_AC1000V_RANGE,
	
// }TYPE_ACVRANGE;



/*----------------------------------------------------------------------------*/
/* Global CONSTANTS                                                           */
/*----------------------------------------------------------------------------*/

// 鍏ㄥ眬鍙橀噺extern澹版槑
extern MCUSTATUS   MCUSTATUSbits;
extern DMMSTATUS   DMMSTATUSbits;
extern TIME_BASE   TSTATUS;
extern KEY_STATUS  KSTATUS;
extern TYPE_flag   flag;
/*----------------------------------------------------------------------------*/
/* DEFINITIONS                                                                */
/*----------------------------------------------------------------------------*/

// Global variables :
// extern unsigned char DisplayBuffer[18];
// extern unsigned char UartRxBuffer[UartBufferSize]={0};
// extern unsigned char UartTxBuffer[UartBufferSize]={0};

//unsigned char UartTxIndex;
//unsigned char UartTxLength;
//unsigned char UartRxIndex;
//unsigned char UartRxLength;
// unsigned char Uart2TxIndex;
// unsigned char Uart2TxLength;
// unsigned char Uart2RxIndex;
// unsigned char Uart2RxLength;

extern unsigned int TimerCount0;
extern unsigned int settling_time;
extern unsigned int TimerCount;
extern unsigned int TB_31P25ms;
extern unsigned int Timer1sCnt;
extern unsigned int Timer2sCnt;
extern unsigned int T1s_Cap_Cnt;

extern unsigned int TimerBCount;
extern unsigned char dummyread;
//unsigned int rxIntHappened0;

extern TYPE_ADC_DATA i32_ad1Out;
extern TYPE_ADC_DATA i32_rmsVal;

extern long i32_temp;
extern long i32_temp1;
extern long i32_temp2;
extern long i32_temp3;
extern long i32_temp4;
extern long i32_temp5;
extern long iAvgAD1;
extern long i32_b_Hz;
extern long i32_b_Volt;
extern long i32_Last_Hz1;
extern long i32_Hzavg;
extern int iAvgRMS;
extern int Last_Hz[8];
extern int temp_Hz_Cnt;
extern int Hz_ZERO_Cnt;
extern int CAP_discharge_Cnt;
extern int CAP_charge_Cnt;
extern int temp_flag1;
extern uint32_t temp_Cnt;
extern uint32_t Volt_6VOffset;
extern uint32_t Volt_AC6VOffset;
extern uint32_t temp_Cnt0;
extern long temp_sum0;
extern long temp_sum1;
extern int8_t Bar_NumBuff;
extern int8_t Bar_NumBuff1;
extern int8_t MINUS;
extern int8_t key_temp;
extern int8_t last_keyTemp;
extern int8_t i32_key_temp;
extern int8_t pressKey;
extern int8_t releaseKey;
extern int8_t longKey;
extern uint8_t u8_cnt;
extern uint8_t u8_keyCnt;
extern uint32_t getkey_temp;
extern int8_t RangeCode1;
extern int8_t RangeCode;
extern int8_t FunCode;
extern int8_t Func_temp;
extern int8_t NowFunc_temp;
extern int8_t LastFunc_temp;
extern uint8_t u8_FuncCnt;
extern long RMSBUFF05;
extern signed long i16_mainVal;
extern unsigned int AD1Data_000;
extern uint8_t SPI_Data[64];
extern uint8_t SPI_DataOut[3];
extern int32_t ADCData;
extern long Cal_AD1_temp;
extern int cal_stepNum;
extern int32_t AD1DataBuffer;
extern int32_t AD2DataBuffer;
extern int32_t AD3DataBuffer;
extern int32_t LPF2DataBuffer;
extern int32_t LPF3DataBuffer;
extern int32_t RMS2DataBufferLo;
extern int32_t RMS2DataBufferHi;
extern int32_t RMS3DataBufferLo;
extern int32_t RMS3DataBufferHi;
extern int32_t PeakMinDataBuffer;
extern int32_t PeakMaxDataBuffer;
extern long RMS2DataBuffer;
extern double Count_A;
extern double Count_B;
extern double Count_C;
extern int8_t R20Buffer;
extern int8_t R29Buffer;
extern int8_t R34Buffer;
extern int8_t INTFBuffer;
extern int32_t FreqBuff2;
extern double FreqBuffer;
extern double DutyBuffer;
extern double PeriodBuffer;
extern double PeriodBuffer1;
extern double PeriodBuffer2;
extern double FreqBuff1;
extern int32_t DutyBuffer1;
extern double TotalTime;
extern uint8_t BeepSTATUS;
extern uint8_t A14Buffer;
//-----------------------------------------------------------------------
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
// Function PROTOTYPES
//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void System_Initial(void);
void InitalUART(void);
void InitalSPI(void);
void InitalTimerA(void);
void InitalTimerB(void);
void InitalADC(void);
void SwitchDMMFun(void);
void Delay (unsigned int num);

unsigned char getKey(void);
void scankey(void);
void keyInit(void);

extern void dropADC(void);


// Function to initialize the SPI for HY3131
void hy3131_spi_init(void);

// Task to handle communication with HY3131
void hy3131_task(void *pvParameters);

#define PIN_NUM_MISO 12
#define PIN_NUM_MOSI 13
#define PIN_NUM_CLK 14
#define PIN_NUM_CS 15

#endif // MM_SPI_H
