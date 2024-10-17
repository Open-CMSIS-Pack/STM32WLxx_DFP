/* -----------------------------------------------------------------------------
 * Copyright (c) 2014 ARM Ltd.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software. Permission is granted to anyone to use this
 * software for any purpose, including commercial applications, and to alter
 * it and redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software in
 *    a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *
 * $Date:        10. April 2019
 * $Revision:    V1.00
 *
 * Project:      Flash Programming Functions for ST STM32WLxx Flash
 * --------------------------------------------------------------------------- */

/* History:
 *  Version 1.00
 *    Initial release
 */

#include "..\FlashOS.H"        // FlashOS Structures

typedef volatile unsigned char    vu8;
typedef          unsigned char     u8;
typedef volatile unsigned short   vu16;
typedef          unsigned short    u16;
typedef volatile unsigned long    vu32;
typedef          unsigned long     u32;
typedef volatile unsigned long    vu64;
typedef          unsigned long     u64;

#define M8(adr)  (*((vu8  *) (adr)))
#define M16(adr) (*((vu16 *) (adr)))
#define M32(adr) (*((vu32 *) (adr)))
#define M64(adr) (*((vu64 *) (adr)))

// Peripheral Memory Map
#define IWDG_BASE         0x40003000
#define FLASH_BASE        0x58004000// 0x40022000
//0x40023C00

#define IWDG            ((IWDG_TypeDef *) IWDG_BASE)
#define FLASH           ((FLASH_TypeDef*) FLASH_BASE)

// Independent WATCHDOG
typedef struct {
  vu32 KR;
  vu32 PR;
  vu32 RLR;
  vu32 SR;
} IWDG_TypeDef;

// Flash Registers
typedef struct {
  vu32 ACR;
  vu32 ACR2;
  vu32 KEYR;
  vu32 OPTKEYR;
  vu32 SR;
  vu32 CR;
  vu32 ECCR;
  vu32 RESERVED0;
  vu32 OPTR;
	vu32 PCROP1ASR;         /*!< FLASH bank1 PCROP start address register, Address offset: 0x24 */
  vu32 PCROP1AER;         /*!< FLASH bank1 PCROP end address register,   Address offset: 0x28 */
  vu32 WRP1AR;           /*!< FLASH bank1 WRP area A address register,  Address offset: 0x2C */
  vu32 WRP1BR;           /*!< FLASH bank1 WRP area B address register,  Address offset: 0x30 */
  vu32 RESERVED2[4];   /*!< Reserved2,                                Address offset: 0x34 */
  vu32 PCROP2SR;         /*!< FLASH bank2 PCROP start address register, Address offset: 0x44 */
  vu32 PCROP2ER;         /*!< FLASH bank2 PCROP end address register,   Address offset: 0x48 */
  vu32 WRP2AR;           /*!< FLASH bank2 WRP area A address register,  Address offset: 0x4C */
  vu32 WRP2BR;           /*!< FLASH bank2 WRP area B address register,  Address offset: 0x50 */
  vu32 PCROP2BSR;           /*!< FLASH PCROP2BSR address register,  Address offset: 0x54 */
   vu32 PCROP2BER;       /*!< PCROP2BER,                               Address offset: 0x58 */
  vu32 ACR_M0;           /*!< FLASH bank2 WRP area B address register,  Address offset: 0x5C */
  vu32 SR_M0;             /*!< FLASH status register,                    Address offset: 0x60 */
  vu32 CR_M0;             /*!< FLASH status register,                    Address offset: 0x64 */
 
} FLASH_TypeDef;

// Flash Keys
#define RDPRT_KEY                    0x00A5
#define FLASH_KEY1                  (0x45670123ul)
#define FLASH_KEY2                  (0xCDEF89ABul)
#define FLASH_SR_BSY1               ((vu32)0x00010000)
#define FLASH_SR_BSY2               ((vu32)0x00010000)
#define FLASH_SR_CFGBSY             ((vu32)0x00040000)
#define FLASH_SR_PESD               ((vu32)0x00080000)
#define FLASH_CR_BKER_D             ((vu32)0x00004000)
#define FLASH_CR_PNB_D              ((vu32)0x00001FF8)


// Flash Control Register definitions
#define FLASH_PG                ((unsigned int)(1U <<  0))
#define FLASH_PER               ((unsigned int)(1U <<  1))
#define FLASH_MER1              ((unsigned int)(1U <<  2))
#define FLASH_PNB_MSK           ((unsigned int)(0xFFU << 3))
#define FLASH_BKER              ((unsigned int)(1U << 11))
#define FLASH_MER2              ((unsigned int)(1U << 15))
#define FLASH_STRT              ((unsigned int)(1U << 16))
#define FLASH_LOCK              ((unsigned int)(1U << 31))


