/******************************************************************************/
/* Filename:HY3131_FUNC.h                                                     */
/******************************************************************************/
#ifndef _HY3131_FUNC_H_
#define _HY3131_FUNC_H_

//#include "main.h"
#include "HY3131_Reg.h"

//#define  E_DCVolt  0
//#define  E_ACVolt  1
//#define  E_DCmVolt 2
//#define  E_ACmVolt 3
//#define  E_Diode   4
//#define  E_Temp    5
//#define  E_OHMCV   6
//#define  E_OHMCC   7
//#define  E_CapCV   8
//#define  E_CapCC   9
//#define  E_Freq    10

/******************************************************************************
 DC Voltage
 ******************************************************************************/
#if defined(AD1)
#define  DCVoltR20  INCMP_SENSE  // R20
#define  DCVoltR21  0  // R21
#define  DCVoltR22  AD1CHOP_VX|AD1OSR_4096  // R22
#define  DCVoltR23  ENAD1|AD1RHBUF|AD1RLBUF|AD1IPBUF|AD1INBUF  // R23
#define  DCVoltR24  SAD1FN_RLU   // R24
#define  DCVolt0p6R25  AD1IG_1p8|EnOPS1 //R25
#define  DCVoltR25  AD1IG_0p9|DisOPS2|DisOPS1  // R25
#define  DCVoltR26  0  // R26
#define  DCVoltR27  0  // R27
#define  DCVoltR28  SAD1RH_PB6|SAD1RL_AGND  // R28
#define  DCVoltR29  PKHSEL_AD1  //R29

#elif defined(AD2)
#define  DCVoltR20  INCMP_SENSE  // R20
#define  DCVoltR21  0  // R21
#define  DCVoltR22  0  // R22
#define  DCVoltR23  0  // R23
#define  DCVoltR24  0  // R24
#define  DCVolt0p6R25  AD2IG_1p0|EnOPS1 //R25
#define  DCVoltR25  AD2IG_1p0|DisOPS2|DisOPS1  // R25
#define  DCVoltR26  ENAD2|AD2OSR_1024  // R26
#define  DCVoltR27  SAD2IP_OP1O|SAD2IN_PB3|SAD2RH_PB6|SAD2RL_AGND  // R27
#define  DCVoltR28  0  // R28
#define  DCVoltR29  ENLPF|LPFBW_1024|PKHSEL_AD2  //R29

#elif defined(LPF)
#define  DCVoltR20  INCMP_SENSE  // R20
#define  DCVoltR21  0  // R21
#define  DCVoltR22  0  // R22
#define  DCVoltR23  0  // R23
#define  DCVoltR24  0  // R24
#define  DCVolt0p6R25  AD2IG_1p0|EnOPS1 //R25
#define  DCVoltR25  AD2IG_1p0|DisOPS2|DisOPS1  // R25
#define  DCVoltR26  ENAD2|AD2OSR_1024  // R26
#define  DCVoltR27  SAD2IP_OP1O|SAD2IN_PB3|SAD2RH_PB6|SAD2RL_AGND  // R27
#define  DCVoltR28  0  // R28
#define  DCVoltR29  ENLPF|LPFBW_1024|PKHSEL_LPF  //R29
#endif

#define  DCVoltR2F  ENVS|RLU_AGND  // R2F
#define  DCVoltR30  SREFO_PB6  // R30
#define  DCVoltR31  ENREFO|ENBIAS|SAGND_0P5|SFUVR_AGNDP0  // R31
#define  DCVoltR32  SOP2P_AGND|SOP1P_AGND  //R32
#define  DCVoltR33  ENOSC|SFT1_100K|SAD1I_AD1FP_AD1FN  //R33

/* Analog Switch Network */
  //For 6V Range
#define  Volt6VR2A  FS1|FS0
#define  Volt6VR2B  0
#define  Volt6VR2C  0
#define  Volt6VR2D  PS7|SS7
#define  Volt6VR2E  FS9|PS8

  //For 60V Range
#define  Volt60VR2A  FS1|FS0
#define  Volt60VR2B  0
#define  Volt60VR2C  0
#define  Volt60VR2D  PS6|SS6
#define  Volt60VR2E  FS9|PS8

  //For 600V Range
