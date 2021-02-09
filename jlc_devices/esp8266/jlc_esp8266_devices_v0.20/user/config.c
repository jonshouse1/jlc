// Load and save user configuration to flash

#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <mem.h>
#include <user_interface.h>
#include <espconn.h>
#include "uart.h"

#include "config.h"
extern SpiFlashChip * flashchip;


int ICACHE_FLASH_ATTR dumphex(unsigned char *ptr, int s)
{
	int i,x;

	os_printf("\nDumping %d bytes as hex\n",s);
	x=0;
	for (i=0;i<s;i++)
	{
		os_printf("%02X ",ptr[i]);
		//if (ptr[i]>32)
			//os_printf("(%c) ",ptr[i]);
		x++;
		if (x>20)
		{
			x=0;
			os_printf("\n");
		}
	}
	os_printf("\n");
}


int ICACHE_FLASH_ATTR configuration_save(configrecord *cfg)
{
	uint32 dc;
	uint32 cs;
	unsigned char *raw=(unsigned char*)cfg;
	int i;

	dc=flashchip->chip_size;
	os_printf("device thinks flash is %08X big\n",dc);
	flashchip->chip_size = 0x01000000;
	os_printf("Writing config rec to Sector %d (SZ:%d) add: %08X\n",(uint32)FLASH_CFGRECORD_SECTOR,SPI_FLASH_SEC_SIZE,FLASH_CFGRECORD_SECTOR*SPI_FLASH_SEC_SIZE);
	os_memcpy(cfg->magic,CONFIG_MAGIC,sizeof(CONFIG_MAGIC));

	cs=0;												// checksum
	for (i=8;i<sizeof(configrecord);i++)								// Add up everything except csum itself
		cs=cs+raw[i];
	cfg->csum=cs;
	dumphex(raw,sizeof(configrecord));
	os_printf("Saving config record of size:%d with a CSUM: %d\n",sizeof(configrecord),cs);

	spi_flash_erase_sector((uint32)FLASH_CFGRECORD_SECTOR);						// I do mean to do this twice or write can be unreliable
	if (spi_flash_erase_sector((uint32)FLASH_CFGRECORD_SECTOR)!=SPI_FLASH_RESULT_OK)
	{
		flashchip->chip_size=dc;
		goto err;
	}
	if (spi_flash_write((uint32)FLASH_CFGRECORD_SECTOR*SPI_FLASH_SEC_SIZE,(uint32 *)cfg,sizeof(configrecord))!=SPI_FLASH_RESULT_OK)
	{
		flashchip->chip_size=dc;
		goto err;
	}
	return(0);

err:	//system_update_cpu_freq(f);
	return(-1);
}




// Returns:	 0 if the record loaded from flash was good, magic correct, checksum correct
//		-1 not configured yet, configuration record is blank (zeros)
//		-2 checksum or magic not good	
int ICACHE_FLASH_ATTR configuration_load(configrecord *cfg)
{
	uint32 dc;
	uint32 cs;
	unsigned char *raw=(unsigned char*)cfg;
	int i;
	int allff,mg,cg;

	//os_printf("\n\rsizeof(configrecord.csum)=%d\n\r\n\r",sizeof(cfg->csum));
	bzero(cfg,sizeof(configrecord));
	dc=flashchip->chip_size;
	os_printf("device thinks flash is %08X big\n",dc);
	flashchip->chip_size = 0x01000000;								// override it
	os_printf("Reading config rec from Sector %d (SZ:%d) add: %08X :",(uint32)FLASH_CFGRECORD_SECTOR,SPI_FLASH_SEC_SIZE,FLASH_CFGRECORD_SECTOR*SPI_FLASH_SEC_SIZE);

	spi_flash_read((uint32)FLASH_CFGRECORD_SECTOR*SPI_FLASH_SEC_SIZE, (uint32 *)cfg, sizeof(configrecord));

	allff=TRUE;
	cs=0;												// calculate checksum for data we just read
	dumphex(raw,sizeof(configrecord));
	for (i=8;i<sizeof(configrecord);i++)								// Add up everything except csum itself
	{
		cs=cs+raw[i];
		if (raw[i]!=0xff)
			allff=FALSE;									// not all bytes are 0xff
	}
	if (cs==0)
	{
		os_printf("blank 0x00\n");
		flashchip->chip_size=dc;
		return(-1);										// not configured yet
	}
	if (allff==TRUE)
	{
		os_printf("blank 0xff\n");
		flashchip->chip_size=dc;
		return(-1);
	}

	mg=FALSE;
	if (strncmp(cfg->magic,CONFIG_MAGIC,sizeof(CONFIG_MAGIC))==0)					// got good magic ?
	{
		mg=TRUE;
		os_printf("magic-good ");
		cg=FALSE;
		if (cfg->csum == cs)									// Is checksum good, we only check if magic was good
		{
			cg=TRUE;
			os_printf("CSUM-good ");
		}
		else	os_printf("CSUM-bad expeted %d got %d",cfg->csum,cs);
	}
	else	os_printf("magic-bad ");
	os_printf("\n");

	if ( mg!=TRUE || cg!=TRUE )									// if magic is bad or csum is bad
	{
		//os_printf("Blanking flash config record\n");
		//bzero(cfg,sizeof(configrecord));
		//spi_flash_erase_sector((uint32)FLASH_CFGRECORD_SECTOR);					// blank sector will now all be 0xff
	}

	flashchip->chip_size=dc;
	if ( mg==TRUE && cg==TRUE)									// all was good, then return 0
		return 0;
	return -2;
}



//flashchip->chip_size=(FLASH_CFGRECORD_SECTOR*SPI_FLASH_SEC_SIZE)+SPI_FLASH_SEC_SIZE;			// Tell it its wrong !