// Flash Status Register definitions
#define FLASH_SR_EOP            ((unsigned int)(1U <<  1))
#define FLASH_OPERR             ((unsigned int)(1U <<  1))
#define FLASH_PROGERR           ((unsigned int)(1U <<  3))
#define FLASH_WRPERR            ((unsigned int)(1U <<  4))
#define FLASH_PGAERR            ((unsigned int)(1U <<  5))
#define FLASH_SIZERR            ((unsigned int)(1U <<  6))
#define FLASH_PGSERR            ((unsigned int)(1U <<  7))
#define FLASH_MISSERR           ((unsigned int)(1U <<  8))
#define FLASH_FASTERR           ((unsigned int)(1U <<  9))
#define FLASH_RDERR             ((unsigned int)(1U << 14))

#define FLASH_BSY               ((unsigned int)(1U << 16))

#define FLASH_PGERR             (FLASH_OPERR  | FLASH_PROGERR | FLASH_PROGERR | FLASH_WRPERR  | FLASH_PGAERR | \
                                FLASH_SIZERR | FLASH_PGSERR  | FLASH_MISSERR | FLASH_FASTERR | FLASH_RDERR   )


/*
 *  Initialize Flash Programming Functions
 *    Parameter:      adr:  Device Base Address
 *                    clk:  Clock Frequency (Hz)
 *                    fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */

int Init (unsigned long adr, unsigned long clk, unsigned long fnc) {


	 if ((FLASH->CR & FLASH_LOCK) != 0){
		 
  FLASH->KEYR = FLASH_KEY1;                             // Unlock Flash
  FLASH->KEYR = FLASH_KEY2;
	 }
   
  return (0);
}

/*
 *  De-Initialize Flash Programming Functions
 *    Parameter:      fnc:  Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *    Return Value:   0 - OK,  1 - Failed
 */

int UnInit (unsigned long fnc) {

  FLASH->CR |= FLASH_LOCK;                                // Lock Flash

  return (0);
}

/*
 *  Erase complete Flash Memory
 *    Return Value:   0 - OK,  1 - Failed
 */
#if defined FLASH_MEM
int EraseChip (void) {

  while (FLASH->SR & FLASH_BSY) {
    IWDG->KR = 0xAAAA;                                    // Reload IWDG
  }
//  FLASH->SR  = 0x000001FF;                               // Reset Error Flags
  
#if defined STM32WLxx_CM0P
	FLASH->SR_M0  = 0x000001FF; 
	  while(FLASH->SR_M0 & (1<<16));                     //check busy flag 
    while(FLASH->SR_M0 & (1<<19));                     // check PSED flag
    while (FLASH->SR & FLASH_SR_CFGBSY);
	 FLASH->SR_M0 |= 0x000001FF;                         // clear error
    FLASH->CR_M0 |=(1<<2) ;                               //select page and set MER
    FLASH->CR_M0 |= (1<<16);                            // set strt bit
    while(FLASH->SR_M0 & (1<<16));                      //check busy flag 
    while (FLASH->SR & FLASH_SR_CFGBSY);
    while(FLASH->SR_M0 & (1<<19));  
    FLASH->CR_M0 &=~(1<<2); 
#else
	FLASH->SR  = 0x000001FF;
	while(FLASH->SR & (1<<16));                     //check busy flag 
    while(FLASH->SR & (1<<19));                     // check PSED flag
    while (FLASH->SR & FLASH_SR_CFGBSY);
	 FLASH->SR |= 0x000001FF;                         // clear error
    FLASH->CR |=(1<<2) ;                               //select page and set MER
    FLASH->CR|= (1<<16);                            // set strt bit
    while(FLASH->SR & (1<<16));                      //check busy flag 
    while (FLASH->SR & FLASH_SR_CFGBSY);
    while(FLASH->SR & (1<<19));  
    FLASH->CR &=~(1<<2); 
			
#endif
	 
  return (0);                                             // Done
}

#endif // FLASH_MEM

/*
 *  Erase Sector in Flash Memory
 *    Parameter:      adr:  Sector Address
 *    Return Value:   0 - OK,  1 - Failed
 */
