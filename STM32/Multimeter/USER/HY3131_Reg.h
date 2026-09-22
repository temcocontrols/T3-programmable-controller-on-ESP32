/******************************************************************************/
/* Filename:HY3131_REG.h                                                      */
/* HY3131 Register Definitions                                                */
/******************************************************************************/

//SPI Protocol
//Register Address<6:0> left shift 1 bit + Read/Write

//---- Data Register -----------------------------------------------------------
//(1) AD1<23:0>:Register for High Resolution ADC (AD1) output data.
//    Max. value: 7FFFFFh, min. value:800000h.
#define  HY3131_AD1_DATA0  0x00 //AD1<7:0>
#define  HY3131_AD1_DATA1  0x01 //AD1<15:8>
#define  HY3131_AD1_DATA2  0x02 //AD1<23:16>

//(2) AD2<23:0>:Register for High Resolution ADC (AD2) output data.
//    Max. value: 03FFFFh, min.value:FC0000h.
//    AD2<18:0> is effective bit, AD<23:19> and AD<18> is the same.
#define  HY3131_AD2_DATA0  0x03 //AD2<7:0>
#define  HY3131_AD2_DATA1  0x04 //AD2<15:8>
#define  HY3131_AD2_DATA2  0x05 //AD2<23:16>

//(3) LPF<23:0>:Register for Low Pass Filter output data.
//    Max. value: 03FFFFh, min. value: FC0000h.
//    LPF<18:0> is effective bit, LPF<23:19> and LPF<18> is the same.
#define  HY3131_LPF_DATA0  0x06 //LPF<7:0>
#define  HY3131_LPF_DATA1  0x07 //LPF<15:8>
#define  HY3131_LPF_DATA2  0x08 //LPF<23:16>

//(4) RMS<39:0>:Register for RMS Converter output data.
//    Max. value: 1FFFFFFFFFh, min. value:E000000000h.
//    RMS<37:0> is effective bit, RMS<39:38> and RMS<37> is the same.
#define  HY3131_RMS_DATA0  0x09 //RMS<7:0>
#define  HY3131_RMS_DATA1  0x0A //RMS<15:8>
#define  HY3131_RMS_DATA2  0x0B //RMS<23:16>
#define  HY3131_RMS_DATA3  0x0C //RMS<31:24>
#define  HY3131_RMS_DATA4  0x0D //RMS<39:32>

//(5) PKHMAX<23:0>:Register for Peak Hold max. output data.
//    Max. value: 03FFFFh, min. value: FC0000h.
//    PKHMAX<18:0> is effective bit, PKHMAX<23:19> and PKHMAX<18> is the same.
#define  HY3131_PKHMIN0  0x0E //PKHMIN<7:0>
#define  HY3131_PKHMIN1  0x0F //PKHMIN<15:8>
#define  HY3131_PKHMIN2  0x10 //PKHMIN<23:16>

//(6) PKHMIN<23:0>:Register for Peak Hold min. output data.
//    Max. value: 03FFFFh, min. value: FC0000h.
//    PKHMIN<18:0> is effective bit, PKHMIN<23:19> and PKHMIN<18> is the same.
#define  HY3131_PKHMAX0  0x11 //PKHMAX<7:0>
#define  HY3131_PKHMAX1  0x12 //PKHMAX<15:8>
#define  HY3131_PKHMAX2  0x13 //PKHMAX<23:16>

// Comparator
#define  HY3131_CTSTA  0x14
         #define  PCNTI 0x80 //Bit7:CNT Buffer output
         #define  ACPO  0x40 //Bit6:Comparator output
         #define  CMPHO 0x20 //Bit5:Comparator High Output
         #define  CMPLO 0x10 //Bit4:Comparator Low Output
         #define  CTBOV 0x01 //Bit0:CTB<23:0> is Over Flow Flag

//(7) CTC<23:0>:Register for Frequency Counter data.
//    Max. value: FFFFFFh, min. value: 000000h.
#define  HY3131_CTC0  0x15 //CTC<7:0>
#define  HY3131_CTC1  0x16 //CTC<15:8>
#define  HY3131_CTC2  0x17 //CTC<23:16>

