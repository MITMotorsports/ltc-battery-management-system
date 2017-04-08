#include <string.h>
#include <stdlib.h>
#include "board.h"
#include "ssm.h"
#include "measure.h"
#include "soc.h"
#include "console.h"
#include "eeprom_config.h"
#include "config.h"
#include "error_handler.h"
#include "brusa.h"

#define BAL_SW 1, 2
#define IOCON_BAL_SW IOCON_PIO1_2
#define CHRG_SW 1, 2
#define IOCON_CHRG_SW IOCON_PIO1_2
#define DISCHRG_SW 1, 2
#define IOCON_DISCHRG_SW IOCON_PIO1_2

#define EEPROM_CS_PIN 0, 7

extern volatile uint32_t msTicks;

static char str[10];

// memory allocation for BMS_OUTPUT_T
static bool balance_reqs[MAX_NUM_MODULES*MAX_CELLS_PER_MODULE];
static BMS_CHARGE_REQ_T charge_req;
static BMS_OUTPUT_T bms_output;

// memory allocation for BMS_INPUT_T
static BMS_PACK_STATUS_T pack_status;
static BMS_INPUT_T bms_input;

// memory allocation for BMS_STATE_T
static BMS_CHARGER_STATUS_T charger_status;
static uint32_t cell_voltages[MAX_NUM_MODULES*MAX_CELLS_PER_MODULE];
static int16_t cell_temperatures[MAX_NUM_MODULES*MAX_THERMISTORS_PER_MODULE];
static uint8_t module_cell_count[MAX_NUM_MODULES];
static PACK_CONFIG_T pack_config;
static BMS_STATE_T bms_state;

// memory for console
static microrl_t rl;
static CONSOLE_OUTPUT_T console_output;


/****************************
 *        HELPERS
 ****************************/

/****************************
 *     INITIALIZERS
 ****************************/

void Init_BMS_Structs(void) {
    bms_output.charge_req = &charge_req;
    bms_output.close_contactors = false;
    bms_output.balance_req = balance_reqs;
    memset(balance_reqs, 0, sizeof(balance_reqs[0])*MAX_NUM_MODULES*MAX_CELLS_PER_MODULE);
    bms_output.read_eeprom_packconfig = false;
    bms_output.check_packconfig_with_ltc = false;

    charge_req.charger_on = 0;
    charge_req.charge_current_mA = 0;
    charge_req.charge_voltage_mV = 0;

    bms_state.charger_status = &charger_status;
    bms_state.pack_config = &pack_config;
    bms_state.curr_mode = BMS_SSM_MODE_INIT;
    bms_state.init_state = BMS_INIT_OFF;
    bms_state.charge_state = BMS_CHARGE_OFF;
    bms_state.discharge_state = BMS_DISCHARGE_OFF;

    charger_status.connected = false;
    charger_status.error = false;

    pack_config.module_cell_count = module_cell_count;
    pack_config.cell_min_mV = 0;
    pack_config.cell_max_mV = 0;
    pack_config.cell_capacity_cAh = 0;
    pack_config.num_modules = 0;
    pack_config.cell_charge_c_rating_cC = 0;
    pack_config.bal_on_thresh_mV = 0;
    pack_config.bal_off_thresh_mV = 0;
    pack_config.pack_cells_p = 0;
    pack_config.cv_min_current_mA = 0;
    pack_config.cv_min_current_ms = 0;
    pack_config.cc_cell_voltage_mV = 0;

    pack_config.cell_discharge_c_rating_cC = 0; // at 27 degrees C
    pack_config.max_cell_temp_C = 0;

    //assign bms_inputs
    bms_input.hard_reset_line = false;
    bms_input.mode_request = BMS_SSM_MODE_STANDBY;
    bms_input.balance_mV = 0; // console request balance to mV
    bms_input.contactors_closed = false;
    bms_input.msTicks = msTicks;
    bms_input.pack_status = &pack_status;
    bms_input.charger_on = false;
    bms_input.eeprom_packconfig_read_done = false;
    bms_input.ltc_packconfig_check_done = false;
    bms_input.eeprom_read_error = false;

    memset(cell_voltages, 0, sizeof(cell_voltages));
    memset(cell_temperatures, 0, sizeof(cell_temperatures));
    pack_status.cell_voltages_mV = cell_voltages;
    pack_status.cell_temperatures_dC = cell_temperatures;
    pack_status.pack_cell_max_mV = 0;
    pack_status.pack_cell_min_mV = 0xFFFFFFFF;
    pack_status.pack_current_mA = 0;
    pack_status.pack_voltage_mV = 0;
    pack_status.max_cell_temp_C = 0;

}

