/*
	espflasher.c
	v 0.2

	Attempt at pushing new firmware to ESP modules over the air

	https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/storage/spi_flash.html

	SPI_FLASH_SEC_SIZE = 4096
*/


// udp_generic_send(char *d, int len, char *destination_ip, int destination_port, int broadcast)


// Called from jcp-client-esp.c
int ICACHE_FLASH_ATTR flasher(char* data, int len, char *ipaddr)
{
	struct flasher_esp *fe = (struct flasher_esp*)data;
	struct flasher_esp fer;									// reply
	int e=0;
	static char flashdata[4096];
	unsigned int offset=0;

	fer.count = fe->count;									// send this back in any reply
	if (strcmp(fe->magic,"FLASH")==0)
	{
		os_sprintf(fer.magic,"FLASH");
		fer.count	= fe->count;							// echo back values in reply
		fer.cmd		= fe->cmd;
		fer.destport	= JCP_SERVER_PORT+1;
		fer.addr	= fe->addr;
		switch (fe->cmd)
		{
			case FLASHER_CMD_REQ_FWVER:
				os_printf("FLASHER_CMD_REQ_FWVER\n");
				os_sprintf((char*)&fer.flashchunk[0], "%s %s %s",MODEL, FW_VER, __DATE__);
				os_printf(">%s:%d FLASHER\n",ipaddr, fe->destport);
				udp_generic_send((char*)&fer, 256, ipaddr, fe->destport, FALSE); 
			break;


			// This is one shot deal, device should reset after doing this
			case FLASHER_CMD_PREPARE:                                               // prepare to write some flash
				os_printf("FLASHER_CMD_PREPARE\n");
				main_suspend_timers();
				app_suspend_timers();
				strcpy(fer.flashchunk,"OK");	
				udp_generic_send((char*)&fer, 32, ipaddr, fe->destport, FALSE); 
			break;
                        
			// Receiver 4 1k byte blocks of data, do the flash write on the receipt of the 4th one
			case FLASHER_CMD_FLASHCHUNK:						// rx data 1k bytes at a time
				os_printf("FLASHER_CMD_FLASHHUNK chunk=%d %08X\n",fe->chunk, (uint32_t)fe->addr);
				offset=fe->chunk * 1024;
				memcpy(&flashdata, fe->flashchunk+offset, 1024);		// copy 1k of data into buffer	
				if (fe->chunk<3)						// chunks 0,1 and 2 ACK receipt
					os_sprintf(fer.flashchunk,"OK CHUNK %d",fe->chunk);	
				else								// chunk 3, do the write
				{
					spi_flash_erase_sector((uint32)fe->addr);
					e=spi_flash_write((uint32)fe->addr,(uint32 *)&flashdata, SPI_FLASH_SEC_SIZE);
					if (e==SPI_FLASH_RESULT_OK)
						strcpy(fer.flashchunk,"OK WRITE");
					else	strcpy(fer.flashchunk,"ERR WRITE");
					bzero(&flashdata, sizeof(flashdata));
				}	
				udp_generic_send((char*)&fer, 32, ipaddr, fe->destport, FALSE); 
			break;

			case FLASHER_CMD_RESTART:
				os_printf("FLASHER_CMD_RESTART\n");
				strcpy(fer.flashchunk,"RESTART");	
				udp_generic_send((char*)&fer, 32, ipaddr, fe->destport, FALSE); 
				system_restart();
			break;
		}
		wdt_feed();
	}
}


//spi_flash_read((uint32)FLASH_CFGRECORD_SECTOR*SPI_FLASH_SEC_SIZE, (uint32 *)cfg, sizeof(configrecord));

//SpiFlashOpResult spi_flash_write(uint32 des_addr, uint32 *src_addr, uint32 size);

//int ICACHE_FLASH_ATTR udp_generic_send(char *d, int len, char *destination_ip, int destination_port, int broadcast);
