#include "HY3131_SW.h"
#include "stdint.h"

// Global variables :
// extern unsigned char DisplayBuffer[18];
// extern unsigned char UartRxBuffer[UartBufferSize]={0};
// extern unsigned char UartTxBuffer[UartBufferSize]={0};

unsigned char UartTxIndex;
unsigned char UartTxLength;
unsigned char UartRxIndex;
unsigned char UartRxLength;
// unsigned char Uart2TxIndex;
// unsigned char Uart2TxLength;
// unsigned char Uart2RxIndex;
// unsigned char Uart2RxLength;


unsigned int TimerCount0; 
unsigned int settling_time; 
unsigned int TimerCount; 
unsigned int TB_31P25ms;
unsigned int Timer1sCnt;
unsigned int Timer2sCnt;
unsigned int T1s_Cap_Cnt;

unsigned int TimerBCount;
unsigned char dummyread;
 //unsigned int rxIntHappened0;

TYPE_ADC_DATA i32_ad1Out;
TYPE_ADC_DATA i32_rmsVal;

long	i32_temp; 
long	i32_temp1;  
long	i32_temp2;
long	i32_temp3;
long	i32_temp4;
long	i32_temp5;
long	iAvgAD1;
long	i32_b_Hz;
long	i32_b_Volt;
long	i32_Last_Hz1;
long	i32_Hzavg;
int 	iAvgRMS;
int		Last_Hz[8];
int		temp_Hz_Cnt;
int 	Hz_ZERO_Cnt;
int     CAP_discharge_Cnt;
int     CAP_charge_Cnt;

int		temp_flag1;


uint32_t  temp_Cnt;
uint32_t  Volt_6VOffset;
uint32_t  Volt_AC6VOffset;
uint32_t  temp_Cnt0;
long  	  temp_sum0;
long  	  temp_sum1;
int8_t    Bar_NumBuff;
int8_t    Bar_NumBuff1;

int8_t MINUS;
 
int8_t key_temp;
int8_t last_keyTemp;
int8_t i32_key_temp;
int8_t pressKey;
int8_t releaseKey;
int8_t longKey;
uint8_t u8_cnt; //u8_cnt -> Debounce cnt
uint8_t u8_keyCnt;	//u8_keyCnt->long key cnt 
uint32_t getkey_temp;

int8_t RangeCode1;
int8_t RangeCode;
int8_t FunCode;
int8_t Func_temp;
int8_t NowFunc_temp;
int8_t LastFunc_temp;
uint8_t u8_FuncCnt;

//unsigned long RMSBUFF05;
long RMSBUFF05;
signed long i16_mainVal;

unsigned int AD1Data_000;///for Test

//int32_t  AD1Data_buff;
uint8_t SPI_Data[64];
uint8_t SPI_DataOut[3];
int32_t ADCData;
long 	Cal_AD1_temp;
int	    cal_stepNum;

int32_t AD1DataBuffer;
int32_t AD2DataBuffer;
int32_t AD3DataBuffer;
int32_t LPF2DataBuffer;
int32_t LPF3DataBuffer;
int32_t RMS2DataBufferLo;
int32_t RMS2DataBufferHi;
int32_t RMS3DataBufferLo;
int32_t RMS3DataBufferHi;
int32_t PeakMinDataBuffer;
int32_t PeakMaxDataBuffer;
long   RMS2DataBuffer;	
double Count_A;
double Count_B;
double Count_C;
int8_t R20Buffer;
int8_t R29Buffer;
int8_t R34Buffer;
int8_t INTFBuffer;

int32_t FreqBuff2;

