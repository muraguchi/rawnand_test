#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#include "rawNand.h"

#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

#define UART_TX_PIN 16
#define UART_RX_PIN 17 

#define FSM_IDLE      0
#define FSM_WPB_S1    1
#define FSM_WPB_S2    2




static uint8_t uart_rx_buff[128];
static uint8_t command_buff[128];
static uint8_t uart_tx_buff[128];
static volatile unsigned int  uart_rx_wr_ptr;
static volatile unsigned int  uart_rx_rd_ptr;


void uart_ring_init();
int  uart_ring_rx_pop(uint8_t * rxdata);
void uart_ring_rx_push(uint8_t rxdata);

//void rawnand_init();

rawNand rn;



uint8_t rawnand_statusRead();
void rawnand_idRead(uint8_t * rxd);
void rawnand_reset();

void uart_ring_init(){
  uart_rx_wr_ptr = 0;
  uart_rx_rd_ptr = 0;
}
int uart_ring_rx_pop(uint8_t * rxdata){
  if(uart_rx_rd_ptr==uart_rx_wr_ptr){
    return 0;
  } else {
    * rxdata = uart_rx_buff[uart_rx_rd_ptr];
    uart_rx_rd_ptr = 0x7f & (uart_rx_rd_ptr + 1);
    return 1;
  }  
}
void uart_ring_rx_push(uint8_t rxdata){
  unsigned int new_rx_wr_ptr;
  new_rx_wr_ptr = 0x7f & (uart_rx_wr_ptr+1);
  // buffer full, ignore read data
  if(new_rx_wr_ptr==uart_rx_rd_ptr){
    return;
  } else {
    uart_rx_buff[uart_rx_wr_ptr]=rxdata;
    uart_rx_wr_ptr= new_rx_wr_ptr;
  }
}

void rawnand_reset(){
	rn.reset(0);
}

uint8_t rawnand_statusRead(){
  return rn.statusRead(0);
}

void rawnand_idRead(uint8_t * rxd){
  rn.idRead(0,rxd);
}

// RX interrupt handler
void on_uart_rx() {
  uint8_t rx;
  while (uart_is_readable(UART_ID)) {
    rx=uart_getc(UART_ID);
    uart_ring_rx_push( rx );
  }
}

int main() {
  // Set up our UART with a basic baud rate.
  uart_init(UART_ID, 2400);
  uart_ring_init();
  // rawnand_init();
  
  // Set the TX and RX pins by using the function select on the GPIO
  // Set datasheet for more information on function select
  gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
  gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
  
  // Actually, we want a different speed
  // The call will return the actual baud rate selected, which will be as close as
  // possible to that requested
  int __unused actual = uart_set_baudrate(UART_ID, BAUD_RATE);
  
  // Set UART flow control CTS/RTS, we don't want these, so turn them off
  uart_set_hw_flow(UART_ID, false, false);
  
  // Set our data format
  uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
  
  // Turn off FIFO's - we want to do this character by character
  uart_set_fifo_enabled(UART_ID, false);
  
  // Set up a RX interrupt
  // We need to set up the handler first
  // Select correct interrupt for the UART we are using
  int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;
  
  // And set up and enable the interrupt handlers
  irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
  irq_set_enabled(UART_IRQ, true);
  
  // Now enable the UART to send interrupts - RX only
  uart_set_irq_enables(UART_ID, true, false);
  
  // OK, all set up.
  // Lets send a basic string out, and then run a loop and wait for RX interrupts
  // The handler will count them, but also reflect the incoming data back with a slight change!
  uart_puts(UART_ID, "Raw NAND test\r\n");
  
  uint8_t rxd;
  //uint8_t state=FSM_IDLE;
  
  uint8_t cmd_len=0;
  
  while (1) {
    if (uart_ring_rx_pop(&rxd)==1){
      // echo back	
      uart_putc(UART_ID,rxd);
      
      if (rxd=='\n' || rxd=='\r' || cmd_len==127) {
	// terminate command buffer
	command_buff[cmd_len]='\0';

	
	// command int
	
	// reset command
	if (strlen((const char *)command_buff)>=5 && memcmp("reset",command_buff,5)==0 ) {
	  unsigned int cs=0;
	  sscanf((const char *)command_buff,"reset %u" ,&cs);
	  rn.reset(cs);
	  sprintf((char *)uart_tx_buff,"reset issued at cs%d\r\n",cs);
	  uart_puts(UART_ID,(const char*)uart_tx_buff);
	} else if ( strlen( (const char *)command_buff)>=2 && memcmp("id",command_buff,2)==0  ){
	  unsigned int cs=0;
	  uint8_t idb[5];
	  sscanf((const char *)command_buff,"id %u" ,&cs);
	  rn.idRead(cs,idb);
	  sprintf((char *)uart_tx_buff,"id read issued at cs%d\r\n",cs);
	  uart_puts(UART_ID,(const char*)uart_tx_buff);
	  sprintf((char *)uart_tx_buff,"id: %02X %02X %02X %02X %02X\r\n",idb[0],idb[1],idb[2],idb[3],idb[4]);
	  uart_puts(UART_ID,(const char*)uart_tx_buff);
	} else if ( strlen( (const char *)command_buff)>=4 && memcmp("read",command_buff,4)==0 ){
	  unsigned int cs=0;
	  unsigned int ba=0;
	  unsigned int pa=0;
	  unsigned int ca=0;
	  unsigned int bs=512;
	  uint8_t rb[2048+128];
	  sscanf((const char *)command_buff,"read %u %u %u %u %u" ,&cs,&ba,&pa,&ca,&bs);
	  rn.pageRead(cs,rb,ba,pa,ca,bs);
	  sprintf((char *)uart_tx_buff,"read issued at cs%d ba%d pa%d ca%d bs%d\r\n",cs,ba,pa,ca,bs);
	  uart_puts(UART_ID,(const char*)uart_tx_buff);
	  for(int l=ca;l<ca+bs;l=l+16) {
	    sprintf((char *)uart_tx_buff,"0x%03X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\r\n",l,
		    rb[l+0] ,rb[l+1] ,rb[l+2] ,rb[l+3],
		    rb[l+4] ,rb[l+5] ,rb[l+6] ,rb[l+7],
		    rb[l+8] ,rb[l+9] ,rb[l+10],rb[l+11],
		    rb[l+12],rb[l+13],rb[l+14],rb[l+15]);
	    uart_puts(UART_ID,(const char*)uart_tx_buff);
	  }
	}
	// TODO : add page program and erase for testing .... 
	// else if ...
	// else if ...	
	cmd_len=0;	
      } else {
	command_buff[cmd_len]=rxd;
	cmd_len++;
      }
    }
  }
}