//(8) CTB<23:0>:Register for Frequency Counter data.
//    Max. value: FFFFFFh, min. value: 000000h.
#define  HY3131_CTB0  0x18 //CTB<7:0>
#define  HY3131_CTB1  0x19 //CTB<15:8>
#define  HY3131_CTB2  0x1A //CTB<23:16>

//(9) CTA<23:0>:Register for Frequency Counter data.
//    Max. value: FFFFFFh, min. value: 000000h.
#define  HY3131_CTA0  0x1B //CTA<7:0>
#define  HY3131_CTA1  0x1C //CTA<15:8>
#define  HY3131_CTA2  0x1D //CTA<23:16>

//---- Interrupt ---------------------------------------------------------------
#define  HY3131_INTF  0x1E  //INTF : IRQ Event Register
         //When VDD lowers than 1.9V, BORF will be set 1. This bit has neither relative INTEx nor IRQ event.
         #define  BORF  0x80

         //RMSF will be set 1 when RMS event takes place.
         #define  RMSF  0x10

         //LPFF will be set 1 when Low Pass Filter event takes place.
         #define  LPFF  0x08

         //AD1F will be set 1 when AD1 event takes place.
         #define  AD1F  0x04

         //AD2F will be set 1 when AD2 event takes place.
         #define  AD2F  0x02

         //CTF will be set 1 when F requency Counter event takes place.
         #define  CTF   0x01

#define  HY3131_INTE  0x1F  //INTE : IRQ Enable Register
         //IRQ is generated when RMS event takes place. 1=Enable, 0=Disable.
         #define  RMSIE 0x10

         //IRQ is generated when Low Pass Filter event takes place. 1=Enable, 0=Disable.
         #define  LPFIE 0x08

         //IRQ is generated when AD1event takes place. 1=Enable, 0=Disable.
         #define  AD1IE 0x04

         //IRQ is generated when AD2 event takes place. 1=Enable, 0=Disable.
         #define  AD2IE 0x02

         //IRQ is generated when Frequency Counter event takes place. 1=Enable, 0=Disable.
         #define  CTIE  0x01

//---- Config ------------------------------------------------------------------
#define  HY3131_R20  0x20
         //SCMPI<2:0>:Comparator Input
         #define  INCMP_SENSE  0x00
         #define  INCMP_FB     0x20
         #define  INCMP_OP1O   0x40
         #define  INCMP_PB0    0x60
         #define  INCMP_PB1    0x80
         #define  INCMP_RLD    0xA0
         #define  INCMP_PB3    0xC0
         #define  INCMP_PB4    0xE0

         //Register bit that can enable CMPH and CMPL comparator. 1=Enable, 0=Disable.
         #define  ENCMP        0x10

         //Register bit that can Enable CNT Buffer and can select input source of Frequency Counter.
         #define  ENCNTI_PCNTI 0x08 //Frequency Counter Input PCNTI
         #define  ENCNTI_ACPO  0x00 //Frequency Counter Input ACPO

         //Register bit that can Enable CMP Pin Buffer. 1=Enable; 0=Disable.
         #define  ENPCMPO      0x04

         //ENCTR:Register bit that can Enable Frequency Counter. 1=Enable;
         //0=Disable and clear CTA<23:0>,CTB<23:0>, CTC<23:0> and CTBOV as 0.
         #define  ENCTR        0x02

#define  HY3131_R21  0x21
         //SCMPRH<3:0>:Comparator voltage selection
         #define  VRHCMP_VDSC16 0x00
         #define  VRHCMP_VDSC13 0x10
         #define  VRHCMP_VDSC11 0x20
         #define  VRHCMP_VDSC10 0x30
         #define  VRHCMP_VDSC9  0x40
         #define  VRHCMP_VDSC8  0x50
         #define  VRHCMP_VDSC7  0x60
         #define  VRHCMP_PB7    0x70
         #define  VRHCMP_AGNDP6 0x80
         #define  VRHCMP_AGNDP5 0x90
         #define  VRHCMP_AGNDP4 0xA0
         #define  VRHCMP_AGNDP3 0xB0
         #define  VRHCMP_AGNDP2 0xC0
         #define  VRHCMP_AGNDP1 0xD0
         #define  VRHCMP_AGNDP0 0xE0
         #define  VRHCMP_AGNDN1 0xF0

         //SCMPRL<3:0>:Comparator voltage selection
         #define  VRLCMP_VDSC2  0x00
         #define  VRLCMP_VDSC5  0x01
         #define  VRLCMP_VDSC7  0x02
         #define  VRLCMP_VDSC8  0x03
         #define  VRLCMP_VDSC9  0x04
         #define  VRLCMP_VDSC10 0x05
         #define  VRLCMP_VDSC11 0x06
         #define  VRLCMP_PB6    0x07
         #define  VRLCMP_AGNDN6 0x08
         #define  VRLCMP_AGNDN5 0x09
         #define  VRLCMP_AGNDN4 0x0A
         #define  VRLCMP_AGNDN3 0x0B
         #define  VRLCMP_AGNDN2 0x0C
         #define  VRLCMP_AGNDN1 0x0D
         #define  VRLCMP_AGNDP0 0x0E
         #define  VRLCMP_AGNDP1 0x0F

