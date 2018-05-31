//
// RA8875 Display Controller Register Definitions.
//
//
#ifndef RA8875_REGS_H
#define RA8875_REGS_H

    // Touch Panel public macros
    
    /* Touch Panel Enable/Disable Reg TPCR0[7] */
    #define TP_ENABLE   ((uint8_t)(1<<7))
    #define TP_DISABLE  ((uint8_t)(0<<7))
    
    /* Touch Panel operating mode Reg TPCR1[6] */
    #define TP_MODE_AUTO    ((uint8_t)(0<<6))   
    #define TP_MODE_MANUAL  ((uint8_t)(1<<6))
    
    /* Touch Panel debounce Reg TPCR1[2]    */
    #define TP_DEBOUNCE_OFF ((uint8_t)(0<<2))
    #define TP_DEBOUNCE_ON  ((uint8_t)(1<<2))
    
    /* Touch Panel manual modes Reg TPCR1[1:0]  */
    #define TP_MANUAL_IDLE      0
    #define TP_MANUAL_WAIT      1
    #define TP_MANUAL_LATCH_X   2
    #define TP_MANUAL_LATCH_Y   3
    
    /* Touch Panel ADC Clock modes Reg TPCR0[2:0] */
    #define TP_ADC_CLKDIV_1            0
    #define TP_ADC_CLKDIV_2            1        
    #define TP_ADC_CLKDIV_4            2        
    #define TP_ADC_CLKDIV_8            3      
    #define TP_ADC_CLKDIV_16           4        
    #define TP_ADC_CLKDIV_32           5        
    #define TP_ADC_CLKDIV_64           6        
    #define TP_ADC_CLKDIV_128          7
            
    
    /* Touch Panel Sample Time Reg TPCR0[6:4] */
    #define TP_ADC_SAMPLE_512_CLKS     ((uint8_t)(0<<4))
    #define TP_ADC_SAMPLE_1024_CLKS    ((uint8_t)(1<<4))
    #define TP_ADC_SAMPLE_2048_CLKS    ((uint8_t)(2<<4))
    #define TP_ADC_SAMPLE_4096_CLKS    ((uint8_t)(3<<4))
    #define TP_ADC_SAMPLE_8192_CLKS    ((uint8_t)(4<<4))
    #define TP_ADC_SAMPLE_16384_CLKS   ((uint8_t)(5<<4))
    #define TP_ADC_SAMPLE_32768_CLKS   ((uint8_t)(6<<4))
    #define TP_ADC_SAMPLE_65536_CLKS   ((uint8_t)(7<<4))
    
    /* RA8875 interrupt enable/flag/clear masks */
    #define RA8875_INT_KEYSCAN          ((uint8_t)(1<<4))    /**< KEYSCAN interrupts  */
    #define RA8875_INT_DMA              ((uint8_t)(1<<3))    /**< DMA interrupts  */
    #define RA8875_INT_TP               ((uint8_t)(1<<2))    /**< Touch panel interrupts  */
    #define RA8875_INT_BTE              ((uint8_t)(1<<1))    /**< BTE process complete interrupts  */
    #define RA8875_INT_BTEMCU_FONTWR    ((uint8_t)(1<<0))    /**< BTE-MCU-R/W or Font-Write interrupts  */

#endif // RA8875_REGS_H