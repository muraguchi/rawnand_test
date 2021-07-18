#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

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


#define NANDIF_IO1_PIN   0
#define NANDIF_IO2_PIN   1
#define	NANDIF_IO3_PIN   2
#define	NANDIF_IO4_PIN   3
#define	NANDIF_IO5_PIN   4
#define	NANDIF_IO6_PIN   5
#define NANDIF_IO7_PIN   6
#define NANDIF_IO8_PIN   7
#define NANDIF_CEB0_PIN  8
#define NANDIF_CEB1_PIN  9
#define NANDIF_CLE_PIN  10
#define NANDIF_ALE_PIN  11
#define NANDIF_WPB_PIN  12
#define NANDIF_WEB_PIN  13
#define	NANDIF_REB_PIN	14
#define NANDIF_RBB_PIN  15



static uint8_t uart_rx_buff[64];
static uint8_t uart_tx_buff[64];
static volatile unsigned int  uart_rx_wr_ptr;
static volatile unsigned int  uart_rx_rd_ptr;


void uart_ring_init();
int  uart_ring_rx_pop(uint8_t * rxdata);
void uart_ring_rx_push(uint8_t rxdata);

void rawnand_init();
uint8_t rawnand_statusRead();
void rawnand_idRead(uint8_t * rxd);

void uart_ring_init(){
  uart_rx_wr_ptr = 0;
  uart_rx_rd_ptr = 0;
}
int uart_ring_rx_pop(uint8_t * rxdata){
  if(uart_rx_rd_ptr==uart_rx_wr_ptr){
    return 0;
  } else {
    * rxdata = uart_rx_buff[uart_rx_rd_ptr];
    uart_rx_rd_ptr = 0x3f & (uart_rx_rd_ptr + 1);
    return 1;
  }  
}
void uart_ring_rx_push(uint8_t rxdata){
  int new_rx_wr_ptr;
  new_rx_wr_ptr = 0x3f & (uart_rx_wr_ptr+1);
  // buffer full, ignore read data
  if(new_rx_wr_ptr==uart_rx_rd_ptr){
    return;
  } else {
    uart_rx_buff[uart_rx_wr_ptr]=rxdata;
    uart_rx_wr_ptr= new_rx_wr_ptr;
  }
}