#define  HY3131_R22  0x22
         //AD1OS<2:0>:Register bit that can configure DVOS of zero input voltage.
         #define  DCOS_0       0x00
         #define  DCOS_0p25    0x20
         #define  DCOS_0p5     0x40
         #define  DCOS_0p75    0x60
         //#define  DCOS_0       0x80
         #define  DCOS_n0p25   0xA0
         #define  DCOS_n0p5    0xC0
         #define  DCOS_n0p75   0xE0

         //AD1CHOP<1:0>:Register bit that can configure Chop AD1 input signal,
         // the result is reflected at the output AD1<23:0> of AD1.
         #define  AD1CHOP_VXpVOS 0x00  //VX+VOS
         #define  AD1CHOP_VXnVOS 0x08  //VX-VOS
         #define  AD1CHOP_VX     0x10  //VX
         //#define  AD1CHOP_VX    0x18  //VX

         //AD1OSR<2:0>:Register bit that can configure Over Sampling Ratio (OSR1) of AD1 Comb Filter.
         //AD1 data output rate=FAD1CLK/OSR1. FAD1CLK is the frequency of AD1CLK.
         #define  AD1OSR_256   0x00
         #define  AD1OSR_512   0x01
         #define  AD1OSR_1024  0x02
         #define  AD1OSR_2048  0x03
         #define  AD1OSR_4096  0x04
         #define  AD1OSR_8192  0x05
         #define  AD1OSR_16384 0x06
         #define  AD1OSR_32768 0x07

#define  HY3131_R23  0x23
         //Register bit that can Enable AD1. 1=Enable, 0=Disable and clear AD1<23:0> as 0.
         #define  ENAD1        0x80

         //Register bit that can configure Gin of AD1 reference signal.
         #define  AD1RG_0p333  0x10 //AD1 Reference Gain 0.333
         #define  AD1RG_1p0    0x00 //AD1 Reference Gain 1.0

         //Register bit that can configure whether positive reference signal of AD1 passes through Buffer.1=Enable?F0=Disable.
         #define  AD1RHBUF     0x08

         //Register bit that can configure whether negative reference signal of AD1 passes through Buffer.1=Enable?F0=Disable.
         #define  AD1RLBUF     0x04

         //Register bit that can configure whether positive input signal of AD1 passes through Buffer.1=Enable?F0=Disable.
         #define  AD1IPBUF     0x02

         //Register bit that can configure whether negative input signal of AD1 passes through Buffer.1=Enable?F0=Disable.
         #define  AD1INBUF     0x01

#define  HY3131_R24  0x24
         //SAD1FP<3:0>:Filter Positive Input
         #define  SAD1FP_SENSE 0x00
         #define  SAD1FP_FB    0x10
         #define  SAD1FP_RLU   0x20
         #define  SAD1FP_OP1O  0x30
         #define  SAD1FP_OP2O  0x40
         #define  SAD1FP_VDD   0x50
         #define  SAD1FP_REFO  0x60
         #define  SAD1FP_VREF  0x70
         #define  SAD1FP_PB0   0x80
         #define  SAD1FP_PB1   0x90
         #define  SAD1FP_PB2   0xA0
         #define  SAD1FP_PB3   0xB0
         #define  SAD1FP_PB4   0xC0
         #define  SAD1FP_PB5   0xD0
         #define  SAD1FP_PB6   0xE0
         #define  SAD1FP_PB7   0xF0

         //Register bit SDIO can connect PB<0> and PB<8> pin.
         #define  SDIO         0x08

         //SAD1FN<2:0>:Filter Negative Input
         #define  SAD1FN_SENSE 0x00
         #define  SAD1FN_RLU   0x01
         #define  SAD1FN_VSS   0x02
         #define  SAD1FN_AGND  0x03
         #define  SAD1FN_PB2   0x04
         #define  SAD1FN_PB3   0x05
         #define  SAD1FN_PB4   0x06
         #define  SAD1FN_PB5   0x07

