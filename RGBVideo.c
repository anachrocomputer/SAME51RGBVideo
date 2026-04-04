/* RGBVideo --- generate RGB video on the ATSAME51J20A      2026-04-03 */


#include <sam.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


volatile uint32_t FieldCounter = 0UL;
volatile bool Tick = false;


/* TC3_Handler --- ISR for 50Hz frame timer interrupt */

void TC3_Handler(void)
{
    int line;
    int i;
    
    // Acknowledge interrupt
    TC3_REGS->COUNT16.TC_INTFLAG |= TC_INTFLAG_MC0(1);
    
    FieldCounter++;
    Tick = true;
    
    PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA00;    // Frame sync HIGH
    
    for (line = 0; line < 290; line++)
    {
        PORT_REGS->GROUP[0].PORT_OUTCLR = PORT_PA01;    // Line sync LOW
        
        for (i = 0; i < 23; i++)
        {
            __asm("nop");
            __asm("nop");
            __asm("nop");
            __asm("nop");
        }
        
        PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA01;    // Line sync HIGH
        
        for (i = 0; i < 293; i++)
        {
            __asm("nop");
            __asm("nop");
            __asm("nop");
            __asm("nop");
        }
        
        __asm("nop");
    }
    
    PORT_REGS->GROUP[0].PORT_OUTCLR = PORT_PA00;    // Frame sync LOW
}


/* t1ou --- send a byte to UART0 by polling */

void t1ou(const int ch)
{
    while ((SERCOM0_REGS->USART_INT.SERCOM_INTFLAG & SERCOM_USART_INT_INTFLAG_DRE(1)) == 0)
        ;
    
    SERCOM0_REGS->USART_INT.SERCOM_DATA = ch;
}


/* _mon_putc --- connect UART0 to 'stdout' */

void _mon_putc(const char ch)
{
    if (ch == '\n')
    {
        t1ou('\r');
    }
    
    t1ou(ch);
}


/* fieldCounter --- return field count since reset */

uint32_t fieldCounter(void)
{
    return (FieldCounter);
}


/* analogRead --- Arduino-like function to start a conversion, wait, and return result */

uint16_t analogRead(const int channel)
{
    ADC0_REGS->ADC_INPUTCTRL = ADC_INPUTCTRL_MUXPOS(channel);   // Select ADC input channel
    
    ADC0_REGS->ADC_SWTRIG |= ADC_SWTRIG_START(1);   // Trigger ADC conversion
    
    while (ADC_SYNCBUSY_SWTRIG_Msk == (ADC0_REGS->ADC_SYNCBUSY & ADC_SYNCBUSY_SWTRIG_Msk))
    {
        ;   // Wait for sync
    }
    
    while ((ADC0_REGS->ADC_INTFLAG & ADC_INTFLAG_RESRDY_Msk) == 0)
    {
        ;   // Wait for conversion complete
    }
    
    return (uint16_t)(ADC0_REGS->ADC_RESULT & ADC_RESULT_RESULT_Msk);
}


#define ADC0_BIASCOMP_POS     (2u)
#define ADC0_BIASCOMP_Msk     (0x7u << ADC0_BIASCOMP_POS)

#define ADC0_BIASREFBUF_POS   (5u)
#define ADC0_BIASREFBUF_Msk   (0x7u << ADC0_BIASREFBUF_POS)

#define ADC0_BIASR2R_POS      (8u)
#define ADC0_BIASR2R_Msk      (0x7u << ADC0_BIASR2R_POS)


/* initADC --- set up one of the two ADCs */

