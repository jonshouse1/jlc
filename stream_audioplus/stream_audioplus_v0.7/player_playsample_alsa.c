// Poll the sound playback system to see if it needs more data. We use this flag as
// the clock to generate the data.  IE when needs_data is true we mix some audio
// and play it back.  This way ALSA drives our output clock. 
// for pulse use system timer ?
//
//

void print_audio_read_method(int rmethod);					// needs this


// How full is the cache alsa is playing from 
int get_playback_latency_ms()
{
 	int framesleft;
	//int ms_in_buffer;
	framesleft=alsa_bufsize-snd_pcm_avail (playback_handle);		// How many samples behind is the player


	return( (framesleft*1000)/playback_sample_rate );			// frames to milliseconds
}



// snd_pcm_avail returns the number of samples required to refresh the playback buffer
// we simply say if has room for another 1000 then its time to stick in another 512 
int sound_system_needs_data_now()
{
	int res;
 	int framesleft;
	snd_pcm_status_t *status = NULL;
   	snd_pcm_status_alloca( &status );					// alloca allocates once per function, does not need a free


	if ((rc=snd_pcm_status(playback_handle,status))!=0)
	{
    		fprintf(stderr,"%s: sound_system_needs_data_now()  - snd_pcm_status() failed %d : %s\n",PROGNAME,rc,snd_strerror(rc));
		fflush(stderr);
    		return(TRUE);
    	}
	res=snd_pcm_status_get_state(status);

	if (res!=SND_PCM_STATE_RUNNING)						// If sound system is not running then it needs data
		return(TRUE);

	framesleft=snd_pcm_avail (playback_handle);
	//printf("res=%d FL=%d\n",res,framesleft); fflush(stdout);
	if (framesleft<2078)
		return(TRUE);
	else
		return(FALSE);
}




int sound_system_needs_data_nowOLD()
{
	int rval;
	rval=snd_pcm_wait(playback_handle,0);					// just return yes or no
	//printf("rval=%d  ",rval);
	if (rval==1)								// invert result
		rval=TRUE;
	else	rval=FALSE;
	return(rval);
}



void playsample(void *bufptr, int bufsize)
{
	//int canswallow=0;
	int numrecs=0;
	int reta=0;								// Don't use snd_pcm_sframes_t or if < will fail
	int retb=0;
	char st[1024];


	numrecs=512;								// correct value IS 512
	//numrecs=bufsize;							// according to most example, doesn't work

	//canswallow =  snd_pcm_avail(playback_handle); 	
	//printf("canswallow=%d\n",canswallow); fflush(stdout);

//DEBUG
	// check audio written is same as audio when initially read
        //audio_write_checksum=audiochunk_checksum((int16 *)bufptr);
	//if ( (audio_read_checksum != audio_write_checksum) & (audio_write_checksum !=0 ) )		// sometimes we write silence but that is not an error
	//{
		//printf("!!CS Mismatch,  audio_read_checksum = %08X\taudio_write_checksum=%08X\n",audio_read_checksum,audio_write_checksum);
		//fflush(stdout);
	//}
//JA


	reta = snd_pcm_writei (playback_handle, bufptr, numrecs);		// Write interleaved

	if (VERBOSE>=3)
	{
		fprintf(stderr,"%s: playsample() - snd_pcm_writei returned %d\n",PROGNAME,reta); 
		fflush(stderr);
	}
	if (reta<numrecs) 
	{
		if (reta>0)
		{
			fprintf (stderr, "%s: playsample() - snd_pcm_writei: wrote only %d\n",PROGNAME,reta); 
			fflush(stderr);
		}
		else
		{
			//printf("%s: playsample() - Error (%s)(%d)\n",PROGNAME,snd_strerror (reta),reta);
			fflush(stdout);
			if (reta == -32)							// if broken pipe -32
			{
				fprintf(stderr,"%s: playsample() - underrun,  ",PROGNAME); 
	                	soundoutput_underruns++;
				retb=snd_pcm_recover(playback_handle,reta,0);			// error passed is result of writei
				if (retb<0)
				{
					fprintf(stderr,"Failed to recover from underrun\n");
					fprintf(stderr, "[%s]\n", snd_strerror (retb));
					fflush(stderr);
					exit(1);
				}
				else
				{
					if (audioin_surplus_chunks >= 0)			// if audio read supported this statistic
					{
						datetime(&st, FALSE);
						if (audioin_surplus_chunks <= 1)		// oh dear, fifo was empty
							fprintf(stderr,"%s audio pipe was empty [%d chunks], mp3decoder slow or reading too slowly",st,audioin_surplus_chunks);
						else	fprintf(stderr,"%s audio pipe had data [%d chunks], something is blocking",st,audioin_surplus_chunks);
					}
					fprintf(stderr,"\n");
					fflush(stderr);
					printf("%s: ALSA recovered, current audio read method is [",PROGNAME);   print_audio_read_method(audio_read_method);  printf("]\n");
				}
			}
			else
			{
				soundoutput_overruns++;
				fprintf (stderr, "%s: playsample() - snd_pcm_writei: (overrun) write to audio interface failed (%s) (%d)\n",PROGNAME, snd_strerror (reta),reta);

				fflush(stderr);
				//if (retb !=-11)							// was not an error we can recover from
					//exit (1);
			}
		}
	}
}