#define  HY3131_R25  0x25
         //AD2IG<1:0>:Register bit that can configure input signal gain of AD2.
         #define  AD2IG_0p5    0x00 //AD2 Input 0.5 Gain
         #define  AD2IG_1p0    0x40 //AD2 Input 1.0 Gain
         #define  AD2IG_1p5    0x80 //AD2 Input 1.5 Gain
         #define  AD2IG_2p0    0xC0 //AD2 Input 2.0 Gain

         //AD1IG<1:0>:Register bit that can configure Gain of AD1 input signal.
         #define  AD1IG_0p9    0x00 //AD1 Input 0.9 Gain
         #define  AD1IG_1p8    0x10 //AD1 Input 1.8 Gain
         #define  AD1IG_2p7    0x20 //AD1 Input 2.7 Gain
         #define  AD1IG_3p6    0x30 //AD1 Input 3.6 Gain

         //SACM<1:0>:Register bit that can select ACM voltage(ACM-VSS).
         #define  SACM_1p2     0x00 //ACM-VSS=1.2V
         #define  SACM_0p9     0x04 //ACM-VSS=0.9V
         #define  SACM_1p5     0x08 //ACM-VSS=1.5V
         #define  SACM_1p125   0x0C //ACM-VSS=1.125V

         //Register bit OPS<2:1> can determine the negative input pin of OPAMP to be OPXN or OPXO pin.
         #define  EnOPS2       0x02 //OP2 Non-Inverting Amplifier
         #define  DisOPS2      0x00 //OP2 Unity-Gain Buffer
         #define  EnOPS1       0x01 //OP1 Non-Inverting Amplifier
         #define  DisOPS1      0x00 //OP1 Unity-Gain Buffer

#define  HY3131_R26  0x26
         //Register bit that can Enable AD2.
         #define  ENAD2        0x80 //1=Enable.
         #define  DisAD2       0x00 //0=Disable and clear AD2<18:0> as 0.

         //Register bit that can Enable Chop input signal of AD2.1=Enable, 0=Disable.
         #define  ENCHOPAD2    0x20

         //Register bit that can configure reference signal gain of AD2.
         #define  AD2RG_1p0    0x00 //1.0
         #define  AD2RG_0p333  0x10 //0.333

         //FSYSCLK is the frequency of SYSCLK; FAD2CLK is the frequency of AD2CLK.
         #define  SAD2CLK_Div2 0x00 //SAD2CLK=0:FAD2CLK=FSYSCLK/2
         #define  SAD2CLK_Div4 0x08 //SAD2CLK=1:FAD2CLK=FSYSCLK/4

         //AD2OSR<2:0>:Register bit that can configure Over Sampling Ratio (OSR2) of AD2 Comb Filter.
         //AD2 data output rate=FAD2CLK/OSR2.
         #define  AD2OSR_32    0x00
         #define  AD2OSR_64    0x01
         #define  AD2OSR_128   0x02
         #define  AD2OSR_256   0x03
         #define  AD2OSR_512   0x04
         #define  AD2OSR_1024  0x05
         //#define  AD2OSR_1024  0x06
         //#define  AD2OSR_1024  0x07

#define  HY3131_R27  0x27
         //SAD2IP<1:0>:The positive input signal of AD2
         #define  SAD2IP_OP1O  0x00
         #define  SAD2IP_OP2O  0x40
         #define  SAD2IP_PB4   0x80
         #define  SAD2IP_PB7   0xC0

         //SAD2IN<1:0>:The negative input signal of AD2
         #define  SAD2IN_RLU   0x00
         #define  SAD2IN_AGND  0x10
         #define  SAD2IN_PB3   0x20
         #define  SAD2IN_PB5   0x30

         //SAD2RH<1:0>:The positive reference signal of AD2
         #define  SAD2RH_FB    0x00
         #define  SAD2RH_REFO  0x04
         #define  SAD2RH_VREF  0x08
         #define  SAD2RH_PB6   0x0C

         //SAD2RL<1:0>:The negative reference signal of AD2
         #define  SAD2RL_RLU   0x00
         #define  SAD2RL_AGND  0x01
         #define  SAD2RL_PB3   0x02
         #define  SAD2RL_PB5   0x03