static void initADC(void)
{
    // Connect GCLK1 to ADC0
    GCLK_REGS->GCLK_PCHCTRL[40] = GCLK_PCHCTRL_GEN(0x1U) | GCLK_PCHCTRL_CHEN_Msk;

    while ((GCLK_REGS->GCLK_PCHCTRL[40] & GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk)
    {
        ; // Wait for sync
    }
    
    // Main Clock setup to supply clock to ADC0
    MCLK_REGS->MCLK_APBDMASK |= MCLK_APBDMASK_ADC0(1);
    
    // Reset ADC0
    ADC0_REGS->ADC_CTRLA |= (1u << ADC_CTRLA_SWRST_Pos);

    while (ADC_SYNCBUSY_SWRST_Msk == (ADC0_REGS->ADC_SYNCBUSY & ADC_SYNCBUSY_SWRST_Msk))
    {
        ; // Wait for sync
    }
    
    // Set up the pin mux for PA06 and PA07 as analog inputs
    PORT_REGS->GROUP[0].PORT_PMUX[3] = PORT_PMUX_PMUXE_B | PORT_PMUX_PMUXO_B;
    PORT_REGS->GROUP[0].PORT_PINCFG[6] = PORT_PINCFG_PMUXEN(1);  // PA06 AIN6 pin 15
    PORT_REGS->GROUP[0].PORT_PINCFG[7] = PORT_PINCFG_PMUXEN(1);  // PA07 AIN7 pin 16
    
    PORT_REGS->GROUP[0].PORT_DIRCLR = PORT_PA06;    // PA06 AIN6 is an input pin
    PORT_REGS->GROUP[0].PORT_DIRCLR = PORT_PA07;    // PA07 AIN7 is an input pin
        
    // Load factory cal values for ADC0
    ADC0_REGS->ADC_CALIB = (uint16_t)((ADC_CALIB_BIASCOMP((((*(uint32_t*)SW0_ADDR) & ADC0_BIASCOMP_Msk) >> ADC0_BIASCOMP_POS)))
                           | ADC_CALIB_BIASR2R((((*(uint32_t*)SW0_ADDR) & ADC0_BIASR2R_Msk) >> ADC0_BIASR2R_POS))
                           | ADC_CALIB_BIASREFBUF(((*(uint32_t*)SW0_ADDR) & ADC0_BIASREFBUF_Msk)>> ADC0_BIASREFBUF_POS ));
    
    ADC0_REGS->ADC_CTRLA |= (2u << ADC_CTRLA_PRESCALER_Pos);       // Set up prescaler
    
    ADC0_REGS->ADC_CTRLB = ADC_CTRLB_RESSEL_12BIT;                 // Select 12 bit mode
    
    ADC0_REGS->ADC_INPUTCTRL = (0u << ADC_INPUTCTRL_MUXPOS_Pos)    // Positive MUX Input Selection
                             | (0u << ADC_INPUTCTRL_DIFFMODE_Pos)  // Non-differential Mode
                             | (24u << ADC_INPUTCTRL_MUXNEG_Pos)   // Negative MUX Input Selection
                             | (0u << ADC_INPUTCTRL_DSEQSTOP_Pos); // Stop DMA Sequencing
        
    ADC0_REGS->ADC_REFCTRL = (3u << ADC_REFCTRL_REFSEL_Pos)        // Select VddAna reference
                           | (0u << ADC_REFCTRL_REFCOMP_Pos);      // Enable Reference Buffer Offset Compensation
    
    ADC0_REGS->ADC_SAMPCTRL = (5u << ADC_SAMPCTRL_SAMPLEN_Pos)     // 5 cycles sampling time
                            | (0u << ADC_SAMPCTRL_OFFCOMP_Pos);    // Enable Comparator Offset Compensation
    
    while(ADC_SYNCBUSY_Msk == (ADC0_REGS->ADC_SYNCBUSY & ADC_SYNCBUSY_Msk))
    {
        ; // Wait for sync
    }
    
    //while (ADC0_REGS->ADC_SYNCBUSY & ADC_SYNCBUSY_ENABLE(1))
    //    ;
    
    // Finally, enable ADC0
    ADC0_REGS->ADC_CTRLA |= (1u << ADC_CTRLA_ENABLE_Pos);
    
    while (ADC_SYNCBUSY_ENABLE_Msk == (ADC0_REGS->ADC_SYNCBUSY & ADC_SYNCBUSY_ENABLE_Msk))
    {
        ; // Wait for sync
    }
}


/* initDAC --- set up the dual 12-bit Digital-to-Analog Converter */

static void initDAC(void)
{
    /// Connect GCLK1 to DAC
    GCLK_REGS->GCLK_PCHCTRL[42] = GCLK_PCHCTRL_GEN(0x1U) | GCLK_PCHCTRL_CHEN_Msk;

    while ((GCLK_REGS->GCLK_PCHCTRL[42] & GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk)
        ;
    
    // Main Clock setup to supply clock to DAC
    MCLK_REGS->MCLK_APBDMASK |= MCLK_APBDMASK_DAC(1);
    
    // Set up I/O pins
    PORT_REGS->GROUP[0].PORT_PMUX[1] = PORT_PMUX_PMUXE_B;
    PORT_REGS->GROUP[0].PORT_PMUX[2] = PORT_PMUX_PMUXO_B;
    PORT_REGS->GROUP[0].PORT_PINCFG[2] = PORT_PINCFG_PMUXEN(1);  // Vout0 on PA02 pin 3
    PORT_REGS->GROUP[0].PORT_PINCFG[5] = PORT_PINCFG_PMUXEN(1);  // Vout1 on PA05 pin 14
    
    // Set up DAC registers
    DAC_REGS->DAC_CTRLA = 0;
    DAC_REGS->DAC_CTRLB = DAC_CTRLB_REFSEL_VDDANA | DAC_CTRLB_DIFF(0);
    DAC_REGS->DAC_DACCTRL[0] = DAC_DACCTRL_DITHER(0) | DAC_DACCTRL_ENABLE(1);
    DAC_REGS->DAC_DACCTRL[1] = DAC_DACCTRL_DITHER(0) | DAC_DACCTRL_ENABLE(1);
    DAC_REGS->DAC_DATA[0] = 0;
    DAC_REGS->DAC_DATA[1] = 0;
    
    DAC_REGS->DAC_CTRLA |= DAC_CTRLA_ENABLE(1);
}


/* initUARTs --- set up UART(s) and buffers */

static void initUARTs(void)
{
    // Connect GCLK1 to SERCOM0
    GCLK_REGS->GCLK_PCHCTRL[7] = GCLK_PCHCTRL_GEN(0x1U) | GCLK_PCHCTRL_CHEN_Msk;

    while ((GCLK_REGS->GCLK_PCHCTRL[7] & GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk)
        ;
    
    // Main Clock setup to supply clock to SERCOM0
    MCLK_REGS->MCLK_APBAMASK |= MCLK_APBAMASK_SERCOM0(1);

    // Set up SERCOM0 as UART0
    SERCOM0_REGS->USART_INT.SERCOM_CTRLA = SERCOM_USART_INT_CTRLA_TXPO_PAD0 |
                                           SERCOM_USART_INT_CTRLA_RXPO_PAD1 |
                                           SERCOM_USART_INT_CTRLA_FORM_USART_FRAME_NO_PARITY |
                                           SERCOM_USART_INT_CTRLA_DORD_LSB |
                                           SERCOM_USART_INT_CTRLA_CMODE_ASYNC |
                                           SERCOM_USART_INT_CTRLA_SAMPR_16X_ARITHMETIC |
                                           SERCOM_USART_INT_CTRLA_MODE_USART_INT_CLK;
    
    while (SERCOM0_REGS->USART_INT.SERCOM_SYNCBUSY & SERCOM_USART_INT_SYNCBUSY_CTRLB(1))
        ;
    
    SERCOM0_REGS->USART_INT.SERCOM_CTRLB = SERCOM_USART_INT_CTRLB_CHSIZE_8_BIT |
                                           SERCOM_USART_INT_CTRLB_SBMODE_1_BIT |
                                           SERCOM_USART_INT_CTRLB_TXEN(1) |
                                           SERCOM_USART_INT_CTRLB_RXEN(1);
    while (SERCOM0_REGS->USART_INT.SERCOM_SYNCBUSY & SERCOM_USART_INT_SYNCBUSY_CTRLB(1))
        ;
    
    SERCOM0_REGS->USART_INT.SERCOM_BAUD = 65326;    // 9600 baud from 48MHz GCLK
    //SERCOM0_REGS->USART_INT.SERCOM_INTENSET = 0;

    PORT_REGS->GROUP[0].PORT_PMUX[4] = PORT_PMUX_PMUXE_C | PORT_PMUX_PMUXO_C;
    PORT_REGS->GROUP[0].PORT_PINCFG[8] = PORT_PINCFG_PMUXEN(1);  // PA08 pin 17 TxD
    PORT_REGS->GROUP[0].PORT_PINCFG[9] = PORT_PINCFG_PMUXEN(1);  // PA09 pin 18 RxD
    
    while (SERCOM0_REGS->USART_INT.SERCOM_SYNCBUSY & SERCOM_USART_INT_SYNCBUSY_ENABLE(1))
        ;
    
    SERCOM0_REGS->USART_INT.SERCOM_CTRLA |= SERCOM_USART_INT_CTRLA_ENABLE(1);
    
    while (SERCOM0_REGS->USART_INT.SERCOM_SYNCBUSY & SERCOM_USART_INT_SYNCBUSY_ENABLE(1))
        ;
}


/* init50HzTimer --- set up a timer to interrupt every field */

static void init50HzTimer(void)
{
    // Connect GCLK1 to TC3
    GCLK_REGS->GCLK_PCHCTRL[26] = GCLK_PCHCTRL_GEN(0x1U) | GCLK_PCHCTRL_CHEN_Msk;

    while ((GCLK_REGS->GCLK_PCHCTRL[26] & GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk)
        ;
    
    // Main Clock setup to supply clock to TC3
    MCLK_REGS->MCLK_APBBMASK |= MCLK_APBBMASK_TC3(1);
    
    // Set up TC3 for regular 20ms/50Hz interrupt
    TC3_REGS->COUNT16.TC_CTRLA = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV64;
    TC3_REGS->COUNT16.TC_COUNT = 0;
    TC3_REGS->COUNT16.TC_WAVE = TC_WAVE_WAVEGEN_MFRQ;
    TC3_REGS->COUNT16.TC_CC[0] = ((48000000 / 64) / 50) - 1;
    TC3_REGS->COUNT16.TC_INTENSET = TC_INTENSET_MC0(1);
    
    /* Set TC3 Interrupt Priority to Level 3 */
    NVIC_SetPriority(TC3_IRQn, 3);
    
    /* Enable TC3 NVIC Interrupt Line */
    NVIC_EnableIRQ(TC3_IRQn);
    
    TC3_REGS->COUNT16.TC_CTRLA |= TC_CTRLA_ENABLE(1);
    
    // Wait until TC3 is enabled
    //while(TC3->COUNT16.STATUS.bit.SYNCBUSY == 1)
    //    ;
}


/* initMCU --- initialise the microcontroller in general */

static void initMCU(void)
{
    // Set up wait states
    NVMCTRL_REGS->NVMCTRL_CTRLA = NVMCTRL_CTRLA_RWS(0) | NVMCTRL_CTRLA_AUTOWS_Msk;
    
    /* GCLK2 initialisation: divide DFLL48 by 48 to get 1MHz */
    GCLK_REGS->GCLK_GENCTRL[2] = GCLK_GENCTRL_DIV(48) | GCLK_GENCTRL_SRC_DFLL | GCLK_GENCTRL_GENEN_Msk;

    while((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL_GCLK2) == GCLK_SYNCBUSY_GENCTRL_GCLK2)
    {
        /* Wait for the Generator 2 synchronisation */
    }
    
    /* Connect GCLK to GCLK2 (1MHz) */
    GCLK_REGS->GCLK_PCHCTRL[1] = GCLK_PCHCTRL_GEN(0x2)  | GCLK_PCHCTRL_CHEN_Msk;
    while ((GCLK_REGS->GCLK_PCHCTRL[1] & GCLK_PCHCTRL_CHEN_Msk) != GCLK_PCHCTRL_CHEN_Msk)
    {
        /* Wait for synchronisation */
    }
    
    /* Configure DPLL    */
    // FILTER = 0
    // LTIME = 0    No time-out
    // REFCLK = 0   GCLK (1MHz)
    // DIV = 0
    OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLCTRLB = OSCCTRL_DPLLCTRLB_FILTER(0) | OSCCTRL_DPLLCTRLB_LTIME(0x0)| OSCCTRL_DPLLCTRLB_REFCLK(0);

    // LDRFRAC = 0
    // LDR = 119    Divide-by-120
    OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLRATIO = OSCCTRL_DPLLRATIO_LDRFRAC(0) | OSCCTRL_DPLLRATIO_LDR(119);

    while((OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLSYNCBUSY & OSCCTRL_DPLLSYNCBUSY_DPLLRATIO_Msk) == OSCCTRL_DPLLSYNCBUSY_DPLLRATIO_Msk)
    {
        /* Wait for synchronisation */
    }
    
    /* Enable DPLL */
    OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLCTRLA = OSCCTRL_DPLLCTRLA_ENABLE_Msk;

    while((OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLSYNCBUSY & OSCCTRL_DPLLSYNCBUSY_ENABLE_Msk) == OSCCTRL_DPLLSYNCBUSY_ENABLE_Msk)
    {
        /* Wait for DPLL enable synchronisation */
    }

    while((OSCCTRL_REGS->DPLL[0].OSCCTRL_DPLLSTATUS & (OSCCTRL_DPLLSTATUS_LOCK_Msk | OSCCTRL_DPLLSTATUS_CLKRDY_Msk)) !=
                (OSCCTRL_DPLLSTATUS_LOCK_Msk | OSCCTRL_DPLLSTATUS_CLKRDY_Msk))
    {
        /* Wait for Ready state */
    }
    
    /* Initialise CPU clock */
    // CPUDIV = 1           Divide-by-1
    MCLK_REGS->MCLK_CPUDIV = MCLK_CPUDIV_DIV(0x01);

    while((MCLK_REGS->MCLK_INTFLAG & MCLK_INTFLAG_CKRDY_Msk) != MCLK_INTFLAG_CKRDY_Msk)
    {
        /* Wait for Main Clock to be Ready */
    }
    
    /* Initialise GCLK0 */
    // GENCTRL0.DIV = 1     Divide-by-1
    // GENCTRL0.DIVSEL = 0  Divide-by-GENTRL0.DIV
    // GENCTRL0.SRC = 1     DPLL0 (120MHz)
    // GENCTRL0.GENEN = 1   Enabled
    GCLK_REGS->GCLK_GENCTRL[0] = GCLK_GENCTRL_DIV(1) | GCLK_GENCTRL_SRC_DPLL0 | GCLK_GENCTRL_GENEN_Msk;

    while((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL_GCLK0) == GCLK_SYNCBUSY_GENCTRL_GCLK0)
    {
        /* Wait for Generator 0 synchronisation */
    }
    
    /* GCLK1 initialisation DFLL (48MHz) divide-by-1 */
    // GENCTRL1.DIV = 1     Divide-by-1
    // GENCTRL1.DIVSEL = 0  Divide-by-GENTRL1.DIV
    // GENCTRL1.SRC = 1     DFLL
    // GENCTRL1.GENEN = 1   Enabled
    GCLK_REGS->GCLK_GENCTRL[1] = GCLK_GENCTRL_DIV(1) | GCLK_GENCTRL_SRC_DFLL | GCLK_GENCTRL_GENEN_Msk;

    while((GCLK_REGS->GCLK_SYNCBUSY & GCLK_SYNCBUSY_GENCTRL_GCLK1) == GCLK_SYNCBUSY_GENCTRL_GCLK1)
    {
        /* Wait for Generator 1 synchronisation */
    }
}


void main(void)
{
    initMCU();
    initUARTs();
    //initADC();
    //initDAC();
    init50HzTimer();
    
    PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA00;    // GPIO pins to outputs
    PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA01;
    
    PORT_REGS->GROUP[1].PORT_DIRSET = PORT_PB31;
    PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA27;
    PORT_REGS->GROUP[1].PORT_DIRSET = PORT_PB23;
    PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA25;
    PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA20;
    PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA18;
    PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA15;
    
    PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA00;    // Frame sync HIGH
    PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA01;    // Line sync HIGH
    
    while (1)
    {
        if (Tick)
        {
            Tick = 0;
            t1ou('F');
        }
    }
}