void Process_Input(BMS_INPUT_T* bms_input) {
    // Read current mode request
    // Override Console Mode Request
    // Read pack status
    // Read hardware signal inputs
    // update and other fields in msTicks in &input
    
    // Contactor Mocking
    if(Board_Switch_Read(CTR_SWTCH) == 0) {
        bms_input->contactors_closed = true;
    } else {
        bms_input->contactors_closed = false;
    }

    if (bms_state.curr_mode != BMS_SSM_MODE_INIT) {
        Board_GetModeRequest(&console_output, bms_input);
        Board_CAN_ProcessInput(bms_input, &bms_output); // CAN has precedence over console
        Board_LTC6804_ProcessInputs(&pack_status, &bms_output);
    }

    bms_input->msTicks = msTicks;
}

void Process_Output(BMS_INPUT_T* bms_input, BMS_OUTPUT_T* bms_output, BMS_STATE_T * bms_state) {
    // If SSM changed state, output appropriate visual indicators
    // Carry out appropriate hardware output requests (CAN messages, charger requests, etc.)
#ifdef FSAE_DRIVERS
    if(bms_output->close_contactors) {
        Board_LED_On(FSAE_FAULT_GPIO);
    } else {
        Board_LED_Off(FSAE_FAULT_GPIO);
    }
#else
    if(bms_output->close_contactors) {
        Board_LED_On(LED2);
    } else {
        Board_LED_Off(LED2);
    }
#endif //FSAE_DRIVERS
    
    if (bms_output->read_eeprom_packconfig){
        if(console_output.config_default){
            Write_EEPROM_PackConfig_Defaults();
            console_output.config_default = false;
        }
        bms_input->eeprom_packconfig_read_done = EEPROM_LoadPackConfig(&pack_config);
        Charge_Config(&pack_config);
        Discharge_Config(&pack_config);
        Board_LTC6804_DeInit(); 

    } else if (bms_output->check_packconfig_with_ltc) {
        bms_input->ltc_packconfig_check_done = Board_LTC6804_Init(&pack_config, cell_voltages);
        // bms_input->ltc_packconfig_check_done = true;
        
    } else {
        Board_LTC6804_ProcessOutput(bms_output->balance_req);
        Board_CAN_ProcessOutput(bms_input, bms_state, bms_output);
    }

}

void Process_Keyboard(void) {
    uint32_t readln = Board_Read(str,50);
    uint32_t i;
    for(i = 0; i < readln; i++) {
        microrl_insert_char(&rl, str[i]);
    }
}

// PLANNED DEMO FOR FSAE AT END OF FEB
    // Turn on BMS
    // Get Cell Voltages
    // Over Serial, Balance 4mV below min
        // If taking too long, get balance to finish by changing voltage
    // Go back to BMS_SSM_MODE_STANDBY
    // Use VCU to enter discharge
        // Look at contactors, current, voltage, etc
    // Go back to BMS_SSM_MODE_STANDBY
    // Plug in Brusa
    // Use serial to enter charge
        // Look at contactors, current, voltage, etc
    // Go back to BMS_SSM_MODE_STANDBY
    // Modify parameters
    // Repeat
    // Cause Error
    // Restart

// BIG TODO LIST
// ----------------
// [TODO] Add mode for continuous data stream           WHO:Skanda
// [TODO] Undervoltage (create error handler)           WHO:Erpo
// [TODO] SOC error (create error handler [CAN msg])    WHO:Erpo
// [TODO] Reasonable way to change polling speeds       WHO:ALL
// [TODO] Add current sense handling                    WHO:Jorge
//        Make sure to add it in the 'measure' command
// [TODO] Add thermistor array handling                 WHO:Jorge
//        Make sure to add it in the 'measure' command
// [TODO] Clean up macros                               WHO:Skanda
// [TODO] Remove LTC_SPI error                          WHO:Erpo
// [TODO] CAN error handling for different CAN errors   WHO:Skanda/Rango

