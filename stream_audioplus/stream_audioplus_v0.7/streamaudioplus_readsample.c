// player_readsamples.c
// Rad audio from external mp3 decoder (normally mplayer).
// Several different methods.
//
// The empeg has limited RAM, at the moment we don't compile the caching read for empeg
//
// 	read_audio_samples_simple	- Just read the named pipe as non blocking file
// 	read_audio_samples_full		- read from named pipe with statistics
// 	read_audio_samples_caching	- split into a parent and child process, the child reads the pipe the parent reads the childs cache
//

// 17 Aug 2013
//    Tried to fix chirp chirp chirp problem with ALSA.  Proved the audio data I am writing is correct (non corrupt) and in the correct order.
//    Seems problem is with ALSA on RPI only ... ho hum


// ******************************************************************************
// how much audio is waiting in the fifo for us to read?
// in theory if its more than a read() worth then the read should not block
int read_fifo_fill_level()
{
	int bytesavailable=0;

	if (fd_audioin<0)
	{
                	printf("%s: Opening fifo %s for read failed (%s)\n",PROGNAME,FIFO_AUDIO,strerror(errno));
                	exit(1);
	}

	ioctl (fd_audioin,FIONREAD,&bytesavailable);							// Ask file channel how many bytes read will give us
	if (bytesavailable<0)										// just treat error as no bytes
		bytesavailable=0;
	audioin_surplus=bytesavailable-sizeof(mplayersamples);						// keep a note of how many bytes left after the next read
	if (audioin_surplus<0)										// oh dear, its no bytes then
		audioin_surplus=0;
	//printf("%d %d \n",audioin_surplus,audioin_surplus_chunks); fflush(stdout);
	audioin_surplus_chunks=audioin_surplus/sizeof(mplayersamples);					// We read data in N byte chunks, how many available 
	return(audioin_surplus_chunks);									// tell the caller we have N chunks of audio ready
}




// ******************************************************************************
// returns a positive size or -2 if a timeout
// now uses proper time
int read_audio_samples_full(int16 *audiochunk)
{
	int chunks_available=0;
	int readsize=0;

	rtimeout=0;
tryread:												// Spin a short time until fd_audioin has enough audio data or we timeout
	chunks_available=read_fifo_fill_level();

	rtimeout++;
	if ( (chunks_available <=0 ) & (rtimeout<audioin_timeout) )					// bytes too few and not timed out yet
	{
		//usleep(1);										// yield to the kernel, just a little bit DON'T CHANGE VALUE >1 kills empeg
#ifndef EMPEG
		// keep short ish, tune for a frame rate of around 50fps when no audio is present
		usleep(100);
#endif
		goto tryread;										// have another look to see if we have audio yet 
	}

	readsize=read(fd_audioin,audiochunk,sizeof(mplayersamples)); 					// Read a block of audio data
//printf("chunksavail=%d  readsize=%d\n", chunks_available, readsize);  fflush(stdout);

	if (rtimeout>=audioin_timeout)									// did we timeout ?
		readsize=-2;										// return timeout

	if (VERBOSE>=2)
	{
		fprintf(stderr,"%s: read_audio_samples_full() - got %d bytes\n",PROGNAME,readsize);  fflush(stderr);
		fflush(stderr);
	}
	return(readsize);
}



// ******************************************************************************
// does same as above but is less complex
// Should work, but highlights the strange behaviour.  It doesn't block, but despite
// having data it will sometimes return a -1 for a number of consecutive reads.
// Basically this is useless but interesting if used with the -dg option to show
// what is going on.
int read_audio_samples_simple(int16 *audiochunk)
{
	int readsize=0;

	read_fifo_fill_level();
	if ( player_loops_for_this_track<100)								// track just started ?
	{
		if (audioin_surplus < (64*1024)/sizeof(mplayersamples) )				// Fifo is 64k Bytes, mplayer samples = 2K bytes - so full is 32 chunks of 2K
			return(-1);
	}

	readsize=read(fd_audioin,audiochunk,sizeof(mplayersamples)); 					// Read a block of audio data
#ifdef EMPEG	
	printf("%d\t",readsize); fflush(stdout);
	if (readsize<=0)
		usleep(8000);										// sleep values <3000 ignored
#else
	usleep(100);
#endif

	if (VERBOSE>=2)
	{
		fprintf(stderr,"%s: read_audio_samples_simple() - got %d bytes\n",PROGNAME,readsize);  fflush(stderr);
		fflush(stderr);
	}
	if (readsize==0)
		readsize=-1;
	return(readsize);										// N bytes or -1 if no data
}



