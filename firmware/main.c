#include "stm8l15x.h"
#define TIM_MMS_ENABLE 0x10
#define TIM_MMS_UPDATE 0x20
#define TIM_TS_ITR1 0x10
#define TIM_TS_ITR2 0x20
#define TIM_TS_ITR3 0x30
#define TIM_SMS_RESET 0x04
#define TIM_SMS_GATED 0x05
#define TIM_SMS_GATED 0x05
#define TIM_SMS_TRIGGER 0x06
#define TIM_SMS_EXTERNAL 0x07
#define v2dac(x) (int)(x/3.3*4096)


float sin[] = {0.000,0.309,0.588,0.809,0.951,
			1.000,0.951,0.809,0.588,0.309,
			0.000,-0.309,-0.588,-0.809,-0.951,
			-1.000,-0.951,-0.809,-0.588,-0.309}; //sinus tabulated;
unsigned int dac_signal[20];  // DMA >> DAC
void set_dac_signal(unsigned int offset, unsigned int amplitude)
{
	int i;
	for(i = 0; i <20; i++)
	{
		dac_signal[i] = (unsigned int)(sin[i]*amplitude + offset);
	}
	return;
}
void main( void )
{
	// CLK
	CLK->CKDIVR = 0;
	CLK->PCKENR1 = CLK_PCKENR1_DAC | CLK_PCKENR1_USART1 | CLK_PCKENR1_TIM2 | CLK_PCKENR1_TIM4;
	CLK->PCKENR2 = CLK_PCKENR2_DMA1 | CLK_PCKENR2_ADC1 | CLK_PCKENR2_COMP;
	// DMA ch3(DAC)
	DMA1_Channel3->CCR = DMA_CCR_TCIE |
                       DMA_CCR_HTIE |
                       DMA_CCR_DTD |
                       DMA_CCR_ARM |
                       DMA_CCR_IDM;
	DMA1_Channel3->CSPR = DMA_CSPR_16BM| DMA_CSPR_PL; //max priority
	DMA1_Channel3->CPARH = ((unsigned int)(&(DAC->CH1RDHRH))) >>8;
	DMA1_Channel3->CPARL = (unsigned char)(&(DAC->CH1RDHRH));

	DMA1_Channel3->CNBTR = 20;
	DMA1_Channel3->CM0EAR = 0;
	DMA1_Channel3->CM0ARH = ((unsigned int)dac_signal)>>8;
	DMA1_Channel3->CM0ARL = ((unsigned int)dac_signal)%0x100;
	DMA1_Channel3->CCR |= DMA_CCR_CE;
	// DMA Fire
	DMA1->GCSR = DMA_GCSR_GE;

	// DAC is driven from TIM4 and uses DMA ch3
	set_dac_signal(v2dac(0.9), v2dac(0.05));  //set output
	DAC->CH1CR1 = DAC_CR1_TEN | DAC_CR1_EN;
	DAC->CH1CR2 = DAC_CR2_DMAEN;
	RI->IOSR3 = 0x10; // CH14(Port B4) Analog switch closed;
	//RI->IOCMR2 = 1 << 4;
	// TIM4 is driven from TIM2 and clocks DAC
	TIM4->CR1 = TIM4_CR1_OPM;
	TIM4->CR2 = TIM_MMS_UPDATE;
	TIM4->SMCR = TIM_TS_ITR3 | TIM_SMS_TRIGGER;
	TIM4->DER = 0;
	TIM4->IER = 0;
	TIM4->CNTR = 0;
	TIM4->ARR = 2;
	TIM4->PSCR = 0;
	// TIM2 is a clock source for ADC and DAC (through TIM4)
	TIM2->CR2 = TIM_MMS_UPDATE;
	TIM2->SMCR = 0;
	TIM2->DER = 0;
	TIM2->IER = 0;
	TIM2->PSCR = 4; // 1 usec @ 16 MHz
	TIM2->ARRH = 0x7;
	TIM2->ARRL = 0xD0; //2000 / 500Hz
	TIM2->CR1 = TIM_CR1_CEN;

	while(1);
}