#if defined FLASH_MEM
int EraseSector (unsigned long adr) {
	
 unsigned long page;
	FLASH->SR = 0x000001FF;

#if defined STM32WLxx_CM0P
	/*Clear the error status*/
  FLASH->SR_M0 = 0x000001FF;
  /*Clear page and block bits*/
  FLASH->CR_M0 &= ~(FLASH_CR_PNB_D );
	
	 page = (adr >>8) & 0x00001FF8;
	while (FLASH->SR_M0 & FLASH_SR_BSY1);
  while (FLASH->SR_M0 & FLASH_SR_CFGBSY);
   while (FLASH->SR_M0 & FLASH_SR_PESD);
	/*Set the page and the Page erase bit*/
  FLASH->CR_M0 |= (FLASH_PER | page);
  /*Start the erase operation*/  
  FLASH->CR_M0 |= FLASH_STRT;
	      __asm volatile("NOP");
        __asm volatile("NOP"); 
	      
	while (FLASH->SR_M0 & FLASH_SR_BSY1);
  while (FLASH->SR_M0 & FLASH_SR_CFGBSY);
  while (FLASH->SR_M0 & FLASH_SR_PESD);

  FLASH->CR_M0 &= (~FLASH_PER);
	FLASH->CR_M0 &= ~(FLASH_CR_PNB_D );
	FLASH->CR_M0 &= ~(FLASH_STRT );
				
#else
	
	/*Clear the error status*/
  FLASH->SR = 0x000001FF;
  /*Clear page and block bits*/
  FLASH->CR &= ~(FLASH_CR_PNB_D );
  /*Calculate the page and set the block bit if needed*/
	 page = (adr >>8) & 0x00001FF8;
  
    while (FLASH->SR & FLASH_SR_BSY1);
  while (FLASH->SR & FLASH_SR_CFGBSY);
   while (FLASH->SR & FLASH_SR_PESD);
  /*Set the page and the Page erase bit*/
  FLASH->CR |= (FLASH_PER |  page);
  /*Start the erase operation*/  
  FLASH->CR |= FLASH_STRT;
 
        __asm volatile("NOP");
        __asm volatile("NOP"); 
				
  /*wait until the operation ends*/
     while (FLASH->SR & FLASH_SR_BSY1 );
  while (FLASH->SR & FLASH_SR_CFGBSY);
    while (FLASH->SR & FLASH_SR_PESD);
 
   FLASH->CR &= (~FLASH_PER);                              // Reset CR
	 FLASH->CR &= (~FLASH_PER);
	FLASH->CR &= ~(FLASH_CR_PNB_D );
	FLASH->CR &= ~(FLASH_STRT );
	#endif
                                         
	if ((* (unsigned long *)0x08000000) == 0xFFFFFFFF)
  {
    FLASH->ACR |= 0x10000;
  }
  return (0);                                             // Done
}
#endif // FLASH_MEM

/*
 *  Program Page in Flash Memory
 *    Parameter:      adr:  Page Start Address
 *                    sz:   Page Size
 *                    buf:  Page Data
 *    Return Value:   0 - OK,  1 - Failed
 */
