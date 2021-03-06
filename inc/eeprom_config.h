#ifndef _EEPROM_CONFIG_H_
#define _EEPROM_CONFIG_H_

#include "chip.h"
#include "state_types.h"
#include "console_types.h"
#include "state_types.h"
#include "lc1024.h"
#include "config.h"
#include "board.h"

#define EEPROM_DATA_START_PCKCFG 0x000000
#define EEPROM_DATA_START_CC 0x000100
#define STORAGE_VERSION 0x03
#define CHECKSUM_BYTESIZE 1
#define VERSION_BYTESIZE 1

// Default Pack Configuration
#define CELL_MIN_mV 2800
#define CELL_MAX_mV 4200
#define CELL_CAPACITY_cAh 250
#define NUM_MODULES 1
#define CELL_CHARGE_C_RATING_cC 5
#define BALANCE_ON_THRESHOLD_mV 4
#define BALANCE_OFF_THRESHOLD_mV 1
#define PACK_CELLS_PARALLEL 12
#define CV_MIN_CURRENT_mA 100
#define CV_MIN_CURRENT_ms 60000
#define CC_CELL_VOLTAGE_mV 4300
#define CELL_DISCHARGE_C_RATING_cC 200 // at 27 degrees C
#define MAX_CELL_TEMP_C 50
#define MODULE_CELL_COUNT 12

void EEPROM_Init(LPC_SSP_T *pSSP, uint32_t baud, uint8_t cs_gpio, uint8_t cs_pin);
uint8_t EEPROM_ChangeConfig(rw_loc_label_t rw_loc, uint32_t val);
bool EEPROM_LoadPackConfig(PACK_CONFIG_T *pack_config);
void Write_EEPROM_PackConfig_Defaults(void);


uint32_t EEPROM_LoadCCPage_Num(uint8_t idx);
void EEPROM_WriteCCPage_Num(uint8_t idx, uint32_t val);
void EEPROM_LoadCCPage(uint32_t *cc);
void EEPROM_WriteCCPage(uint32_t *cc);
#endif
