/* V1729.H : source code for V1729 module */
/* created 14.10.2009 by Franco.LARI     */

// Register address  
#define V1729_RESET_BOARD 			0x0800
#define V1729_START_ACQUISITION		0x1700
#define V1729_TRIGGER_TYPE 			0x1D00
#define V1729_FP_FREQUENCY 			0x8100
#define V1729_INTERRUPT				0x8000
#define V1729_LOAD_TRIGGER_THS		0x0900
#define V1729_THRESHOLD      		0x0A00
#define V1729_SOFTWARE_TRIGGER		0x1C00
#define V1729_RAM_DATA_VME			0x0D00
#define V1729_RAM_INT_ADD_LSB		0x0E00
#define V1729_RAM_INT_ADD_MSB		0x0F00
#define V1729_PRETRIG_LSB			0x1800
#define V1729_PRETRIG_MSB			0x1900
#define V1729_POSTTRIG_LSB			0x1A00
#define V1729_POSTTRIG_MSB			0x1B00
#define V1729_TRIGGER_CHANNEL_SRC	0x1E00
#define V1729_TRIG_REC				0x2000
#define V1729_FAST_READ_MODES		0x2100
#define V1729_NB_OF_COLS_TO_READ	0x2200
#define V1729_CHANNEL_MASK			0x2300
#define V1729_FPGA_VERSION			0x8200
#define V1729_EN_VME_IRQ			0x8300
#define V1729_EN_SYNC_OUT			0x1100
#define V1729_MODE_REGISTER         0x0300

#define V1729                       0x3
#define V1729A                      0xF



#define CAENBASEADDRESS  0x30010000
#define V1729_RAM_DEPH 10252
#define V1729_NOS 2560
#define V1729_VERNIER_DEPH 65536

/************************************
GLOBAL VARIABLES
************************************/

int32_t handle; 
const CVDataWidth data_size = cvD32
const unsigned short addr_mode = cvA32_U_DATA;

/*********************
 Function Declarations
*********************/

CVErrorCodes write_to_vme(uint32_t, uint32_t *);
CVErrorCodes read_from_vme(uint32_t, uint32_t *); 
CVErrorCodes reset_vme(void);
CVErrorCodes start_acq(void);
int wait_for_interrupt(void); 
