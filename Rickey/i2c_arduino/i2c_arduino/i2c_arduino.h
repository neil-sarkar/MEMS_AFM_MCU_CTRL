
// Bit Definitions
#define BIT0   0x01u
#define BIT1   0x02u
#define BIT2   0x04u
#define BIT3   0x08u
#define BIT4   0x10u
#define BIT5   0x20u
#define BIT6   0x40u
#define BIT7   0x80u
#define BIT8   0x100u
#define BIT9   0x200u
#define BIT10  0x400u
#define BIT11  0x800u
#define BIT12  0x1000u
#define BIT13  0x2000u
#define BIT14  0x4000u
#define BIT15  0x8000u
#define BIT16  0x10000u
#define BIT17  0x20000u
#define BIT18  0x40000u
#define BIT19  0x80000u
#define BIT20  0x100000u
#define BIT21  0x200000u
#define BIT22  0x400000u
#define BIT23  0x800000u
#define BIT24  0x1000000u
#define BIT25  0x2000000u
#define BIT26  0x4000000u
#define BIT27  0x8000000u
#define BIT28  0x10000000u
#define BIT29  0x20000000u
#define BIT30  0x40000000u
#define BIT31  0x80000000u


#define I2CDOWNLOAD_INIT 0x08u

/***** function definitions *****/

void set_address(void);
void i2c_send_init(void);
void i2c_send_data_bytes(int num_bytes);
void request_slave_read(int num_bytes);
void erase_memory(int pages);
void run_memory(void);
void set_data(void);
unsigned char rotl8 (char value, unsigned int count);