#define  Volt600VR2A  FS1|FS0
#define  Volt600VR2B  0
#define  Volt600VR2C  PS5|SS5
#define  Volt600VR2D  0
#define  Volt600VR2E  FS9|PS8

  //For 1000V Range
#define  Volt1kVR2A  FS1|FS0
#define  Volt1kVR2B  0
#define  Volt1kVR2C  PS4|SS4
#define  Volt1kVR2D  0
#define  Volt1kVR2E  FS9|PS8

/******************************************************************************
 AC Voltage
 ******************************************************************************/
#define  ACVoltR20  INCMP_PB4|ENCMP|ENCNTI_ACPO|ENPCMPO|ENCTR  // R20
#define  ACVoltR21  VRHCMP_AGNDP1|VRLCMP_AGNDN1  // R21
#define  ACVoltR22  0  // R22
#define  ACVoltR23  0  // R23
#define  ACVoltR24  0  // R24
#define  ACVoltR25  AD2IG_1p0|EnOPS1  // R25
#define  ACVoltR26  ENAD2|AD2OSR_64  // R26
#define  ACVoltR27  SAD2IP_OP1O|SAD2IN_PB3|SAD2RH_PB6|SAD2RL_AGND  // R27
#define  ACVoltR28  0 // R28
#define  ACVoltR29  ENRMS|ENLPF|LPFBW_16384|PKHSEL_AD2 //R29

#define  ACVoltR2F  ENVS|RLU_AGND // R2F
#define  ACVoltR30  SREFO_PB6 // R30
#define  ACVoltR31  ENREFO|ENBIAS|SAGND_0P5 // R31
#define  ACVoltR32  ENOP1|SOP1P_SENSE //R32
#define  ACVoltR33  ENOSC|SFT1_nil|SAD1I_AD1FP_AD1FN  //R33

/******************************************************************************
 DC MilliVoltage
 ******************************************************************************/
#if defined(AD1)
#define  DCmVoltR20  INCMP_PB0  // R20
#define  DCmVoltR21  0  // R21
#define  DCmVoltR22  DCOS_0|AD1CHOP_VX|AD1OSR_16384  // R22
#define  DCmVoltR23  ENAD1|AD1RHBUF|AD1IPBUF   // R23
#define  DCmVoltR24  SAD1FP_OP1O|SAD1FN_PB3 //  R24
#define  DC50mVoltR25  AD1IG_0p9|DisOPS2|EnOPS1  // R25
#define  DC500mVoltR25  AD1IG_0p9|DisOPS2|DisOPS1  // R25
#define  DCmVoltR26  0  // R26
#define  DCmVoltR27  0  // R27
#define  DCmVoltR28  SAD1RH_PB6|SAD1RL_AGND // R28
#define  DCmVoltR29  PKHSEL_AD1 //R29

#elif defined(AD2)
#define  DCmVoltR20  INCMP_PB0  // R20
#define  DCmVoltR21  0  // R21
#define  DCmVoltR22  0  // R22
#define  DCmVoltR23  0  // R23
#define  DCmVoltR24  0  // R24
#define  DC50mVoltR25  AD2IG_1p0|DisOPS2|EnOPS1  // R25
#define  DC500mVoltR25  AD2IG_1p0|DisOPS2|DisOPS1  // R25
#define  DCmVoltR26  ENAD2|AD2OSR_1024  // R26
#define  DCmVoltR27  SAD2IP_OP1O|SAD2IN_PB3|SAD2RH_PB6|SAD2RL_AGND  // R27
#define  DCmVoltR28  0 // R28
#define  DCmVoltR29  ENLPF|LPFBW_1024|PKHSEL_AD2 //R29

#elif defined(LPF)
#define  DCmVoltR20  INCMP_PB0  // R20
#define  DCmVoltR21  0  // R21
#define  DCmVoltR22  0  // R22
#define  DCmVoltR23  0  // R23
#define  DCmVoltR24  0  // R24
#define  DC50mVoltR25  AD2IG_1p0|DisOPS2|EnOPS1  // R25
#define  DC500mVoltR25  AD2IG_1p0|DisOPS2|DisOPS1  // R25
#define  DCmVoltR26  ENAD2|AD2OSR_1024  // R26
#define  DCmVoltR27  SAD2IP_OP1O|SAD2IN_PB3|SAD2RH_PB6|SAD2RL_AGND  // R27
#define  DCmVoltR28  0 // R28
#define  DCmVoltR29  ENLPF|LPFBW_1024|PKHSEL_LPF //R29
#endif