void rawnand_init(){
  gpio_init_mask( 1 << NANDIF_IO1_PIN  |         \
		  1 << NANDIF_IO2_PIN  |	 \
		  1 << NANDIF_IO3_PIN  |	 \
		  1 << NANDIF_IO4_PIN  |	 \
		  1 << NANDIF_IO5_PIN  |	 \
		  1 << NANDIF_IO6_PIN  |	 \
		  1 << NANDIF_IO7_PIN  |	 \
		  1 << NANDIF_IO8_PIN  |	 \
		  1 << NANDIF_CEB0_PIN |	 \
		  1 << NANDIF_CEB1_PIN |	 \
		  1 << NANDIF_ALE_PIN  |	 \
		  1 << NANDIF_CLE_PIN  |	 \
		  1 << NANDIF_WPB_PIN  |	 \
		  1 << NANDIF_WEB_PIN  |	 \
		  1 << NANDIF_REB_PIN  |	 \
		  1 << NANDIF_RBB_PIN  );
  gpio_put_masked(1 << NANDIF_IO1_PIN  |	 \
		  1 << NANDIF_IO2_PIN  |	 \
		  1 << NANDIF_IO3_PIN  |	 \
		  1 << NANDIF_IO4_PIN  |	 \
		  1 << NANDIF_IO5_PIN  |	 \
		  1 << NANDIF_IO6_PIN  |	 \
		  1 << NANDIF_IO7_PIN  |	 \
		  1 << NANDIF_IO8_PIN  |	 \
		  1 << NANDIF_CEB0_PIN |	 \
		  1 << NANDIF_CEB1_PIN |	 \
		  1 << NANDIF_ALE_PIN  |	 \
		  1 << NANDIF_CLE_PIN  |	 \
		  1 << NANDIF_WPB_PIN  |	 \
		  1 << NANDIF_WEB_PIN  |	 \
		  1 << NANDIF_REB_PIN  |	 \
		  1 << NANDIF_RBB_PIN,    	 \
		  1 << NANDIF_IO1_PIN  |	 \
		  1 << NANDIF_IO2_PIN  |	 \
                  1 << NANDIF_IO3_PIN  |         \
                  1 << NANDIF_IO4_PIN  |         \
                  1 << NANDIF_IO5_PIN  |         \
                  1 << NANDIF_IO6_PIN  |         \
		  1 << NANDIF_IO7_PIN  |	 \
		  1 << NANDIF_IO8_PIN  |	 \
		  1 << NANDIF_CEB0_PIN |	 \
		  1 << NANDIF_CEB1_PIN |	 \
		  1 << NANDIF_ALE_PIN  |	 \
		  1 << NANDIF_CLE_PIN  |	 \
		  0 << NANDIF_WPB_PIN  |	 \
		  1 << NANDIF_WEB_PIN  |	 \
		  1 << NANDIF_REB_PIN  |	 \
		  0 << NANDIF_RBB_PIN  );
  gpio_set_dir_masked(1 << NANDIF_IO1_PIN  |         \
		      1 << NANDIF_IO2_PIN  |         \
		      1 << NANDIF_IO3_PIN  |         \
		      1 << NANDIF_IO4_PIN  |         \
		      1 << NANDIF_IO5_PIN  |         \
		      1 << NANDIF_IO6_PIN  |         \
		      1 << NANDIF_IO7_PIN  |         \
		      1 << NANDIF_IO8_PIN  |         \
		      1 << NANDIF_CEB0_PIN |         \
		      1 << NANDIF_CEB1_PIN |         \
		      1 << NANDIF_ALE_PIN  |         \
		      1 << NANDIF_CLE_PIN  |         \
		      1 << NANDIF_WPB_PIN  |         \
		      1 << NANDIF_WEB_PIN  |         \
		      1 << NANDIF_REB_PIN  |         \
		      1 << NANDIF_RBB_PIN,           \
		      0 << NANDIF_IO1_PIN  |         \
                      0 << NANDIF_IO2_PIN  |         \
                      0 << NANDIF_IO3_PIN  |         \
                      0 << NANDIF_IO4_PIN  |         \
                      0 << NANDIF_IO5_PIN  |         \
                      0 << NANDIF_IO6_PIN  |         \
                      0 << NANDIF_IO7_PIN  |         \
                      0 << NANDIF_IO8_PIN  |         \
                      1 << NANDIF_CEB0_PIN |         \
                      1 << NANDIF_CEB1_PIN |         \
                      1 << NANDIF_ALE_PIN  |         \
                      1 << NANDIF_CLE_PIN  |         \
                      1 << NANDIF_WPB_PIN  |         \
                      1 << NANDIF_WEB_PIN  |         \
                      1 << NANDIF_REB_PIN  |         \
                      0 << NANDIF_RBB_PIN  );
  gpio_pull_up(NANDIF_RBB_PIN);
}

uint8_t rawnand_statusRead(){
  uint8_t status;
  while ( gpio_get(NANDIF_RBB_PIN)==false ) {
    uart_puts(UART_ID, "Waiting.\r\n");
  }

  gpio_put(NANDIF_CEB0_PIN,0);
  gpio_put(NANDIF_CLE_PIN,1);
  gpio_put(NANDIF_ALE_PIN,0);
  gpio_put(NANDIF_WEB_PIN,0);
  gpio_put(NANDIF_REB_PIN,1);
  
  // IO output
  gpio_put_masked(0xff,0x70);
  gpio_set_dir_masked(0xff,0xff);
  sleep_ms(1);

  // web 1
  gpio_put(NANDIF_WEB_PIN,1);
  sleep_ms(1);

  // cle 0
  gpio_put(NANDIF_CLE_PIN,0);
  sleep_ms(1);

  // io input
  gpio_set_dir_masked(0xff,0x00);

  // reb 0
  gpio_put(NANDIF_REB_PIN,0);
  sleep_ms(1);

  // read status
  status = gpio_get_all()&0xff;

  // reb 1
  gpio_put(NANDIF_REB_PIN,1);
  sleep_ms(1);

  // ceb 1
  gpio_put(NANDIF_CEB0_PIN,1);
  sleep_ms(1);

  return status;
}

