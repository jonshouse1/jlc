// User settings to save in flash

#include "os_type.h"
#include "ts.h"

#define CONFIG_MAGIC "CONFIG"							// String to identify flash block as something we wrote


// Must be less than SPI_FLASH_SEC_SIZE (4KBytes)
typedef struct __attribute__((packed, aligned(4)))				// flasg write needs to be word aligned
{
	uint32 csum;                                                            // Must always be first
	char pad[4];								// do not remove, csum+pad must be 8 bytes
	char magic[sizeof(CONFIG_MAGIC)];
	char ssid[33];								// ($SSID)	
	char password[33];							// ($PASSWORD)

	char location[33];
	char swi1[17];								// Generic storage
	char swi2[17];
	char swi3[17];
} configrecord;



// ********* NOTE: This works, but at the moment  spi_flash_read() and write think the flash is small, odd !
// Change this to match the flash size your device device !
// Each chunk is 512K Bytes, IE  1=512k, 2=1Meagabyte, 8 for 4Meagabyte etc
#define FLASHSIZE_CHUNKS		8								// How many 512KByte chunks is this flash

// 512k  		Flash is 0x080000 size
// 1024k (1Megabyte) 	Flash is 0x100000
// 2048k (2Megabyte)    Flash is 0x200000
// 4096k (4Megabyte)    Flash is 0x400000 

// Should be the very last but one block in flash. Using the very last seemed to cause issues - it kind of worked, sometimes
#define FLASH_CFGRECORD_SECTOR 	((FLASHSIZE_CHUNKS * 0x80000) - (SPI_FLASH_SEC_SIZE*2)) / SPI_FLASH_SEC_SIZE