#define  DCmVoltR2F  ENVS|SMODE0 // R2F
#define  DCmVoltR30  SREFO_PB6 // R30
#define  DCmVoltR31  ENREFO|ENBIAS|SAGND_0P5 // R31
#define  DCmVoltR32  ENOP1|SOP1P_PB0 //R32
#define  DCmVoltR33  ENOSC|SFT1_100K|SAD1I_AD1FP_AD1FN //R33

/* Analog Switch Network */
 //For 60mV Range
 //For 600mV Range
#define  mVoltR2A  0
#define  mVoltR2B  0
#define  mVoltR2C  0
#define  mVoltR2D  0
#define  mVoltR2E  PS8
/******************************************************************************
 AC MilliVoltage
 ******************************************************************************/
#define  ACmVoltR20  INCMP_PB0  // R20
#define  ACmVoltR21  0  // R21
#define  ACmVoltR22  0  // R22
#define  ACmVoltR23  0  // R23
#define  ACmVoltR24  0  // R24
#define  AC50mVoltR25  AD2IG_1p0|DisOPS2|EnOPS1  // R25
#define  AC500mVoltR25  AD2IG_1p0|DisOPS2|DisOPS1  // R25
#define  ACmVoltR26  ENAD2|AD2OSR_64  // R26
#define  ACmVoltR27  SAD2IP_OP1O|SAD2IN_PB3|SAD2RH_PB6|SAD2RL_AGND  // R27
#define  ACmVoltR28  0 // R28
#define  ACmVoltR29  ENRMS|ENLPF|LPFBW_16384|PKHSEL_AD2 //R29
            
#define  ACmVoltR2F  ENVS|SMODE0 // R2F
#define  ACmVoltR30  SREFO_PB6 // R30
#define  ACmVoltR31  ENREFO|ENBIAS|SAGND_0P5 // R31
#define  ACmVoltR32  ENOP1|SOP1P_RLU //R32
#define  ACmVoltR33  ENOSC|SFT1_nil|SAD1I_AD1FP_AD1FN //R33

/******************************************************************************
 Diode
 ******************************************************************************/
#define  DiodeR20  INCMP_PB0  // R20
#define  DiodeR21  0  // R21
#define  DiodeR22  DCOS_0|AD1CHOP_VX|AD1OSR_16384  // R22
#define  DiodeR23  ENAD1|AD1RHBUF|AD1IPBUF   // R23
#define  DiodeR24  SAD1FP_PB0|SDIO|SAD1FN_PB3 //  R24
#define  DiodeR25  AD1IG_0p9  // R25
#define  DiodeR26  0  // R26
#define  DiodeR27  0  // R27
#define  DiodeR28  SAD1RH_PB6|SAD1RL_AGND // R28
#define  DiodeR29  0 //R29

#define  DiodeRR2F  ENVS|SMODE2|SMODE1 // R2F //Forward
#define  DiodeFR2F  ENVS|SMODE2|SMODE1|SMODE0 // R2F //Reverse
#define  DiodeR30  SREFO_PB6 // R30
#define  DiodeR31  ENREFO|ENBIAS|SAGND_0P5|SFUVR_VDS16 // R31
#define  DiodeR32  0 //R32
#define  DiodeR33  ENOSC|SFT1_100K|SAD1I_AD1FP_AD1FN //R33

/* Analog Switch Network */
  //For Diode
#define  DiodeR2A  0
#define  DiodeR2B  DS3
#define  DiodeR2C  PS4
#define  DiodeR2D  0
#define  DiodeR2E  0

/******************************************************************************
 Temperature
 ******************************************************************************/
#define  TempR20  INCMP_PB0  // R20
#define  TempR21  0  // R21
#define  TempR22  DCOS_0|AD1CHOP_VX|AD1OSR_16384  // R22
#define  TempR23  ENAD1|AD1RHBUF|AD1IPBUF   // R23
#define  TempR24  SAD1FP_OP1O|SAD1FN_PB3  //  R24
#define  TempR25  AD1IG_0p9|DisOPS2|DisOPS1  // R25
#define  TempR26  0  // R26
#define  TempR27  0  // R27
#define  TempR28  SAD1RH_PB6|SAD1RL_AGND // R28
#define  TempR29  0 //R29