// [TODO] Do heartbeats, see board.c todo               WHO:Rango
// [TODO] Cleanup board                                 ALL
// [TODO] Print out mod/cell to min/max, error          WHO:Eric
// [TODO] Refactor similiar functions in error handler  WHO:Rango
// [TODO] Line 253, console.c                           WHO:Rango!!
// [TODO] Validate packconfig values in EEPROM either 
//              needs to be moved 
//        elsewhere and implement bounds checking
// [TODO] EEPROM checksum!!!                            WHO:Skanda [DONE]
// [TODO] Review/cleanup GetModeRequest in board.c:523  WHO:ALL
// [TODO] SPACES AND TABS ARE CONSISTENT                WHO:Skanda [DONE]
// [TODO] On a Force Hang, write the error to EEPROM    WHO:Rango
// [TODO] Implement watchdog timer                      WHO:Erpo
// [TODO] Open contactors if pack                       WHO:Jorge
//        current goes above maximum and move to
//        standby
// [TODO] open contactors if the charge current is      WHO:Jorge
//        above the charge C rating and move to standby
// [TODO] Open contactors if a cell goes                
//        above the maximum allowed temperature and go
//        to standby
// [TODO] Convert thermistor voltages into cell         
//        temperatures
// [TODO] Get minimum, maximum, and average cell        
//        temperature
// [TODO] Control fans                                  
// [TODO] implement all messages in FSAE CAN spec       WHO:Jorge
// [TODO] send CAN messages when BMS hangs as well      WHO:Jorge
// [TODO] implement logic that opens contactors if a    
//        blown fuse is detected
// [TODO] set charge enable pin to logic high if BMS    WHO:Jorge
//        is ready to charge
// [TODO] make the BMS hang if the pack current is high 
//        while the BMS is in standby, init, or balance
// [TODO] make the BMS hang if the contactors are       
//        closed when the BMS is in standby, init, or
//        balance
// [TODO] Implement things in FSAE's BMS specification  WHO:Jorge
// [TODO] Remove code for discharge requests            WHO:Jorge
// [TODO] process input struct,output other signals     
// [TODO] Add print out leveling                        WHO:Erpo/Skanda
//
// [TODO at the end] Add console print handling **      WHO:Rango
// [TODO at the end] Add console history                WHO:Rango
// [TODO at the end] BRUSA error handling               WHO:Erpo
//
// [Not critical]
// [TODO] Send warnings through CAN                     WHO:Jorge
int main(void) {

    Init_BMS_Structs();

    Board_Chip_Init();
    Board_GPIO_Init();
    Board_Headroom_Init();
    Board_CAN_Init(CAN_BAUD);
    Board_UART_Init(UART_BAUD);

#ifdef DEBUG_ENABLE
	Board_Println("Board Up (DEBUG)");
#else
    Board_Println("Board Up");   
#endif

    EEPROM_Init(LPC_SSP1, EEPROM_BAUD, EEPROM_CS_PIN); 
    SOC_Init();
    Board_Println_BLOCKING("Finished EEPROM init");
    
    Error_Init();
    SSM_Init(&bms_input, &bms_state, &bms_output);

    //setup readline
    microrl_init(&rl, Board_Print);
    microrl_set_execute_callback(&rl, executerl);
    console_init(&bms_input, &bms_state, &console_output);

    Board_Println("Applications Up");

    uint32_t last_count = msTicks;

    while(1) {

        Board_Headroom_Toggle(); // Used for measuring main-loop length

        Process_Keyboard(); // Handle UART Input
        Process_Input(&bms_input); // Process Inputs to board for bms
        SSM_Step(&bms_input, &bms_state, &bms_output);
        Process_Output(&bms_input, &bms_output, &bms_state);
        Output_Measurements(&console_output, &bms_input, &bms_state, msTicks);

        if (Error_Handle(bms_input.msTicks) == HANDLER_HALT) {
            break; // Handler requested a Halt
        }
        
        // Testing Code
        bms_input.contactors_closed = bms_output.close_contactors; // [DEBUG] For testing purposes

        // LED Heartbeat
        if (msTicks - last_count > 1000) {
            last_count = msTicks;
            Board_LED_Toggle(LED1);  
            // Board_PrintNum(SOC_Estimate(), 10);
        }
    }

    Board_Println("FORCED HANG");

    bms_output.close_contactors = false;
    bms_output.charge_req->charger_on = false;
    memset(bms_output.balance_req, 0, sizeof(bms_output.balance_req[0])*Get_Total_Cell_Count(&pack_config));
    bms_output.read_eeprom_packconfig = false;
    bms_output.check_packconfig_with_ltc = false;

    while(1) {
        //set bms_output
        Process_Output(&bms_input, &bms_output, &bms_state);
        Process_Keyboard();
        if(bms_state.curr_mode == BMS_SSM_MODE_INIT) {
            console_output.config_default = false;
            Write_EEPROM_PackConfig_Defaults();
            bms_state.curr_mode = BMS_SSM_MODE_STANDBY;
        }
    }
    return 0;
}

int hardware_test(void) {
    Init_BMS_Structs();

    Board_Chip_Init();
    Board_GPIO_Init();
    Board_UART_Init(UART_BAUD);

    Board_Println("HW Test Board Up"); 

    EEPROM_Init(LPC_SSP1, EEPROM_BAUD, EEPROM_CS_PIN);
    Board_LTC6804_Init(&pack_config, cell_voltages);

    Board_Println("HW Test Drivers Up"); 

    SSM_Init(&bms_input, &bms_state, &bms_output);
    //setup readline
    microrl_init(&rl, Board_Print);
    microrl_set_execute_callback(&rl, executerl);
    console_init(&bms_input, &bms_state, &console_output);
    
    Board_Println("HW Test Applications Up");

    uint32_t last_count = msTicks;

    while(1) {
        // Process_Output(&bms_input, &bms_output);
        Process_Keyboard();

        // LED Heartbeat
        if (msTicks - last_count > 1000) {
            last_count = msTicks;
            Board_LED_Toggle(LED1);  
        }
    }

    return 0;
}