void rawnand_idRead(uint8_t * rxd){
  while ( gpio_get(NANDIF_RBB_PIN)==false ) {
    uart_puts(UART_ID, "Waiting.\r\n");
  }

  gpio_put(NANDIF_CEB0_PIN,0);
  gpio_put(NANDIF_CLE_PIN,1);
  gpio_put(NANDIF_ALE_PIN,0);
  gpio_put(NANDIF_WEB_PIN,0);
  gpio_put(NANDIF_REB_PIN,1);

  // IO output                                                                                                                                                                                                    
  gpio_put_masked(0xff,0x90);
  gpio_set_dir_masked(0xff,0xff);
  sleep_ms(1);

  // web 1                                                                                                                                                                                                        
  gpio_put(NANDIF_WEB_PIN,1);
  sleep_ms(1);

  // cle 0, ale 1, address 0x00                                                                                                                                                                                  
  gpio_put(NANDIF_CLE_PIN,0);
  gpio_put(NANDIF_ALE_PIN,1);
  gpio_put(NANDIF_WEB_PIN,0);
  gpio_put_masked(0xff,0x00);
  sleep_ms(1);

  // web 1
  gpio_put(NANDIF_WEB_PIN,1);
  sleep_ms(1);

  // ale 0
  gpio_put(NANDIF_ALE_PIN,0);
  sleep_ms(1);

  gpio_set_dir_masked(0xff,0x00);
  
  for (int l=0;l<5;l++){
      gpio_put(NANDIF_REB_PIN,0);
      sleep_ms(1);
      *(rxd+l) = gpio_get_all()&0xff;
      gpio_put(NANDIF_REB_PIN,1);
      sleep_ms(1);
  }
  // ceb 1                                                                                                                                                                                                       
  gpio_put(NANDIF_CEB0_PIN,1);
  sleep_ms(1);
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
    rawnand_init();
    
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
    uint8_t state=FSM_IDLE;
    while (1) {
      if (uart_ring_rx_pop(&rxd)==1){
	// echo back
	uart_putc(UART_ID,rxd);
	switch (state) {
	case FSM_IDLE:
	  if(rxd=='s'){
	    uart_puts(UART_ID, "\r\nStatus Read\r\n");
	    sprintf((char *)uart_tx_buff,"status=0x%02X\r\n",rawnand_statusRead());
	    uart_puts(UART_ID,(const char*)uart_tx_buff);                            
	  }
	  else if (rxd=='i'){
	    uart_puts(UART_ID, "\r\nID Read\r\n");
	    uint8_t id_read_rxd[5];
	    rawnand_idRead(id_read_rxd);
	    sprintf((char *)uart_tx_buff,"0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n",id_read_rxd[0],id_read_rxd[1],id_read_rxd[2],id_read_rxd[3],id_read_rxd[4]);
            uart_puts(UART_ID,(const char*)uart_tx_buff);
	  }
	  else if (rxd=='w'){
	    state=FSM_WPB_S1;
	  }
	  break;
	case FSM_WPB_S1:
	  if (rxd=='p')
	    state=FSM_WPB_S2;
	  else
	    state=FSM_IDLE;
	  break;
	case FSM_WPB_S2:
	  if(rxd=='0'){
	    gpio_put(NANDIF_WPB_PIN,0);
	    uart_puts(UART_ID,"\r\nWPB=0\r\n");
	  }
	  else if (rxd=='1'){
	    gpio_put(NANDIF_WPB_PIN,1);
	    uart_puts(UART_ID,"\r\nWPB=1\r\n");
	  }
	  state=FSM_IDLE;
	  break;
	}
      
      }
    }
}