#define  TempR2F  ENVS|SMODE0 // R2F
#define  TempR30  SREFO_PB6 // R30
#define  TempR31  ENREFO|ENBIAS|SAGND_0P5|SFUVR_AGNDP0 // R31
#define  TempR32  ENOP1|SOP1P_PB0 //R32
#define  TempPR33  ENOSC|SFT1_100K|SAD1I_TS1N_TS2P //R33 //TCode2 //Positive
#define  TempNR33  ENOSC|SFT1_100K|SAD1I_TS1P_TS2N //R33 //TCode1 //Negative

/* Analog Switch Network */
#define  TempR2A  0
#define  TempR2B  0
#define  TempR2C  0
#define  TempR2D  0
#define  TempR2E  PS8

/******************************************************************************
 Constant Voltage Resistance 
 ******************************************************************************/
#define  OHMCVR20  INCMP_PB0  // R20
#define  OHMCVR21  0  // R21
#define  OHMCVR22  DCOS_0|AD1CHOP_VX|AD1OSR_16384  // R22
#define  OHMCVR23  ENAD1|AD1RHBUF|AD1RLBUF|AD1IPBUF|AD1INBUF   // R23
#define  OHMCVR24  SAD1FP_PB0|SAD1FN_PB3 //  R24
#define  OHMCVR25  AD1IG_0p9|DisOPS1  // R25
#define  OHMCVR26  0  // R26
#define  OHMCVR27  0  // R27
#define  OHMCVR28  SAD1RH_FB|SAD1RL_RLU // R28
#define  OHMCVR29  0 //R29

#define  OHMCVR2F  ENVS|RLU_RLD|SMODE2 // R2F
#define  OHMCVR30  SREFO_PB6 // R30
#define  OHMCVR31  ENREFO|ENBIAS|SAGND_0P3|SFUVR_VDS15 // R31
#define  OHMCVR32  ENOP1|SOP1P_PB0 //R32
#define  OHMCVR33  ENOSC|SFT1_100K|SAD1I_AD1FP_AD1FN //R33

/* Analog Switch Network */
  //For 600ohm Range
#define  OHMCV600R2A  0
#define  OHMCV600R2B  DS3
#define  OHMCV600R2C  DS4|FS4
#define  OHMCV600R2D  0
#define  OHMCV600R2E  DS8
  //For 6kohm Range
#define  OHMCV6kR2A  0
#define  OHMCV6kR2B  DS3
#define  OHMCV6kR2C  DS5|FS5
#define  OHMCV6kR2D  0
#define  OHMCV6kR2E  DS8
  //For 60kohm Range
#define  OHMCV60kR2A  0
#define  OHMCV60kR2B  DS3
#define  OHMCV60kR2C  0
#define  OHMCV60kR2D  DS6|FS6
#define  OHMCV60kR2E  DS8
  //For 600kohm Range
#define  OHMCV600kR2A  0
#define  OHMCV600kR2B  DS3
#define  OHMCV600kR2C  0
#define  OHMCV600kR2D  DS7|FS7
#define  OHMCV600kR2E  DS8
 //For 6Mohm Range
#define  OHMCV6MR2A  0
#define  OHMCV6MR2B  DS3
#define  OHMCV6MR2C  DS5|FS5
#define  OHMCV6MR2D  0
#define  OHMCV6MR2E  DS8|FS8

/******************************************************************************
 Constant Current Resistance 
 ******************************************************************************/
#define  OHMCCR20  INCMP_PB0  // R20
#define  OHMCCR21  0  // R21
#define  OHMCCR22  DCOS_0|AD1CHOP_VX|AD1OSR_16384  // R22
#define  OHMCCR23  ENAD1|AD1IPBUF|AD1INBUF   // R23
#define  OHMCCR24  SAD1FP_PB0|SAD1FN_PB3 //  R24
#define  OHMCCR25  AD1IG_0p9|DisOPS1  // R25
#define  OHMCCR26  0  // R26
#define  OHMCCR27  0  // R27
#define  OHMCCR28  SAD1RH_PB6|SAD1RL_AGND // R28
#define  OHMCCR29  0 //R29

