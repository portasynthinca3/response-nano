//Response Nano Firmware
//By portasynthinca3 for Lithium Aerospace

//System-wide necessary definitions
#ifndef NULL
#define NULL ((void*)0)
#endif

//Settings
#define SERIAL_BAUD_RATE                            115200
#define SERIAL_GREETINGS                            F(">Is anyone there?\n>Oh.\n>Hi!_\n---=== Response Nano firmware is running ===---")

//Pins
#define PIN_LED                                     5
#define PIN_I2C_SDA                                 21
#define PIN_I2C_SCL                                 22

//State definitions
#define STATE_IDLE                                  0
#define STATE_CRASH                                 1