#ifndef CELL_TEMPERATURES_H
#define CELL_TEMPERATURES_H

// ltc-battery-management-system
#include "state_types.h"
#include "config.h"

// constants
#define NUMBER_OF_MULTIPLEXER_LOGIC_CONTROL_INPUTS 5
#define GROUP_ONE_THERMISTOR_COUNT 13
#define GROUP_TWO_THERMISTOR_OFFSET 3

// Contansts for linear curve fit relating thermistor voltages (in mV) to temperatures
// (in dC)
// The equation is temp_dC = thermistorVoltage/4 - 39
#define SHIFT_VOLTAGE 2
#define A0 -39

/****************************************************************************************
 * Public Functions
 * *************************************************************************************/

/**
 * @details updates array of cell temperatures in pack_status with values stored in 
 *          gpioVoltages
 *
 * @param gpioVoltages array of voltages measured on each GPIO of the LTC6804 chips
 *                     gpioVoltages is structured as follows {GPIO1_module1, ..., 
 *                     GPIO5_module1, GPIO1_module2, ..., GPIO5_module2, ...}
 * @param currentThermistor number of thermistor currently selected
 * @param pack_status mutable datatype containing array of cell temperatures
 */
void CellTemperatures_UpdateCellTemperaturesArray(uint32_t * gpioVoltages, 
        uint8_t currentThermistor, BMS_PACK_STATUS_T * pack_status);

/****************************************************************************************
 * Private Functions
 * *************************************************************************************/

/**
 * @details creates an array of thermistor temperatures (in dC) from an array of gpio 
 * voltages (in mV)
 *
 * @param gpioVoltages array of voltages measured on each GPIO of the LTC6804 chips
 *                     gpioVoltages is structured as follows {GPIO1_module1, ..., 
 *                     GPIO5_module1, GPIO1_module2, ..., GPIO5_module2, ...}
 * @param thermistorTemperatures mutable array of thermistor temperatures. At the end
 * of the function call, thermistorTemperatures will have the following form:
 * {thermistorTemperature_GPIO1_module1, thermistorTemperature_GPIO1_module2, ..., 
 * thermistorTemperature_GPIO1_modulen, ...}
 */
void getThermistorTemperatures(uint32_t * gpioVoltages, 
        int16_t * thermistorTemperatures);

#endif //CELL_TEMPERATURES
