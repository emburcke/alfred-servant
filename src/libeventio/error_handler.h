#ifndef __PYALFRED_ERROR_HANDLING
#define __PYALFRED_ERROR_HANDLING 0x001000

static void allthread_fatal_memory_error(char *text,size_t len)
{
	write(2,text,len);	// because we encountered a memory error, we dont check if the write actually writed anything, we just hope that it writed
	_exit(1);		// exit() calls atexit, and so on, but we runed out of memory here, so we close as soon as we can
}
#define onethread_fatal_memory_error allthread_fatal_memory_error	// all means that all threads concerned, one means that we only need that thread to be shut down. in fatal_memory error, this is the same
static void onethread_fatal_errno_error(char *describer)
{
	perror(describer);
	pthread_exit(NULL);
	return;
}
/*
static void allthread_fatal_errno_error(char *describer)
{
	perror(describer);
	_exit(1);
	return;
}*/	// no one use it, commented

#endif /* __PYALFRED_ERROR_HANDILG */
