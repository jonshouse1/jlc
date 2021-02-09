//Copyright 2015 <>< Charles Lohr Under the MIT/x11 License, NewBSD License or
// ColorChord License.  You Choose.

//Not to be confused with MFS for the AVR.
// JA Moved things around, added notes, new flash layout for 1 mega byte flash on H801 device

#ifndef _MFS_H
#define _MFS_H

#include "mem.h"
#include "c_types.h"

// JA hex = decimal = K bytes cheat sheet
//	0x000000	=	     0		= 0 		(start of 0x00000.bin)
//	0x010000	=	 65536		= 64k byte
//	0x020000	= 	131072		= 128k byte
//	0x030000	=	196608		= 192k byte
//	0x040000	=	262144		= 256k byte	(start of 0x40000.bin)
//	0x080000	=	524288		= 512k byte
//	0x090000	=	589824		= 576k byte
//	0x100000	=	1048576		= 1M Byte (flash top for 

//SPI_FLASH_SEC_SIZE      4096

//If you are on a chip with limited space, MFS can alternatively live here, with a max size of 180kB.
//#define MFS_ALTERNATIVE_START 0x10000						// 65536 = 64 k byte
#define MFS_ALTERNATIVE_START 0x80000						// 524288 = 512k byte


#define MFS_STARTFLASHSECTOR  0x100						// 256 decimal
#define MFS_START	(MFS_STARTFLASHSECTOR*SPI_FLASH_SEC_SIZE)		// 256 * 4096 = 1048576 (0x100000) = 1 Mega byte
#define MFS_SECTOR	256


#define MFS_FILENAMELEN 32-8

//Format:
//  [FILE NAME (24)] [Start (4)] [Len (4)]
//  NOTE: Filename must be null-terminated within the 24.
struct MFSFileEntry
{
	char name[MFS_FILENAMELEN];
	uint32 start;  //From beginning of mfs thing.
	uint32 len;
};


struct MFSFileInfo
{
	uint32 offset;
	uint32 filelen;					// Really this is remaing to read counter
	uint32 filesize;				// the size of this file 
};



//Returns 0 on succses.
//Returns size of file if non-empty
//If positive, populates mfi.
//Returns -1 if can't find file or reached end of file list.
int8_t ICACHE_FLASH_ATTR MFSOpenFile( const char * fname, struct MFSFileInfo * mfi );
int32_t MFSReadSector( uint8_t* data, struct MFSFileInfo * mfi ); //returns # of bytes left in file.
int8_t ICACHE_FLASH_ATTR MFSFileList(int idx, char *fname, int *length );




#endif