#define  OHMCCR2F  ENVS|SMODE2|SMODE1 // R2F
#define  OHMCCR30  SREFO_PB6 // R30
#define  OHMCCR31  ENREFO|ENBIAS|SAGND_0P3|SFUVR_VDS16 // R31
#define  OHMCCR32  ENOP1|SOP1P_PB0 //R32
#define  OHMCCR33  ENOSC|SFT1_100K|SAD1I_AD1FP_AD1FN //R33

/* Analog Switch Network */
  //For 6kohm Range
#define  OHMCC6kR2A  0
#define  OHMCC6kR2B  DS3
#define  OHMCC6kR2C  PS4
#define  OHMCC6kR2D  0
#define  OHMCC6kR2E  0
  //For 60kohm Range
#define  OHMCC60kR2A  0
#define  OHMCC60kR2B  DS3
#define  OHMCC60kR2C  PS5
#define  OHMCC60kR2D  0
#define  OHMCC60kR2E  0
  //For 600kohm Range
#define  OHMCC600kR2A  0
#define  OHMCC600kR2B  DS3|PS2
#define  OHMCC600kR2C  0
#define  OHMCC600kR2D  PS6
#define  OHMCC600kR2E  0
 //For 6Mohm Range
#define  OHMCC6MR2A  0
#define  OHMCC6MR2B  DS3|PS2
#define  OHMCC6MR2C  0
#define  OHMCC6MR2D  PS7
#define  OHMCC6MR2E  0
  //For 60Mohm Range
#define  OHMCC60MR2A  0
#define  OHMCC60MR2B  DS3|PS2
#define  OHMCC60MR2C  0
#define  OHMCC60MR2D  0
#define  OHMCC60MR2E  0

/******************************************************************************
 Constant Voltage  Capacitance 
 ******************************************************************************/
#define  CapCVR20  INCMP_PB0|ENCMP|ENCNTI_ACPO|ENPCMPO|ENCTR  // R20
#define  CapCVR21  VRHCMP_VDSC13|VRLCMP_VDSC5  // R21
#define  CapCVR22  0  // R22
#define  CapCVR23  0  // R23
#define  CapCVR24  0  // R24
#define  CapCVR25  0  // R25
#define  CapCVR26  0  // R26
#define  CapCVR27  0  // R27
#define  CapCVR28  0  // R28
#define  CapCVR29  0  // R29

#define  CapCVR2F  ENVS|CAP_CV_MODE // R2F
#define  CapCVR30  0 // R30
#define  CapCVR31  ENREFO|ENBIAS|SAGND_0P5|SFUVR_VDS16 // R31
#define  CapCVR32  ENOP1|SOP1P_PB2 //R32
#define  CapCVR33  ENOSC|SFT1_nil|SAD1I_AD1FP_AD1FN //R33

/* Analog Switch Network */
  //use 1.11M ohm
#define  CapCVR0R2A 0
#define  CapCVR0R2B PS2
#define  CapCVR0R2C 0
#define  CapCVR0R2D PS7
#define  CapCVR0R2E 0
  //use 101k ohm
#define  CapCVR1R2A 0
#define  CapCVR1R2B PS2
#define  CapCVR1R2C 0
#define  CapCVR1R2D PS6
#define  CapCVR1R2E 0
  //use 10k ohm
#define  CapCVR2R2A 0
#define  CapCVR2R2B PS2
#define  CapCVR2R2C PS5
#define  CapCVR2R2D 0
#define  CapCVR2R2E 0

  //use 1k ohm
#define  CapCVR3R2A 0
#define  CapCVR3R2B PS2
#define  CapCVR3R2C PS4
#define  CapCVR3R2D 0
#define  CapCVR3R2E 0

/******************************************************************************
 Constant Current Capacitance 
 ******************************************************************************/
