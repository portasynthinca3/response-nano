//Response Nano Firmware
//By portasynthinca3 for Lithium Aerospace

//System-wide necessary definitions
#ifndef NULL
#define NULL ((void*)0)
#endif

//Settings
#define SERIAL_BAUD_RATE                            115200
#define SERIAL_GREETINGS                            F(">Is anyone there?\n>Oh.\n>Hi!_\n---=== Response Nano firmware is running ===---")
#define SETTINGS_FILE                               F("/rnsettings")
#define SETTINGS_DEFAULT                            F("ground_level=0\nrfd_data_freq=10\nrfd_format=acc_x:float acc_y:float acc_z:float gyro_x:float gyro_y:float gyro_z:float baro_height:float pyro0:int hall:int baro_temp:int\n")

//Pins
#define PIN_LED                                     5
#define PIN_I2C_SDA                                 21
#define PIN_I2C_SCL                                 22

//State definitions
#define STATE_IDLE                                  0
#define STATE_CRASH                                 1
#define STATE_FLIGHT                                2