// The empeg kernel cant do mmap shared memory
#ifndef EMPEG
// ******************************************************************************
// Problem:  reading audio samples from the named pipe has unhelpful behaviour. 
// sometimes read returns -1 for a number of consecutive reads, despite having data
// once in while the read will stall and take >100ms to return.  The caching code
// hands the fifo read off to the cache helper process. The main loop takes
// its data from memory it shares with the child process
// 
// fore some reason memcpy didn't work
// memcpy(&mplayersamples,samplecache->mplayersamples[0][cache_record],sizeof(mplayersamples));
//

#include <sys/mman.h>
int haveforked=FALSE;
#define CACHESIZE		FRAMES_PER_SECOND * 5							// trade off seconds against CPU overhead of cache management

struct sharedmemrecord 
{
	char message;											// 'q' quit
	int flag_firstfill;										// true if cache is initially filling
	int flag_readnow;										// unlocks the cache process to read some more
	int cachefill;											// number of chunks of audio in cache
	int surplus_chunks;										// pass back to parent audioin fifo fill level
	int buffer_number[CACHESIZE];									// each chunk of audio has a number
        int16 mplayersamples[NUM_SAMPLES*2][CACHESIZE]; 					      	// 44.1Khz stereo interleaved data

	// Uncomment for debugging
	//int32 mplayersamples_checksum[CACHESIZE];							// DEBUGGING, checksum for each audio sample chunk
	//int read_order[CACHESIZE];									// each read() increments it, now we can check frames come out in order
}; 
struct sharedmemrecord *samplecache=NULL;								// pointer to some memory



void cache_calc_fill()
{
	int i;
	int cache_records_filled=0;								// How many cache records have pending data in them (cache fill)

	cache_records_filled=0;	
	for (i=0;i<CACHESIZE;i++)
		if (samplecache->buffer_number[i]!=-1)
			cache_records_filled++;
	samplecache->cachefill=cache_records_filled;
}


int cache_find_oldest()
{
	int i;
	int cache_oldest=0;
	int cache_record=0;

	cache_oldest=INT_MAX;										// everything should be larger than the largest possible number
	cache_record=-1;										// the cache records are 0 to 7 if CACHESIZE is 8
	for (i=0;i<CACHESIZE;i++)									// find oldest cache entry, this is what we want to play
	{
		if (samplecache->buffer_number[i]!=-1)							// only active cache entries
		{
			if (samplecache->buffer_number[i]<cache_oldest)					// is this one newer (lower number)?
			{
				cache_oldest=samplecache->buffer_number[i];				// then currently its the oldest
				cache_record=i;
			}
		}
	}
	return(cache_record);
}