#define  HY3131_R28  0x28
         //SAD1RH<2:0>:The positive reference signal of AD1
         #define  SAD1RH_FB    0x00
         #define  SAD1RH_REFO  0x10
         #define  SAD1RH_VREF  0x20
         #define  SAD1RH_PB6   0x30
         #define  SAD1RH_RLU   0x40
         #define  SAD1RH_VDD   0x50
         #define  SAD1RH_AGND  0x60

         //SAD1RL<2:0>:The negative reference signal of AD1
         #define  SAD1RL_RLU   0x00
         #define  SAD1RL_AGND  0x01
         #define  SAD1RL_PB3   0x02
         #define  SAD1RL_PB5   0x03
         #define  SAD1RL_FB    0x04
         #define  SAD1RL_VSS   0x05
         #define  SAD1RL_VREF  0x06

#define  HY3131_R29  0x29
         //Register bit that can Enable RMS Converter. 1=Enable; 0=Disable and clear RMS<37:0> as 0.
         #define  ENRMS        0x80

         //Register bit that can Enable Low Pass Filter. 1=Enable; 0=Disable and clear LPF<18:0> as 0.
         #define  ENLPF        0x40

         //LPFBW<2:0>:Register bit that can configure Over Sampling Ratio (OSR4) of Low Pass Filter.
         //Low Pass Filter data output rate=data input rate/OSR4.
         #define  LPFBW_128    0x00
         #define  LPFBW_256    0x08
         #define  LPFBW_512    0x10
         #define  LPFBW_1024   0x18
         #define  LPFBW_2048   0x20
         #define  LPFBW_4096   0x28
         #define  LPFBW_8192   0x30
         #define  LPFBW_16384  0x38

         //Register bit that can Enable Peak Hold.
         //1=Enable; 0=Disable and clear PKHMAX<18:0> as 40000h, PKHMIN<18:0> as 3FFFFh.
         #define  ENPKH        0x04

         //PKHSEL<1:0>:Register bit that can select Peak Hold input to be AD1<23:5>, AD2<18:0> or LPF<18:0>.
         #define  PKHSEL_AD2   0x00 //AD2<18:0>
         #define  PKHSEL_AD1   0x01 //AD1<23:5>
         #define  PKHSEL_LPF   0x02 //LPF<18:0>
         //#define  PKHSEL_LPF   0x03 //LPF<18:0>

#define  HY3131_R2A  0x2A
         #define  PS1 0x80  //PS1?GPA<1> power select control bit
         #define  DS1 0x40  //DS1?GPA<1> OP3 output select control bit
         #define  FS1 0x20  //FS1?GPA<1> Feedback select control bit
         #define  SS1 0x10  //SS1?GPA<1> Sense end select control bit
         #define  PS0 0x08  //PS0?GPA<0> power select control bit
         #define  DS0 0x04  //DS0?GPA<0> OP3 output select control bit
         #define  FS0 0x02  //FS0?GPA<0> Feedback select control bit
         #define  SS0 0x01  //SS0?GPA<0> Sense end select control bit

#define  HY3131_R2B  0x2B
         #define  PS3 0x80  //PS3?GPA<3> power select control bit
         #define  DS3 0x40  //DS3?GPA<3> OP3 output select control bit
         #define  FS3 0x20  //FS3?GPA<3> Feedback select control bit
         #define  SS3 0x10  //SS3?GPA<3> Sense end select control bit
         #define  PS2 0x08  //PS2?GPA<2> power select control bit
         #define  DS2 0x04  //DS2?GPA<2> OP3 output select control bit
         #define  FS2 0x02  //FS2?GPA<2> Feedback select control bit
         #define  SS2 0x01  //SS2?GPA<2> Sense end select control bit