#if defined(Gain1)
#define  CapCCR20  INCMP_PB0|ENCMP|ENCNTI_ACPO|ENPCMPO|ENCTR  // R20
#elif defined(Gain10)
#define  CapCCR20  INCMP_OP1O|ENCMP|ENCNTI_ACPO|ENPCMPO|ENCTR  // R20
#endif
#define  CapCCR21  VRHCMP_VDSC13|VRLCMP_VDSC11  // R21
//#define  CapCCR21  VRHCMP_AGNDP5|VRLCMP_AGNDN5  // R21
#define  CapCCR22  0  // R22
#define  CapCCR23  0  // R23
#define  CapCCR24  0  // R24
#if defined(Gain1)
#define  CapCCR25  0  // R25
#elif defined(Gain10)
#define  CapCCR25  EnOPS1  // R25
#endif
#define  CapCCR26  0  // R26
#define  CapCCR27  0  // R27
#define  CapCCR28  0  // R28
#define  CapCCR29  0  // R29

#define  CapCCHiR22  DCOS_0|AD1CHOP_VXpVOS|AD1OSR_4096  // R22
#define  CapCCHiR23  ENAD1|AD1RHBUF|AD1IPBUF  // R23
#define  CapCCHiR24  SAD1FP_PB2|SAD1FN_PB3  // R24
#define  CapCCHiR25  AD1IG_0p9|DisOPS1  // R25
#define  CapCCHiR28  SAD1RH_VDD|SAD1RL_VSS // R28

#define  CapCCR2F  ENVS|CAP_CC_MODE // R2F
#define  CapCCR30  0 // R30
#define  CapCCR31  ENREFO|ENBIAS|SAGND_0P5|SFUVR_VDS15 // R31
#define  CapCCR32  ENOP1|SOP1P_PB2 //R32
#define  CapCCR33  ENOSC|SFT1_nil|SAD1I_AD1FP_AD1FN //R33

/* Analog Switch Network */
  //use 1.11M ohm
#define  CapCCR0R2A 0
#define  CapCCR0R2B PS2
#define  CapCCR0R2C 0
#define  CapCCR0R2D PS7
#define  CapCCR0R2E 0
  //use 101k ohm
#define  CapCCR1R2A 0
#define  CapCCR1R2B PS2
#define  CapCCR1R2C 0
#define  CapCCR1R2D PS6
#define  CapCCR1R2E 0
  //use 10k ohm
#define  CapCCR2R2A 0
#define  CapCCR2R2B PS2
#define  CapCCR2R2C PS5
#define  CapCCR2R2D 0
#define  CapCCR2R2E 0

  //use 1k ohm
#define  CapCCR3R2A 0
#define  CapCCR3R2B PS2
#define  CapCCR3R2C PS4
#define  CapCCR3R2D 0
#define  CapCCR3R2E 0

/******************************************************************************
 Frequency 
 ******************************************************************************/
#if defined(FIN_ACPO)
#define  FreqR20 INCMP_PB4|ENCMP|ENCNTI_ACPO|ENPCMPO|ENCTR  // R20
#elif defined(FIN_CNTI)
#define  FreqR20  INCMP_PB4|ENCMP|ENCNTI_PCNTI|ENPCMPO|ENCTR  // R20
#endif
#define  FreqR21  VRHCMP_AGNDP1|VRLCMP_AGNDN1  // R21
#define  FreqR22  0  // R22
#define  FreqR23  0  // R23
#define  FreqR24  0  // R24
#define  FreqR25  EnOPS1  // R25
#define  FreqR26  0  // R26
#define  FreqR27  0  // R27
#define  FreqR28  0  // R28
#define  FreqR29  0  // R29

#define  FreqR2F  ENVS|RLU_AGND // R2F
#define  FreqR30  0 // R30
#define  FreqR31  ENREFO|ENBIAS|SAGND_0P5 // R31
#define  FreqR32  ENOP1|SOP1P_SENSE //R32
#define  FreqR33  ENOSC|SFT1_nil|SAD1I_AD1FP_AD1FN //R33

  //IRQ Enable
#if defined(AD1)
#define  DCIRQ    AD1IE
#elif defined(AD2)
#define  DCIRQ    AD2IE
#elif defined(LPF)
#define  DCIRQ    LPFIE
#endif

#define  ACIRQ    RMSIE
#define  FreqIRQ  CTIE
#define  TempIRQ  AD1IE
#define  OHMIRQ   AD1IE
#define  DiodeIRQ AD1IE

#endif // _HY3131_FUNC_H_