double FreqBuffer;
double DutyBuffer;
double PeriodBuffer;
double PeriodBuffer1;
double PeriodBuffer2;
double FreqBuff1;
int32_t DutyBuffer1;
double TotalTime;
uint8_t BeepSTATUS;
uint8_t A14Buffer;
void SPI_3131_Read(void)	
{	
	if (DrvGPIO_GetBit(HY3131_PORT,MISO_PIN)) //IRQ occur, Detect Data Ready Output
    {
      HY3131_ReadData (SPI_Data,HY3131_AD1_DATA0,52);  //Read All Register Data

      SPI_DataOut[0]=0x00; //Clear All Event Flag
      HY3131_WriteData (SPI_DataOut,HY3131_INTF,1);
      //HY3131_ReadData (SPI_DataOut,HY3131_INTF,1); //For Debug

      INTFBuffer=SPI_Data[HY3131_INTF]&SPI_Data[HY3131_INTE];
      R20Buffer=SPI_Data[HY3131_R20];
      R29Buffer=SPI_Data[HY3131_R29];
	  // A14Buffer=SPI_Data[HY3131_CTSTA];
      if((INTFBuffer&AD1F)==AD1F) //Check AD1 Event Flag
      {
        AD1DataBuffer=(SPI_Data[HY3131_AD1_DATA2]<<16)+(SPI_Data[HY3131_AD1_DATA1]<<8)+SPI_Data[HY3131_AD1_DATA0];
		AD1DataBuffer=AD1DataBuffer>>7;	//0x1FFFF~0x0FFFF
		ADCData=AD1DataBuffer;     
      }
      if((INTFBuffer&AD2F)==AD2F) //Check AD2 Event Flag
      {
        AD2DataBuffer=(SPI_Data[HY3131_AD2_DATA2]<<16)+(SPI_Data[HY3131_AD2_DATA1]<<8)+SPI_Data[HY3131_AD2_DATA0];
      }
      if((INTFBuffer&LPFF)==LPFF) //Check LPF Event Flag
      {
        LPF2DataBuffer=(SPI_Data[HY3131_LPF_DATA2]<<16)+(SPI_Data[HY3131_LPF_DATA1]<<8)+SPI_Data[HY3131_LPF_DATA0];
      }
      if((INTFBuffer&RMSF)==RMSF) //Check RMS Event Flag
      {
        RMS2DataBufferHi=(SPI_Data[HY3131_RMS_DATA4]<<24)+(SPI_Data[HY3131_RMS_DATA3]<<16)+(SPI_Data[HY3131_RMS_DATA2]<<8)+SPI_Data[HY3131_RMS_DATA1];
        RMS2DataBufferLo=SPI_Data[HY3131_RMS_DATA0];

      }
      if((R29Buffer&ENPKH)==ENPKH)
      {
        PeakMaxDataBuffer=(SPI_Data[HY3131_PKHMAX2]<<16)+(SPI_Data[HY3131_PKHMAX1]<<8)+SPI_Data[HY3131_PKHMAX0];
        PeakMinDataBuffer=(SPI_Data[HY3131_PKHMIN2]<<16)+(SPI_Data[HY3131_PKHMIN1]<<8)+SPI_Data[HY3131_PKHMIN0];
      }
      if((INTFBuffer&CTF)==CTF) //Check Frequency Counter Event Flag
      {
		flag.b_freqTimeOut=1; 
		flag.b_over2Sec_Freq=1;
		T1s_Cap_Cnt=0;
        Timer2sCnt=0;		
        Count_A=(SPI_Data[HY3131_CTA2]<<16)+(SPI_Data[HY3131_CTA1]<<8)+SPI_Data[HY3131_CTA0];
        Count_B=(SPI_Data[HY3131_CTB2]<<16)+(SPI_Data[HY3131_CTB1]<<8)+SPI_Data[HY3131_CTB0];
        Count_C=(SPI_Data[HY3131_CTC2]<<16)+(SPI_Data[HY3131_CTC1]<<8)+SPI_Data[HY3131_CTC0];

		if(flag.b_Freqlarge1M==1){TotalTime=(Count_A+CTA_Preset);}
		else{TotalTime=(Count_A+CTA_Preset_Low);} //1000000h-CTA<23:0>Initial+CTA<23:0>Final==>T1	  
	    //TotalTime=(Count_A+CTA_Preset_Low);  //1000000h-CTA<23:0>Initial+CTA<23:0>Final==>T1
        //FreqBuffer=(Count_B*49152)/(TotalTime/100);
		FreqBuffer=(Count_B*491520)/(TotalTime/1000);	
        //PeriodBuffer=(TotalTime*1000)/(Count_B*49152);
		PeriodBuffer=(TotalTime*1000000)/(Count_B*49152);
        DutyBuffer=(Count_C*10)/(TotalTime/1000);		//xx.xx%

        SPI_Data[0]=R20Buffer&(~ENCTR);
        HY3131_WriteData (SPI_Data,HY3131_R20,1);
        Delay(1000);
        HY3131_Count_Initial(R20Buffer);
      }

    }
}	