#define  HY3131_R2C  0x2C
         #define  PS5 0x80  //PS5?GPA<5> power select control bit
         #define  DS5 0x40  //DS5?GPA<5> OP3 output select control bit
         #define  FS5 0x20  //FS5?GPA<5> Feedback select control bit
         #define  SS5 0x10  //SS5?GPA<5> Sense end select control bit
         #define  PS4 0x08  //PS4?GPA<4> power select control bit
         #define  DS4 0x04  //DS4?GPA<4> OP3 output select control bit
         #define  FS4 0x02  //FS4?GPA<4> Feedback select control bit
         #define  SS4 0x01  //SS4?GPA<4> Sense end select control bit

#define  HY3131_R2D  0x2D
         #define  PS7 0x80  //PS7?GPA<7> power select control bit
         #define  DS7 0x40  //DS7?GPA<7> OP3 output select control bit
         #define  FS7 0x20  //FS7?GPA<7> Feedback select control bit
         #define  SS7 0x10  //SS7?GPA<7> Sense end select control bit
         #define  PS6 0x08  //PS6?GPA<6> power select control bit
         #define  DS6 0x04  //DS6?GPA<6> OP3 output select control bit
         #define  FS6 0x02  //FS6?GPA<6> Feedback select control bit
         #define  SS6 0x01  //SS6?GPA<6> Sense end select control bit

#define  HY3131_R2E  0x2E
         #define  PS9 0x80  //PS9?GPA<9> power select control bit
         #define  DS9 0x40  //DS9?GPA<9> OP3 output select control bit
         #define  FS9 0x20  //FS9?GPA<9> Feedback select control bit
         #define  SS9 0x10  //SS9?GPA<9> Sense end select control bit
         #define  PS8 0x08  //PS8?GPA<8> power select control bit
         #define  DS8 0x04  //DS8?GPA<8> OP3 output select control bit
         #define  FS8 0x02  //FS8?GPA<8> Feedback select control bit
         #define  SS8 0x01  //SS8?GPA<8> Sense end select control bit

#define  HY3131_R2F  0x2F
         //Register bit that can enable Voltage Reference Generator1. 1=Enable?A0=Disable.
         #define  ENVS     0x80

         //SMODE<6:0>
         #define  SMODE6   0x40 //Reserve
         #define  SMODE5   0x20 //SMODE5:Register bit SMODE<5> can connect RLU and AGND pin.
         #define  RLU_AGND SMODE5
         #define  SMODE4   0x10 //SMODE4:Register bit SMODE<4> can connect RLU and RLD pin.
         #define  RLU_RLD  SMODE4
         #define  SMODE3   0x08
         #define  SMODE2   0x04
         #define  SMODE1   0x02
         #define  SMODE0   0x01

         //SMODE Setting Application
         #define  PCC_Source  0x06 //00000110:Positive Constant Current Source
         #define  NCC_Source  0x07 //00000111:Negative Constant Current Source
         #define  CAP_CC_MODE 0x0F //0000111x:Capacitor measurement Constant Current Source
         #define  AGND_Source 0x11 //00000001:AGND Source
         #define  VDD_Source  0x12 //00000010:VDD Source
         #define  VSS_Source  0x13 //00000011:VSS Source
         #define  PCV_Source  0x14 //00010100:Positive Constant Voltage Source
         #define  NCV_Source  0x15 //00010101:Negative Constant Voltage Source
         #define  CAP_CV_MODE 0x1A //0001101x:Capacitor measurement Constant Voltage Source

#define  HY3131_R30  0x30
         //Register bit that can select input source of REFO Buffer.
         #define  SREFO_PB6 0x80 //1: select PB<6> pin.
         #define  SREFO_Int 0x00 //0: select relative AGND 1.2V voltage of the internal Band gap;

         //ACC<6:0>:Capacitor array can compensate ACV measurement bandwidth; the capacitance value is shown in the
         //above graph. The capacitance value is controlled by register bit ACC<6:0>.

