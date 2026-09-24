//ADCCLK=> SYSCLK=Crystal=4.9152MHz
//         SYSCLK/30=AD1CLK=163.84KHz
//         AD1 data output rate=AD1CLK/OSR1 ,OSR1=101=>8192
//         AD1 data output rate=163.84K/8192=20SPS

#include "Dmm_DCV.h"
extern void Delay(unsigned int num);

void AD1_DCV_PROC(signed int AD1Data_buff)
{
  if((NowFunc_temp!=Func_DCV) || (FunCode!=E_DCVolt)) {return;}	
  if(flag.b_unStable_state==1){return;}			
  if((AD1Data_buff<0)||(AD1Data_buff>=0x10000))//0x80000000
  { 	
    AD1Data_buff=AD1Data_buff-Volt_6VOffset;
    AD1Data_buff=~AD1Data_buff;
    AD1Data_buff++;
	AD1Data_buff=AD1Data_buff& 0x0001FFFF;
	MINUS=1;
	  if(AD1Data_buff>=0x10000)
	  {
	    AD1Data_buff=~AD1Data_buff;
	    AD1Data_buff++;
	    AD1Data_buff=AD1Data_buff& 0x0000FFFF;
	    MINUS=0;
	  }
 
  }
  else
  { 
	AD1Data_buff=AD1Data_buff-Volt_6VOffset;
    MINUS=0;
	if(AD1Data_buff<0)
	{
		AD1Data_buff=0;
	}
  }

  AD1Data_buff=(AD1Data_buff)*10000/9472;  //56826 => ADC@1.0000V=0x24C7+0x36(Voffset)=9469
  AD1Data_buff=AD1Data_buff/10;
  Bar_NumBuff=AD1Data_buff/200;
  Bar_NumBuff1=AD1Data_buff%200;
  i32_temp=AD1Data_buff;
  
}

//************Volt6V_ZERO*************//
void Get_Volt6V_Zero(void) 
{
  RangeCode=5;
  SwitchDMMFun();
  temp_Cnt = 50;
  while(temp_Cnt)
  {
	asm("NOP");
	SPI_3131_Read();
	Delay(10000);
	temp_Cnt--;	
  }	  
  Volt_6VOffset=AD1DataBuffer;
  flag.b_minus=0;
  if(Volt_6VOffset>=0x10000)
  {
	Volt_6VOffset=~Volt_6VOffset;
	Volt_6VOffset++;
	Volt_6VOffset=Volt_6VOffset& 0x0000FFFF;
  }	  
  else {flag.b_minus=1;}
}
//************Volt6V_ZERO*************//  

void DCV_initial(void)
{
  TimerCount=0;
  settling_time=C_125ms;
  RangeCode=0;
  Bar_NumBuff=0;
  Bar_NumBuff1=0;
  FunCode=E_DCVolt;
  flag.b_Auto_Status=1;
  KSTATUS.b_AutoEn = 1;
  KSTATUS.b_longKey = 0;
  KSTATUS.b_longKeyEn = 0;
  MCUSTATUSbits._byte = 0;
  DMMSTATUSbits._byte = 0;

  flag.b_ad1Ok = 0;
  temp_Cnt0=0;
  Get_Volt6V_Zero();
  RangeCode=0;
  SwitchDMMFun();
}

