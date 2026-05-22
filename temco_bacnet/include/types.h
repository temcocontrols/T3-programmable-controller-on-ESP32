#ifndef _TYPES_DOT_H

#define _TYPES_DOT_H       /* avoid recompilation */

#define MINI64




#define Byte 		unsigned char
#define Uint		unsigned short int
#define Ulong		unsigned long
#define S8_T			char
#define S16_T			short int
#define S32_T			long

#define U8_T 		unsigned char
#define U16_T		unsigned short int
#define U32_T		unsigned long


#define uint16		unsigned short int
#define uint8		unsigned char
#define uint32		unsigned long

#define int16		short int

/*#define uint16_t	unsigned short int
#define uint8_t		unsigned char
#define uint32_t	unsigned long*/

#define u16		unsigned int
#define u8		unsigned char
#define u32		unsigned long


#define BOOL		unsigned short int

#define TRUE    			1
#define FALSE   			0


//#define LOW_BYTE(word)	(word & 0x00FF)
//#define HIGH_BYTE(word)	((word & 0xFF00) >> 8)

/* Bit Definitions for Bitwise Operation */
#if !defined(BIT0)
#define BIT0 0x01
#endif
#if !defined(BIT1)
#define BIT1 0x02
#endif
#if !defined(BIT2)
#define BIT2 0x04
#endif
#if !defined(BIT3)
#define BIT3 0x08
#endif
#if !defined(BIT4)
#define BIT4 0x10
#endif
#if !defined(BIT5)
#define BIT5 0x20
#endif
#if !defined(BIT6)
#define BIT6 0x40
#endif
#if !defined(BIT7)
#define BIT7 0x80
#endif
#if !defined(BIT8)
#define BIT8 0x100
#endif
#if !defined(BIT9)
#define BIT9 0x200
#endif

#if !defined(BIT10)
#define BIT10 0x400
#endif
#if !defined(BIT11)
#define BIT11 0x800
#endif
#if !defined(BIT12)
#define BIT12 0x1000
#endif

#if !defined(BIT13)
#define BIT13 0x2000
#endif
#if !defined(BIT14)
#define BIT14 0x4000
#endif

#if !defined(BIT15)
#define BIT15 0x8000
#endif

#if !defined(BIT16)
#define BIT16 0x10000
#endif
#if !defined(BIT17)
#define BIT17 0x20000
#endif
#if !defined(BIT18)
#define BIT18 0x040000
#endif
#if !defined(BIT19)
#define BIT19 0x080000
#endif
#if !defined(BIT20)
#define BIT20 0x100000
#endif
#if !defined(BIT21)
#define BIT21 0x200000
#endif
#if !defined(BIT22)
#define BIT22 0x400000
#endif
#if !defined(BIT23)
#define BIT23 0x800000
#endif
#if !defined(BIT24)
#define BIT24 0x1000000
#endif
#if !defined(BIT25)
#define BIT25 0x2000000
#endif

#if !defined(BIT26)
#define BIT26 0x4000000
#endif
#if !defined(BIT27)
#define BIT27 0x8000000
#endif
#if !defined(BIT28)
#define BIT28 0x10000000
#endif

#if !defined(BIT29)
#define BIT29 0x20000000
#endif
#if !defined(BIT30)
#define BIT30 0x40000000
#endif

#if !defined(BIT31)
#define BIT31 0x80000000
#endif

//#define BIT0		0x01
//#define BIT1		0x02
//#define BIT2		0x04
//#define BIT3		0x08
//#define BIT4		0x10
//#define BIT5		0x20
//#define BIT6		0x40
//#define BIT7		0x80
//#define BIT8		0x0100
//#define BIT9		0x0200
//#define BIT10		0x0400
//#define BIT11		0x0800
//#define BIT12		0x1000
//#define BIT13		0x2000
//#define BIT14		0x4000
//#define BIT15		0x8000
//#define	BIT16		0x00010000
//#define	BIT17		0x00020000
//#define	BIT18		0x00040000
//#define	BIT19		0x00080000
//#define	BIT20		0x00100000
//#define	BIT21		0x00200000
//#define	BIT22		0x00400000
//#define	BIT23		0x00800000
//#define	BIT24		0x01000000
//#define	BIT25		0x02000000
//#define	BIT26		0x04000000
//#define	BIT27		0x08000000
//#define	BIT28		0x10000000
//#define	BIT29		0x20000000
//#define	BIT30		0x40000000
//#define	BIT31		0x80000000


#ifndef NULL
 #define NULL ((void *) 0L)
#endif



/* Keil compiler user define */
#define	KEIL_CPL

#ifdef KEIL_CPL
// #define XDATA	xdata
 #define IDATA	idata
 #define BDATA	bdata
 #define CODE	code
 #define FAR	far
#else
 #define XDATA
 #define IDATA
 #define BDATA
 #define CODE
 #define FAR
#endif

/* Serial interface command direction */
#define	SI_WR				BIT0
#define	SI_RD				BIT1

#define	FLASH_WR_ENB		(PCON |= PWE_)
#define	FLASH_WR_DISB		(PCON &= ~PWE_)

#endif  /* _TYPES_DOT_H */