pid_t	mypid;
int read_audio_samples_caching(int16 *audiochunk)
{
	int record=0;
	int numbytes=0;
	unsigned int counter=0;
	int cache_record;
	int i;
	int16 somesamples[NUM_SAMPLES *2];
	int flag_initial;

	// whenever we fork we get a nice clean cache, no need to clear the cache we just tell the child to terminate and make a new one
	if (haveforked!=TRUE)										// not forked yet?
	{
		if (samplecache != NULL)								// mmap does a malloc,is the memory still hanging around from last time ?
		{
			munmap (samplecache,sizeof(struct sharedmemrecord));				// free it
			samplecache=NULL;								// show everyone its free
		}

    		samplecache = mmap(NULL, sizeof(struct sharedmemrecord), PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);	// memory shared between parent and child
    		if (samplecache == MAP_FAILED)
		{
			fprintf(stderr,"%s: cacheing_audio_read() - mmap failed, cant create shared memory for my child\n",PROGNAME);
			fflush(stderr);
			exit(1);
		}

		samplecache->message='x';								// something harmless
		for (i=0;i<CACHESIZE;i++)
			samplecache->buffer_number[i]=-1;						// mark all slots as free
		mypid=fork();
		haveforked=TRUE;									// both parent and child know we forked
		if (mypid!=0)
			return(-2);
	}

	if (mypid<0)
	{
		fprintf(stderr,"%s: cacheing_audio_read() -  bugger, forked failed\n",PROGNAME);
		fflush(stderr);
		exit(1);
	}

	if (mypid == 0)											// I am the child
	{
		printf("cache_reader: I am your child, hello\n");
		fflush(stdout);
		// As I am the child I can block to my hearts content, what I need to do is read audio chunks and place them in a ring buffer
		// so the parent knows which is oldest I can number them
	
		//close(fd_audioin);									// will work blocking, but then fifo fill level always 100%
        	//fd_audioin=open(FIFO_AUDIO, O_RDONLY);						// Open it blocking this time

		// Documentation for fork implies this is not required, it is 
 		signal(SIGINT, SIG_DFL);								// restore default signal handler for this process, die cleanly on interrupt 
		counter=0;
		record=0;
		samplecache->cachefill=-1;
		samplecache->flag_readnow=TRUE;								// unlock the child so it can read stuff
		samplecache->flag_firstfill=TRUE;							// newly started process so we are filling
		flag_initial=TRUE;
		while (TRUE)
		{
			while (samplecache->flag_readnow != TRUE)
				usleep(500);								// loop waiting to be unlocked

			if (flag_initial == TRUE)							// only if this is the first fill
			{
				if (samplecache->cachefill <( CACHESIZE) )				// full now, isfilling is false
					samplecache->flag_firstfill=TRUE;				// we are filling
				else	samplecache->flag_firstfill=FALSE;				// not full enough yet, don't start playback
			}

			if (samplecache->message == 'q')
			{
				printf("cache_reader: Parent asked me to terminate\n");
				samplecache->message='x';
				exit(0);								// don't munmap here, let the parent do that
			}

			numbytes=read(fd_audioin,&somesamples,sizeof(mplayersamples)); 			// Read a block of audio data
			if (numbytes==sizeof(mplayersamples))						// got a chunk of audio ?
			{
				//printf("cache_reader: got %d bytes from fifo\n",numbytes);
				//fflush(stdout);

				record=0;
				while (samplecache->buffer_number[record]!=-1)				// find a free slot
					record++;

				if ( (record>=CACHESIZE) | (record<0) )
				{
					fprintf(stderr,"cache_reader: Error, free slot = %d - must be >=0 <=%d\n",record,CACHESIZE);
					fflush(stderr);
					goto skip;
				}

        			//samplecache->mplayersamples_checksum[record]=audiochunk_checksum(somesamples);	// DEBUGGING only
				//samplecache->read_order[record]=counter;				// Debugging, record frame counter

				for (i=0;i<(NUM_SAMPLES * 2);i++)
					samplecache->mplayersamples[i][record]=somesamples[i];		// memcpy didn't work
				samplecache->buffer_number[record]=counter;				// mark this record as having active contents
				counter++;
			}
			else
			{
				if (numbytes>=1)							// ignore -1 or 0 
					printf("cache child, did short read of %d bytes expected %d bytes\n",numbytes,(int)sizeof(mplayersamples));
				fflush(stdout);
			}

// WASTING CPU, could do better here
// changed timings 4 Aug 2013
			cache_calc_fill();
			if (samplecache->cachefill>=CACHESIZE)		//correct
			{
				samplecache->flag_readnow=FALSE;					// lock myself, we are full no point in reading until some cache is used
				flag_initial=FALSE;
				samplecache->flag_firstfill=FALSE;					// cache is full so its no longer the initial cache fill
			}

skip:			samplecache->surplus_chunks=read_fifo_fill_level();
			if ( (samplecache->cachefill > CACHESIZE - (CACHESIZE / 3)) & (samplecache->flag_firstfill != TRUE) )
			{
				//printf("z"); fflush(stdout);
				usleep(10000);								// be lazy about it if cache is pretty full
			}
			else	usleep(200);								// must have some sleep here or process stalls parent if realtime is set
		}
	}
	else												// I am parent, control my child and read results
	{
		//printf("fifofill=%d   samplecache->cachefill=%d  oldest=%d\n",samplecache->surplus_chunks,samplecache->cachefill,samplecache->oldest); fflush(stdout);
		cache_calc_fill();									// every time we go for a read update the fill level

		if ( samplecache->cachefill < CACHESIZE - (CACHESIZE/2 ) )				// if cache is less than 3/4 full 
			samplecache->flag_readnow = TRUE;						// unlock the child to read until the cache is full

		if (samplecache->flag_firstfill == TRUE)						// still doing the initial cache fill
			return(-2);									// timeout until the cache is filled

		cache_record=cache_find_oldest();
		if (cache_record != -1)									// copy oldest cache record to be played
		{	
			// DEBUGGING
			//audio_read_checksum=samplecache->mplayersamples_checksum[cache_record];	// DEBUG, get checksum child calculated
			//printf("read_order=%d\n",samplecache->read_order[cache_record]);		// are we returning consecutive frames of audio, should be !
			//fflush(stdout);

		 	for (i=0;i< (NUM_SAMPLES * 2);i++)
				audiochunk[i]=samplecache->mplayersamples[i][cache_record];		// memcpy didn't work

			i=samplecache->cachefill;							// the order matters. Tell child it has one less record
			i--;										// remember two processes here, copy the variable, subtract 1, write it back
			if (i<0)
				i=0;
			samplecache->cachefill=i;							// the order matters. Tell child it has one less record
			audioin_surplus_chunks=samplecache->cachefill;					// surplus chunks is cache fill level in read mode 'c', on screen with -dg
			samplecache->buffer_number[cache_record]=-1;					// mark cache record as used (is now free)
			samplecache->flag_readnow = TRUE;						// unlock the child to read until the cache is full
			return(sizeof(mplayersamples));							// tell caller we read a chunk of audio
		}
		else
		{
			//printf("N"); fflush(stdout);
			bzero(audiochunk,sizeof(mplayersamples));					// return silence
			return(-2);									// if no entries then tell caller we timeout
		}
	}
}
#endif	



