//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

//#include "mystuff.h"
#include "mfs.h"
#include "spi_flash.h"

#include "osapi.h"
#include "ets_sys.h"


// Possible locations for Memory File System
#define MFLX 5
uint32 mfs_locations[MFLX] = { 	0x10000,		// 64k
				0x080000,		// 512k
				0x090000,		// 576k
				0x100000, 		// 1M 
				0x200000 };		// 2M
uint32 mfs_at = 0;
int did_mfs_search=FALSE;

extern SpiFlashChip * flashchip;
void ICACHE_FLASH_ATTR FindMPFS()
{
	flashchip->chip_size = 0x01000000;
	int i;
	unsigned char mfsmagic[8] __attribute__ ((aligned (4)));

	if (did_mfs_search==TRUE)
		return;
	for (i=0;i<MFLX;i++)
	{
		os_printf("Looking for MFS at (%08x)\t",mfs_locations[i]);
		bzero(&mfsmagic,sizeof(mfsmagic));
		spi_flash_read( mfs_locations[i], (uint32*)&mfsmagic, sizeof(mfsmagic) );
		if (strncmp("MPFSMPFS", (char*)mfsmagic, sizeof(mfsmagic))==0)
		{
			os_printf("Found MFS\n");
			mfs_at=mfs_locations[i];
			break;
		}	
		else	os_printf("Not here\n");
	}
	if (mfs_at==0)
		os_printf("Error: Could not find MFS!\n");
	did_mfs_search=TRUE;
}




//Returns 0 on succses.
//Returns size of file if non-empty
//If positive, populates mfi.
//Returns -1 if can't find file or reached end of file list.
int8_t ICACHE_FLASH_ATTR MFSOpenFile( const char * fname, struct MFSFileInfo * mfi )
{
	if( mfs_at == 0 )
	{
		FindMPFS();
	}
	if( mfs_at == 0 )
	{
		return -1;
	}

	//EnterCritical();
	flashchip->chip_size = 0x01000000;
	uint32 ptr = mfs_at;
	struct MFSFileEntry e;
	while(1)
	{
		spi_flash_read( ptr, (uint32*)&e, sizeof( e ) );		
		ptr += sizeof(e);
		if( e.name[0] == 0xff || ets_strlen( e.name ) == 0 ) break;

		if( ets_strcmp( e.name, fname ) == 0 )
		{
			mfi->offset = e.start;
			mfi->filelen = e.len;
			mfi->filesize = e.len;
			flashchip->chip_size = 0x00080000;
			//ExitCritical();
			return 0;
		}
	}
	flashchip->chip_size = 0x00080000;
	//ExitCritical();
	return -1;
}



//JA New
// Web server needs to know if we have a matching file
// increment idx until the fname string is NULL
int8_t ICACHE_FLASH_ATTR MFSFileList(int idx, char *fname, int *length )
{
	int i=0;
	if( mfs_at == 0 )
	{
		FindMPFS();
	}
	if( mfs_at == 0 )
	{
		return -1;
	}

	//EnterCritical();
	flashchip->chip_size = 0x01000000;
	uint32 ptr = mfs_at;
	struct MFSFileEntry e;
	i=0;
	do
	{
		spi_flash_read( ptr, (uint32*)&e, sizeof( e ) );		
		ptr += sizeof(e);
		if ( i == idx )
		{
			if ( (e.name[0] == 0xff ) || ( e.name[0]==0) || (ets_strlen( e.name ) == 0) )
				return -1;
			memcpy(fname, &e.name, sizeof(e.name));
			*length = e.len;
			flashchip->chip_size = 0x00080000;
			//ExitCritical();
			return 0;
		}
		i++;
	} while(i<=idx);	

	flashchip->chip_size = 0x00080000;
	//ExitCritical();
	return -1;
}




int32_t ICACHE_FLASH_ATTR MFSReadSector( uint8_t* data, struct MFSFileInfo * mfi )
{
	 //returns # of bytes left tin file.
	if( !mfi->filelen )
	{
		return 0;
	}

	int toread = mfi->filelen;
	if( toread > MFS_SECTOR ) toread = MFS_SECTOR;

	//EnterCritical();
	flashchip->chip_size = 0x01000000;
	spi_flash_read( mfs_at+mfi->offset, (uint32*)data, MFS_SECTOR );
	flashchip->chip_size = 0x00080000;
	//ExitCritical();

	mfi->offset += toread;
	mfi->filelen -= toread;
	return mfi->filelen;
}



