#include <usitwislave.h>
#include <usitwislave_devices.h>

/*
 * 
static void twi_callback(uint8_t buffer_size,
           volatile uint8_t input_buffer_length, volatile const uint8_t *input_buffer,
                        volatile uint8_t *output_buffer_length, volatile uint8_t *output_buffer)
 * buffer_size = the size of the internal input and output buffers,
 currently this is 16 bytes, but it may enlarged by recompiling
  the library. Do not write more bytes than the buffer_size or
  mayhem will be the result!
input_buffer_length = the amount of bytes received from the master
input_buffer = the bytes received from the master
output_buffer_length = the amount of bytes you want to send back to
  the master
output_buffer = the bytes you want to send back to the master*/

void twi_callback( uint8_t input_buffer_length, const uint8_t *input_buffer,
                         uint8_t *output_buffer_length, uint8_t *output_buffer)
                        
{
  
                          
}

void idle()
{
  
}

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  uint8_t buffer_out[10];
  usi_twi_slave(0x04, 0, twi_callback,  idle);
}
