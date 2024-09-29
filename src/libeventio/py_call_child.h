#ifndef __PYALFRED_CALL_CHILD
#define __PYALFRED_CALL_CHILD 0x100000		// == 1.0.0 ; each number is take out 2 space

typedef struct pidfd_s
{
	struct pidfd_s *next;
	pid_t pid;
	int fd;
} pidfd_t;

pidfd_t *child_data_start;
pidfd_t *child_data_end;
pthread_mutex_t child_data_lock;
pthread_mutex_t is_waiting_for_child;

static void *waitchilds(void *Py_UNUSED(arg))
{
	int statusholder;
	pid_t pid;
	pidfd_t *now;
	pidfd_t *last=NULL;

	while (1)
	{
		pid=wait(&statusholder);
		pthread_mutex_lock(&child_data_lock);
		now=child_data_start;
		while (NULL != now)
		{
			if (now->pid == pid)
			{
				break;
			}
			last=now;
			now=last->next;
		}
		if (NULL == now)
		{
			pthread_mutex_unlock(&child_data_lock);
			dprintf(2,"%d pid waited, but we dont found it's output file handler.\n",pid);
			continue;
			// we simply forget about this pid, because we dont have the right fd for it.
		}
		if (NULL == last)
		{
			child_data_start=NULL;
			child_data_end=NULL;
		}
		else {
			last->next=now->next;
			if (NULL == last->next)
			{
				child_data_end=last;
			}
		}
		pthread_mutex_unlock(&child_data_lock);
		blockingfor_add_pidfd(now->pid,now->fd);
		free(now);
		pthread_mutex_lock(&child_data_lock);
		if (NULL == child_data_end)
		{
			pthread_mutex_unlock(&is_waiting_for_child);
			pthread_mutex_unlock(&child_data_lock);
			return NULL;
		}
		pthread_mutex_unlock(&child_data_lock);
	}
}

static PyObject* call_child(PyObject * Py_UNUSED(self), PyObject* args)
{
	// TODO decrease stack size with unions
	char *child_data;
	Py_ssize_t child_data_len;

	char **c_exec_argv;
	PyObject *exec_argv;
	Py_ssize_t exec_argv_len;

	Py_ssize_t index;
	char *c_item;
	PyObject *item;
	Py_ssize_t item_lenght;

	pid_t pid;
	int readfd;
	int infd[2];
	int outfd[2];

	ssize_t writed;
	pthread_t threadholder;

	pidfd_t *out_data=malloc(sizeof(pidfd_t));
	if (NULL == out_data)
	{
		return PyErr_NoMemory();
	}
	out_data->next=NULL;

	if (!PyArg_ParseTuple(args, "Os#",&exec_argv,&child_data,&child_data_len))
		return NULL;
	if (!PyList_CheckExact(exec_argv))
	{
		PyErr_SetString(PyExc_ValueError, "The first parameter must be list, not even a subclass is allowed");
		return NULL;
	}
	exec_argv_len=PyList_Size(exec_argv);
	if (!exec_argv_len)
	{
		PyErr_SetString(PyExc_ValueError, "There must be at least one argv parameter so we can run it");
		return NULL;
	}
	c_exec_argv=calloc(exec_argv_len + 1,sizeof(char *));
	if (NULL == c_exec_argv)
	{
		return PyErr_NoMemory();
	}
	c_exec_argv[exec_argv_len]=NULL; // terminating zero.
	for (index=0;index < exec_argv_len;index++)
	{
		item=PyList_GetItem(exec_argv,index);
		if (NULL == item)
		{
			free(c_exec_argv);
			return NULL;
		}
		if (PyBytes_AsStringAndSize(item,&c_item,&item_lenght) == -1)
		{
			free(c_exec_argv);
			return NULL;
		}
		c_exec_argv[index]=strdup(c_item); // safe to call, because the PyBytes_AsStringAndSize already checked if the string is contained any zeros, and added a terminating.
		if (NULL == c_exec_argv[index])
		{
			return PyErr_NoMemory();
		}
	}
	if (-1 == pipe2(infd,O_CLOEXEC))
	{
		return PyErr_SetFromErrno(PyExc_OSError);
	}	
	if (-1 == pipe2(outfd,O_CLOEXEC))
	{
		return PyErr_SetFromErrno(PyExc_OSError);
	}
	pid=fork();	// haha python, its forking time.  ;; use this function from main thread.
	if (0 > pid) // error
	{
		PyErr_SetString(PyExc_OSError, "failed to fork.");
		free(c_exec_argv);
		return NULL;
	}
	if (! pid)
	{
		dup2(infd[0],0); // in to stdin
		dup2(outfd[1],1); // out to stdout
		// we dont need to close anything because exec do it for us
		execv(c_exec_argv[0],c_exec_argv);
	}
	free(c_exec_argv);
	close(infd[0]);
	close(outfd[1]);
	readfd=outfd[0];
	fcntl(infd[1], F_SETFL, O_NONBLOCK ); // so the write returns instantly
	index=0;
	while (1)
	{
		writed=write(infd[1],child_data + index,child_data_len - index);
		if (-1 == writed)
		{
			return PyErr_SetFromErrno(PyExc_OSError);
		}
		index=index + writed;
		if (index >= child_data_len)
		{
			close(infd[1]);
			out_data->pid=pid;
			out_data->fd=readfd;
			pthread_mutex_lock(&child_data_lock);
			if (NULL == child_data_end)
			{
				child_data_start=out_data;
			}
			else
			{
				child_data_end->next=out_data;
			}
			child_data_end=out_data;
			pthread_mutex_unlock(&child_data_lock);
			if (!pthread_mutex_trylock(&is_waiting_for_child))
			{
				pthread_create(&threadholder,NULL,waitchilds,NULL);
				pthread_detach(threadholder);	// every thread needs to be joined or detachde so their values can be freed see: man 'PTHREAD_DETACH(3)'
			}
			return Py_BuildValue("i",pid);
		}
	}
}
#endif /* __PYALFRED_CALL_CHILD */
