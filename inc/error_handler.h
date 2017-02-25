#ifndef _ERROR_HANDLER_H
#define _ERROR_HANDLER_H

#include <stdint.h>
#include <stdbool.h>


typedef enum error {
    ERROR_LTC6804_PEC,
    ERROR_LTC6804_CVST,
    ERROR_LTC6804_OWT,
    EEPROM_ERROR,
    ERROR_INVALID_SSM_STATE,
    ERROR_CONTACTORS_ERRONEOUS_STATE,
    ERROR_CELL_UNDER_VOLTAGE,
    ERROR_CELL_OVER_VOLTAGE,
    ERROR_CELL_OVER_TEMP,
    ERROR_OVER_CURRENT,
    ERROR_BRUSA,
    ERROR_CAN,
    ERROR_NUM_ERRORS
} ERROR_T;

static const char * const ERROR_NAMES[ERROR_NUM_ERRORS] = {
    "ERROR_LTC6804_PEC",
    "ERROR_LTC6804_CVST",
    "ERROR_LTC6804_OWT",
    "EEPROM_ERROR",
    "ERROR_INVALID_SSM_STATE",
    "ERROR_CONTACTORS_ERRONEOUS_STATE",
    "ERROR_CELL_UNDER_VOLTAGE",
    "ERROR_CELL_OVER_VOLTAGE",
    "ERROR_CELL_OVER_TEMP",
    "ERROR_OVER_CURRENT",
    "ERROR_BRUSA", // [TODO] Remove for FSAE
    "ERROR_CAN"
};

typedef enum error_handler_status {
	HANDLER_FINE,
	HANDLER_HALT
} ERROR_HANDLER_STATUS_T;

typedef struct error_status {
	bool 		handling;
	bool		error;
	uint16_t 	count;
	uint32_t 	time_stamp;
} ERROR_STATUS_T;

typedef  ERROR_HANDLER_STATUS_T (*ERROR_HANDLER)(ERROR_STATUS_T* , const uint32_t );


void Error_Init(void);
void Error_Assert(ERROR_T er_t, uint32_t msTicks);
void Error_Pass(ERROR_T er_t);
const ERROR_STATUS_T *  Error_GetStatus(ERROR_T er_t);
ERROR_HANDLER_STATUS_T Error_Handle(uint32_t msTicks);


#endif