// ******************************************************************************
// wrapper function, we always call this function, it calls one of the above
// methods to read real data
// returns -2 for timeout or the number of bytes read, should be sizeof(mplayersamples)
int read_audio(int rmethod,void *audiochunk)
{
	int ret=-2;

	switch (rmethod)
	{
		case 's'  :  
			ret=read_audio_samples_simple(audiochunk);
			//audio_read_checksum=audiochunk_checksum(audiochunk);			// DEBUGGING
		break;

		case 'f' :
			ret=read_audio_samples_full(audiochunk);
			//audio_read_checksum=audiochunk_checksum(audiochunk);			// DEBUGGING
		break;
#ifndef EMPEG
		case 'c' :
			ret=read_audio_samples_caching(audiochunk);
			if (ret==-2)
				usleep(500);
		break;
#endif
	
		default:
			fprintf(stderr,"%s: read_audio() - Unknown audio read method [%c]\n",PROGNAME,rmethod);
	}
	return(ret);
}



// ******************************************************************************
void print_audio_read_method(int rmethod)
{
	if (player_mode == PLAYER_SLAVE)
	{
		printf("Slave mode, UDP audio+ data");
		return;
	}

	switch (rmethod)
	{
		case 's'  : 	printf("Simple");
		break; 
		case 'f'  : 	printf("Full");
		break; 
		case 'c'  : 	printf("Caching");
		break; 
		default   :	printf("No idea!!");
	}
}


// ******************************************************************************
// Send a message to the child, q=quit
void audio_read_flush_cache(int rmethod, char message)
{
#ifndef EMPEG
	int i;
	int timeout=400;
#endif

	printf("%s: audio_read_flush_cache() - send shared memory cache process a message [%c]\n",PROGNAME,message);
	if (rmethod!='c')									// this code only applies to caching reads
		return;

#ifndef EMPEG
	if (message!='x')									// if a real message
	{
		if (message=='q')								// tell player to quit
		{
			do									// wait for child to ack message my replacing it with an 'x'
			{
				samplecache->message = message;					// send child a message
				samplecache->flag_readnow = TRUE;				// unlock child so it can action the message
				usleep(200);
				timeout--;
				reap_all();
			} while ( (timeout>0) & (samplecache->message != 'x') );
			if (timeout<=0)								// did we timeout, say so and kill the child
			{
                		kill (SIGCLD,mypid);						// send child an exit now signal
				printf("%s: Timeout waiting for child to ack message [%c], send SIGCLD to pid %d\n",PROGNAME,message,mypid);
			}

			haveforked=FALSE;							// don't call read_audio or you will get another child
			samplecache->cachefill=-1;
			for (i=0;i<CACHESIZE;i++)
				samplecache->buffer_number[i]=-1;				// mark all slots as free
		}
	}
#endif
}


