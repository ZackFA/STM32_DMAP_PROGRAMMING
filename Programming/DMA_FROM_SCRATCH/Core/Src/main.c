#include <stdint.h>
#include "stm32f446xx.h"

void button_init(void);
void UART2_init(void);
void DMA1_init(void);
void sendData(void);
void dma1_it_configuration(void);
void enable_dma_stream(void);


void HT_Complete_callback(void);
void FT_Complete_callback(void);
void TE_error_callback(void);
void DME_error_callback(void);
void FE_error_callback(void);

#define GPIOC_BASEADDRESS		GPIOC
char   datastream[] = "Hello world\n";

int main(void)
{
	button_init();
	UART2_init();
	sendData();
	DMA1_init();
	dma1_it_configuration();
	enable_dma_stream();

	while(1);
	return 0;
}


void button_init(void)
{

	// Button is connected to GPIOC PC13
	GPIO_TypeDef *pGPIOC = GPIOC_BASEADDRESS;
	RCC_TypeDef *pRCC = RCC;
	EXTI_TypeDef *pEXTI = EXTI;
	SYSCFG_TypeDef *pSYSCFG = SYSCFG;

	/* 1. Enable the clock */
	pRCC->AHB1ENR |= (1 << 2); // ENABLE THE BUS
	pRCC->APB2ENR |= (1 << 14);// FOR SYSTEM CONFIG REGISTERS BLOCK


	/* 2. Keep the GPIO in input mode */
	pGPIOC->MODER &= ~(0x3 << 26);


	/* 3. Interrupt enable */
	pEXTI->IMR |= (1 <<13); // Interrupt mask register


	/* 4. Configuring the SYSCFG CR4 Register */
	pSYSCFG->EXTICR[3] &= ~(0xF << 4); // clearing
	pSYSCFG->EXTICR[3] &= ~(0x2 << 4); // setting

	/* 5. Configure the edge detection */
	pEXTI->FTSR |= (1 << 13);


	/* 6. Enable the IRQ related to that GPIO Pin in the NVIC in the processor */
	// Find out the IRQ Number for GPIOC from reference manual to external event line mapping
	// At vector table, search for EXTI13, the IRQ number is 40
	NVIC_EnableIRQ(EXTI15_10_IRQn);

}

void UART2_init(void)
{

	USART_TypeDef *pUART2 = USART2;
	RCC_TypeDef *pRCC = RCC;
	GPIO_TypeDef *pGPIOA = GPIOA;

	/* 1. Enable the clock */
	pRCC->APB1ENR |= (1 << 17);


	/* 2. Configure the GPIO for  UART2 pins */
	// PA2 TX
	// PA3 RX

	/* 2.1 Enable the clock for GPIOA */
	pRCC->APB1ENR |= (1 << 0);
	/* 2.2 Change the mode for PA2 and PA3 */
	pGPIOA->MODER &= ~(0x3 << 4); // PA2
	pGPIOA->MODER |= (0x2 << 4); // PA2
	pGPIOA->AFR[0] &= ~(0xF << 8); // PA2
	pGPIOA->AFR[0] |= (0x7 << 8); // PA2

	pGPIOA->MODER &= ~(0x3 << 6); // PA3
	pGPIOA->MODER |= (0x2 << 6); // PA3
	pGPIOA->AFR[0] &= ~(0xF << 12); // PA3
	pGPIOA->AFR[0] |= (0x7 << 12); // PA3

	/* 2.3 Enable or disable Pullup resistor if required */
	pGPIOA->PUPDR |= (0x1 << 4); // in serial communication pullup resistor is rquired because idle state is high
	pGPIOA->PUPDR |= (0x1 << 6);


	/* 3. Configure the Baudrate */
	pUART2->BRR = 0x8B;

	/* 4. Configure the data width, no of stop bits, etc. */
	/* Data width is by default 8 bits, stop bits is 1, no need to configure */

	/* 5. Enable the TX engine of the peripheral */
	pUART2->CR1 |= (1 << 3);

	/* 6. Enable the UART peripheral */
	pUART2->CR1 |= (1 << 13);


}

void sendData(void)
{
	USART_TypeDef *pUART2 = USART2;
	char data[] = "Hello world\r\n";
	uint32_t length = sizeof(data);

	while( !(pUART2->SR & (1 << 7) ) ); // Check if TXE is 1, then put data

	for(uint32_t i=0;i<length;i++)
	{
		pUART2->DR = data[i];
	}

}

void DMA1_init(void)
{
	DMA_TypeDef *pDMA1 = DMA1;
	DMA_Stream_TypeDef *pStream6 = DMA1_Stream6;
	RCC_TypeDef *pRCC = RCC;
	USART_TypeDef *pUART2 = USART2;

	/* 1. Enable the clock */
	pRCC->AHB1ENR |= (1 << 21);

	/* 2. Identify the stream which is suitable for your peripheral */
	// channel 4, stream 6



	/* 3. Identify the channel number on which UART2 peripheral sends DMA request */
	//channel 4
	pStream6->CR &= ~(0x7 << 25);
	pStream6->CR |=  (0x4 << 25);

	/* 4. Program the source address */
	pStream6->M0AR = (uint32_t ) datastream;


	/* 5. Program the destination address */
	pStream6->PAR = (uint32_t ) &pUART2->DR;

	/* 6. Program number of data items to send */
	uint32_t  length = sizeof(datastream);
	pStream6->NDTR = length;

	/* 7. Direction of the data transfer */
	pStream6->CR |= (0x1 << 6); //M2P direction

	/* 8. Program the source & destination data width */
	pStream6->CR &= ~(0x3 << 13);
	pStream6->CR &= ~(0x3 << 11);

	/* 8.a Memory increment mode */
	pStream6->CR |= (1 << 10);

	/* 9. Select whether its a direct mode or FIFO mode */
	pStream6->FCR |= (1 << 2);

	/* 10. Select the FIFO threshold */
	// FULL FIFO
	pStream6->FCR &= ~(0x3 << 0);
	pStream6->FCR &=  (0x3 << 0);

	/* 11. Enable circular mode if required */
	//default

	/* 12. Single transfer or burst transfer */
	//default

	/* 13. Stream priority */
	// default



}

void enable_dma_stream(void)
{
	DMA_Stream_TypeDef *pStream6 = DMA1_Stream6;

	/* 1. Enable the stream */
	pStream6->CR |= ( 1 << 0);

}

void dma1_it_configuration(void)
{
	DMA_Stream_TypeDef *pStream6 = DMA1_Stream6;

	/* 1. Half Transfer IE */
	pStream6->CR |= (1 << 3);

	/* 2. Transfer Complete IE */
	pStream6->CR |= (1 << 4);

	/* 3. Transfer Error IE */
	pStream6->CR |= (1 << 2);

	/* 4. FIFO Overrun/Underrun IE */
	pStream6->FCR |= (1 << 7);

	/* 5. Direct Mode Error IE */
	pStream6->CR |= (1 << 1);

	NVIC_EnableIRQ(DMA1_Stream6_IRQn);

}

void HT_Complete_callback(void)
{

}

void FT_Complete_callback(void)
{
	USART_TypeDef *pUART2 = USART2;

	DMA_Stream_TypeDef *pSTREAM6 = DMA1_Stream6;

	//Program number of data items to send
	uint32_t len = sizeof(datastream);
	pSTREAM6->NDTR = len;

	pUART2->CR3 &= ~( 1 << 7);

	enable_dma_stream();

}


void TE_error_callback(void)
{
	while(1);
}

void FE_error_callback(void)
{

	while(1);
}

void DME_error_callback(void)
{
	while(1);
}