#define  HY3131_R31  0x31
         //Register bit that can enable approximately 1.2V voltage of relative AGND of the internal Band gap
         //Voltage Reference and can enable REFO Buffer. 1=Enable, 0=Disable. When it is set 0, REFO pin is in Floating status.
         #define  ENREFO       0x80

         //Register bit that can enable bias circuit, providing bias for all analog circuit. 1=Enable, 0=Disable.
         #define  ENBIAS       0x40

         //SAGND<1:0>:AGND pin voltage select
         #define  SAGND_0P5    0x00 //00:AGND=0.5xVDD
         #define  SAGND_0P3    0x10 //01:AGND=0.3xVDD
         #define  SAGND_0P1    0x20 //10:AGND=0.1xVDD
         #define  SAGND_Dis    0x30 //11:Disable AGND Generator, AGND pin is in Floating status

         //SFUVR<3:0>:VREF voltage selection
         #define  SFUVR_VDS17  0x00
         #define  SFUVR_VDS16  0x01
         #define  SFUVR_VDS15  0x02
         #define  SFUVR_AGNDP9 0x03
         #define  SFUVR_AGNDP8 0x04
         #define  SFUVR_AGNDP7 0x05
         #define  SFUVR_AGNDP6 0x06
         #define  SFUVR_AGNDP0 0x07
         #define  SFUVR_VDS1   0x08
         #define  SFUVR_VDS2   0x09
         #define  SFUVR_VDS3   0x0A
         #define  SFUVR_AGNDN9 0x0B
         #define  SFUVR_AGNDN8 0x0C
         #define  SFUVR_AGNDN7 0x0D
         #define  SFUVR_AGNDN6 0x0E
         #define  SFUVR_PB7    0x0F

#define  HY3131_R32  0x32
         //Enable OP2 respectively
         #define  ENOP2       0x80

         //SOP2P<2:0>:OP2 Positive Input
         #define  SOP2P_SENSE 0x00
         #define  SOP2P_FB    0x10
         #define  SOP2P_RLU   0x20
         #define  SOP2P_AGND  0x30
         #define  SOP2P_PB0   0x40
         #define  SOP2P_PB1   0x50
         #define  SOP2P_PB2   0x60
         #define  SOP2P_PB3   0x70

         //Enable OP1 respectively
         #define  ENOP1       0x08

         //SOP1P<2:0>:OP1 Positive Input
         #define  SOP1P_SENSE 0x00
         #define  SOP1P_FB    0x01
         #define  SOP1P_RLU   0x02
         #define  SOP1P_AGND  0x03
         #define  SOP1P_PB0   0x04
         #define  SOP1P_PB1   0x05
         #define  SOP1P_PB2   0x06
         #define  SOP1P_PB8   0x07

#define  HY3131_R33  0x33
         //OP1CHOP<1:0>:The chopper clock of OP1
         #define  CHOP_CLK0   0x00 //0
         #define  CHOP_CLK1k  0x40 //1k Hz square wave
         #define  CHOP_CLK2k  0x80 //2k Hz Square wave
         #define  CHOP_CLK1   0xC0 //1

         //Register bit that can enable Crystal Oscillator. 1=Enable; 0=Disable.
         #define  ENOSC     0x20

         //Register bit that can select system clock. SYSCLK. 0:SYSCLK=Crystal Oscillator output?F1:SYSCLK =XIN.
         #define  ENXI      0x10

         //SFT1<1:0>:Register bit that can select filter resistor as 100K, 10K, 0 or nil, as shown in the above graph.
         #define  SFT1_100K 0x00
         #define  SFT1_10K  0x04
         #define  SFT1_0    0x08
         #define  SFT1_nil  0x0C

         //SAD1I<1:0>:The input signal of AD1
         #define  SAD1I_AD1FP_AD1FN 0x00 //AD1IP:AD1FP,  AD1IN:AD1FN
         #define  SAD1I_FB_RLU      0x01 //AD1IP:FB,  AD1IN:RLU
         #define  SAD1I_TS1P_TS2N   0x02 //AD1IP:TS1P,  AD1IN:TS2N
         #define  SAD1I_TS1N_TS2P   0x03 //AD1IP:TS1N,  AD1IN:TS2P

#define  HY3131_R37  0x37 //Testing Mode, Don't use or Write "0" only

//------------------------------------------------------------------------------
#define  ReadComm  0x01
#define  WriteComm 0x00


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

// > 6000,向上換檔 6000
#define	C_upLimit	6000
// < 570, 向下跳檔 6000*0.095=570
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

MCUSTATUS  	MCUSTATUSbits;
DMMSTATUS  	DMMSTATUSbits;
TIME_BASE	TSTATUS;
KEY_STATUS	KSTATUS;
//TYPE_flag	flag;

/******************************************************************************
  END
 ******************************************************************************/