#if defined FLASH_MEM
int ProgramPage (unsigned long adr, unsigned long sz, unsigned char *buf) {                                   
   
   unsigned char tab[8];
   int i;
  unsigned long align = adr%8;
  unsigned long tempadd = 0;
	
	 sz = (sz + 7) & ~7;          	// Adjust size for two words
	
 #if defined STM32WLxx_CM0P
	  
   /*Clear the error status*/
  FLASH->SR_M0 = 0x000001FF;
  /*Flash programming enabled*/
  FLASH->CR_M0 |= FLASH_PG;  
  
   while (sz>0)                                      
    {      
       if(sz>=8)
      {
        FLASH->SR = 0x000001FF;
          if (align !=0)
      {
         tempadd = (adr -align);
            for(i=0;i<align;i++)
        {
          tab[i]= *((unsigned char *)tempadd);
          tempadd=tempadd+1;
        }        
          for(i=0;i<8-align;i++)
        {
          tab[i+align]=*((unsigned char *)buf);
          buf=buf+1;
        }
             tempadd = (adr -align);
      while (FLASH->SR_M0 & FLASH_SR_BSY1 );
      while (FLASH->SR_M0 & FLASH_SR_CFGBSY);
      while (FLASH->SR_M0 & FLASH_SR_PESD);
   /*write double world*/
    M32(tempadd ) = *((u32 *)&tab );                   // Program the first word of the Double Word
    M32(tempadd + 4) = *((u32 *)(&tab + 4)); 
        __asm volatile("NOP");
        __asm volatile("NOP"); 
       
               sz =sz -(8-align);               
               adr= adr +(8-align);
               align = 0;                
  }      
      while (FLASH->SR_M0 & FLASH_SR_BSY1 );
      while (FLASH->SR_M0 & FLASH_SR_CFGBSY);
      while (FLASH->SR_M0 & FLASH_SR_PESD);
   /*write double world*/
      M32(adr ) = *((u32 *)buf );                   // Program the first word of the Double Word
      M32(adr + 4) = *((u32 *)(buf + 4)); 
        buf +=8; 
        sz -= 8;
        adr +=8;
        __asm volatile("NOP");
        __asm volatile("NOP");            
      }    
      else{ 
         /*write a word completed with 0xFF*/         

        for(i=0;i<sz;i++)
        {
          tab[i]= *((unsigned char *)buf);
          buf=buf+1;
        }        
         for(i=0;i<8-sz;i++)
        {
          tab[i+sz]=0xFF;
        }
      while (FLASH->SR_M0 & (FLASH_SR_BSY1 | FLASH_SR_BSY2));
      while (FLASH->SR_M0 & FLASH_SR_CFGBSY);
      while (FLASH->SR_M0 & FLASH_SR_PESD);
   
        M32(adr ) = *((u32 *)&tab );                   // Program the first word of the Double Word
        M32(adr + 4) = *((u32 *)(&tab + 4)); 
				
         sz =0;            
      }      
        /*wait until the operation ends*/
      while (FLASH->SR_M0 & FLASH_SR_BSY1 );
      while (FLASH->SR_M0 & FLASH_SR_CFGBSY);
      while (FLASH->SR_M0 & FLASH_SR_PESD);         
         }   
    FLASH->CR_M0 &= (~FLASH_PG);
#else
	
	  /*Clear the error status*/
  FLASH->SR = 0x000001FF;
  /*Flash programming enabled*/
  FLASH->CR |= FLASH_PG;  

   while (sz>0)                                   
    {      
      if(sz>=8)
      {
        FLASH->SR = 0x000001FF;
          if (align !=0)
      {
         tempadd = (adr -align);
            for(i=0;i<align;i++)
        {
          tab[i]= *((unsigned char *)tempadd);
          tempadd=tempadd+1;
        }        
          for(i=0;i<8-align;i++)
        {
          tab[i+align]=*((unsigned char *)buf);
           buf=buf+1;
        }
        tempadd = (adr -align);
         while (FLASH->SR & FLASH_SR_BSY1 );
         while (FLASH->SR & FLASH_SR_CFGBSY);
         while (FLASH->SR & FLASH_SR_PESD);
      /*write double world*/
	      M32(tempadd ) = *((u32 *)&tab );                   // Program the first word of the Double Word
        M32(tempadd + 4) = *((u32 *)(&tab + 4)); 
				   
			
        __asm volatile("NOP");
        __asm volatile("NOP"); 
          
               sz =sz -(8-align);               
               adr= adr +(8-align);
               align = 0;                
  }
      
       while (FLASH->SR & FLASH_SR_BSY1 );
       while (FLASH->SR & FLASH_SR_CFGBSY);
       while (FLASH->SR & FLASH_SR_PESD);
     /*write double world*/
	     M32(adr ) = *((u32 *)buf );                   // Program the first word of the Double Word
       M32(adr + 4) = *((u32 *)(buf + 4)); 
    
	    
	
        buf +=8; 
        sz -= 8;
        adr +=8;

        __asm volatile("NOP");
        __asm volatile("NOP");  
      }
    
      else{ 
        for(i=0;i<sz;i++)
        {
          tab[i]= *((unsigned char *)buf);
          buf=buf+1;
        }        
           for(i=0;i<8-sz;i++)
        {
          tab[i+sz]=0xFF;
        }
               while (FLASH->SR & FLASH_SR_BSY1 );
               while (FLASH->SR & FLASH_SR_CFGBSY);
               while (FLASH->SR & FLASH_SR_PESD);
				
				 M32(adr ) = *((u32 *)&tab );                   // Program the first word of the Double Word
        M32(adr + 4) = *((u32 *)(&tab + 4)); 
           
				 
               sz =0;      
      }
        /*wait until the operation ends*/
          while (FLASH->SR & FLASH_SR_BSY1 );
          while (FLASH->SR & FLASH_SR_CFGBSY);
          while (FLASH->SR & FLASH_SR_PESD);        
         } 
  
  FLASH->CR &= (~FLASH_PG);
	#endif
	 if ((* (unsigned long *)0x08000000) != 0xFFFFFFFF)
  {
    FLASH->ACR &= 0xFFFEFFFF;
  }

  return (0);                                             
}
#endif // FLASH_MEM
