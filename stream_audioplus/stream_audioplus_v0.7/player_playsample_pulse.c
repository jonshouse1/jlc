
void print_audio_read_method(int rmethod);                                      // needs this


// Used by the visuals to determine the delay
int get_playback_latency_ms()
{
	int error;
 	pa_usec_t latency;

        latency = pa_simple_get_latency(s, &error);
	return(latency / 1000 );
}




void playsample(void *bufptr, int bufsize)
{
 	pa_usec_t latency;
	int error;

        //latency = pa_simple_get_latency(s, &error);
	if (latency<0)
	{
            	fprintf(stderr, __FILE__": pa_simple_get_latency() failed: %s\n", pa_strerror(error));
		exit(1);
        }


	// ALWAYS write the newest sample to pulse, we might have just flushed its buffer
	//gettimeofday(&tv1,NULL);
	if (VERBOSE>=2)
	{
		fprintf(stderr,"%s: playsample() - pa_simple_write\n",PROGNAME);	
		fflush(stderr);
	}

        if ( pa_simple_write(s, bufptr, bufsize, &error) <0)
	{
            	fprintf(stderr,"%s: pa_simple_write() failed : %s\n",PROGNAME, pa_strerror(error));
		exit(1);
	}


	// Calculate duration of call in useconds
	//gettimeofday(&tv2,NULL);
	//float calltime=(tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
}
