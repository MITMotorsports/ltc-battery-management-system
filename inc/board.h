/**
 * @file board.h
 * @author Eric Ponce
 * @date 25 June 2015
 * @brief Board peripheral driver
 */

#ifndef _BOARD_H_
#define _BOARD_H_

#ifdef TEST_HARDWARE
    #include <stdbool.h>
    #include <stdio.h>
#else
    #include "chip.h"
    #include "can.h"
    #include "ltc6804.h"
    #include "console.h"
#endif
 
#include "error_handler.h"
#include "config.h"
#include "state_types.h"
#include <string.h>
#include <stdint.h>


#define LED0 2, 8
#define LED1 2, 10
#define LED2 1, 3 // pin 39
#define IOCON_LED2 IOCON_PIO1_3
#define CTR_SWTCH 1, 4 // pin 40 on lpc24, used as input 
#define IOCON_CTR_SWTCH IOCON_PIO1_4

#ifdef FSAE_DRIVERS
#define FSAE_FAULT_GPIO 3, 0
#define IOCON_FSAE_FAULT_GPIO IOCON_PIO3_0
#endif //FSAE_DRIVERS

#define BAL_SW 1, 2
#define IOCON_BAL_SW IOCON_PIO1_2
#define CHRG_SW 1, 2
#define IOCON_CHRG_SW IOCON_PIO1_2
#define DISCHRG_SW 1, 2
#define IOCON_DISCHRG_SW IOCON_PIO1_2

#define UART_BUFFER_SIZE 100 // may need to change based on number of BMS size, Rx and Tx size

#define CONTACTOR_P 2, 8
#define CONTACTOR_N 2, 7
#define CONTACTOR_PRE 2, 1

#define HEADROOM 3, 2
#define IOCON_HEADROOM IOCON_PIO3_2
#define Hertz2Ticks(freq) SystemCoreClock / freq

#define TIME_PER_THERMISTOR_MS 400

// ltc6804 constants
#define LTC6804_SHIFT_REGISTER_DATA_IN 4
#define LTC6804_SHIFT_REGISTER_CLOCK 3
#define LTC6804_SHIFT_REGISTER_LATCH 2
#define LTC6804_GPIO_COUNT 5


typedef enum {
    LTC6804_INIT_NONE, LTC6804_INIT_CFG, LTC6804_INIT_CVST, LTC6804_INIT_OWT, LTC6804_INIT_DONE
} LTC6804_INIT_STATE_T;


void Board_Chip_Init(void);

void Board_GPIO_Init(void);

/**
 * @details Initialize Board Status LEDs
 */
void Board_LED_Init(void);

/**
 * @details Turn status LED on
 */
void Board_LED_On(uint8_t led_gpio, uint8_t led_pin);

/**
 * @details Turn status LED off
 */
void Board_LED_Off(uint8_t led_gpio, uint8_t led_pin);

void Board_LED_Toggle(uint8_t led_gpio, uint8_t led_pin);

void Board_Headroom_Init(void);

void Board_Headroom_Toggle(void);

void Board_BlockingDelay(uint32_t dlyTicks);

bool Board_Switch_Read(uint8_t gpio_port, uint8_t pin);

void Board_CAN_Init(uint32_t baudRateHz);

/**
 * @details Initialize the UART used for debugging
 * 
 * @param baudRateHz the desired baud rate
 */
void Board_UART_Init(uint32_t baudRateHz);

/**
 * @details Non-blocking printing for user interface
 *
 * @param str string to print
 */
uint32_t Board_Print(const char *str);

/**
 * @details Non-blocking printing for user interface. Appends a newline
 *
 * @param str string to print
 */
uint32_t Board_Println(const char *str);

/**
 * @details Non-blocking printing for user interface. Appends a newline
 *
 * @param str string to print
 */
uint32_t Board_Write(const char *str, uint32_t count);

uint32_t Board_Read(char *charBuffer, uint32_t length);

uint32_t Board_Print_BLOCKING(const char *str);

uint32_t Board_Println_BLOCKING(const char *str);

/******** LTC6804 Board Functions ***********/

/**
 * @details Initialize board input switch
 */
bool Board_LTC6804_Init(PACK_CONFIG_T * pack_config, uint32_t * cell_voltages_mV);

void Board_LTC6804_DeInit(void);

void Board_LTC6804_ProcessInputs(BMS_PACK_STATUS_T *pack_status, BMS_OUTPUT_T * bms_output);

void Board_LTC6804_ProcessOutput(bool *balance_req);

/**
 * @details get cell voltages
 *
 * @param mutable array of cell voltages
 */
void Board_LTC6804_GetCellVoltages(BMS_PACK_STATUS_T* pack_status, BMS_OUTPUT_T* bms_output);
/**
 * @details iterates through thermistors and saves cell temperatures in pack_status
 *
 * @param pack_status mutable datatype containing array of cell temperatures
 */
void Board_LTC6804_GetCellTemperatures(BMS_PACK_STATUS_T * pack_status);

#ifndef TEST_HARDWARE
/**
 * @details handles status returned by an LTC6804 driver
 *
 * @param status status of the LTC6804
 */
void Board_HandleLtc6804Status(LTC6804_STATUS_T status);
#endif //TEST_HARDWARE

/**
 * @details prints thermistor temperatures of module module
 *
 * @param module number of the module whose thermistor temperatures will be printed
 * @param pack_status datatype containing array of thermistor temperatures
 */
void Board_PrintThermistorTemperatures(uint8_t module, BMS_PACK_STATUS_T * pack_status);

/**
 * @details does a CVST
 *
 * @return true if CVST shows working config, false if waiting. 
 */
bool Board_LTC6804_CVST(void);

/**
 * @details balance selected cell
 * 
 * @param balance_requests balance_requests[i] is true if ith cell should be 
 *                         balanced, false otherwise
 */
void Board_LTC6804_UpdateBalanceStates(bool *balance_req);

/**
 * @details checks that pack configuration is consistent with number of connected LTC6804 slaves
 *
 * @param pack_config configuration of the battery pack:
 * @return true if pack configuration is consistent with number of connected LTC6804 slaves, false otherwise
 */
bool Board_LTC6804_ValidateConfiguration(void);

bool Board_LTC6804_OpenWireTest(void);

/******** Contactor Board Functions ***********/

/**
 * @details closes or opens contactors
 *
 * @param close_contactors true if contactors should be closed, false otherwise
 */
void Board_Close_Contactors(bool close_contactors);

/**
 * @details get the status of the contactors
 *
 * @return true if contactors are closed, false otherwise
 */
bool Board_Contactors_IsClosed(void);

/******** Test Hardware Exemptions ***********/

#ifndef TEST_HARDWARE

/**
 * @details get mode request
 *
 * @param console_output output from console
 * @param bms_input pointer to input to SSMs to modify bal and req
 * @return none
 */
void Board_GetModeRequest(const CONSOLE_OUTPUT_T * console_output, BMS_INPUT_T* bms_input);


void Board_CAN_ProcessInput(BMS_INPUT_T * bms_input, BMS_OUTPUT_T *bms_output);

void Board_CAN_ProcessOutput(BMS_INPUT_T *bms_input, BMS_STATE_T * bms_state, BMS_OUTPUT_T *bms_output);

#endif


#endif
