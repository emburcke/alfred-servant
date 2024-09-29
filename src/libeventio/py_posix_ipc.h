#ifndef __PYALFRED_POSIX_IPC
#define __PYALDRED_POSIX_IPC 0x100000	// major minor patch

typedef struct message_arg_s
{
	size_t max_message_size;
	long max_messages;
	mqd_t mqd;
} message_arg_t;

static void *message_listener(void *arg)
{
	ssize_t size;
	char *msg=NULL;
	char *data=NULL;
	unsigned int priority;
	message_arg_t *self=(message_arg_t *) arg;


	msg=malloc(self->max_message_size);
	if (NULL == msg)
	{
		onethread_fatal_memory_error("Can not allocate memory for new message\n",40);	// text, textsize
	}

	while (1)
	{
		size=mq_receive(self->mqd, msg, self->max_message_size, &priority);
		if (-1 == size)
		{
			if (EINTR == errno) // interrupted
			{
				continue;
			}
			free(msg);
			onethread_fatal_errno_error("a thread local fatal error occured at message queue recive: exiting thread");
		}
		else if (0 == size)
		{
			continue;	// we pass the 0 sized messages, as they don't contain any data
		}
		data=malloc(size);
		if (NULL == data)
		{
			onethread_fatal_memory_error("Can not allocate memory for the exact message\n",46);
		}
		data=memcpy(data,msg,size);
		if (0 != blockingfor_add_str(data,(Py_ssize_t) size))
		{
			onethread_fatal_memory_error("Can not allocate memory for the nem que\n",40);
		}
	}
	return NULL;
}

static PyObject* listen_to_message(PyObject * Py_UNUSED(self), PyObject* args)
{
	pthread_t threadholder;
	struct mq_attr attr;
	char *name;
	message_arg_t *message;
	
	message=malloc(sizeof(message_arg_t));
	if (NULL == message)
	{
		return PyErr_NoMemory();
	}

	message->mqd=(mqd_t)-1;
	message->max_messages=10;
	message->max_message_size=8192;

	if (!PyArg_ParseTuple(args, "y|ll",&name,&(message->max_messages),&(message->max_message_size)))
	{
		free(message);
		return NULL;
	}
	if (strlen(name) < 2)
	{
		free(message);
		PyErr_SetString(PyExc_ValueError, "name needs to contain at least 2 char");
		return NULL;
	}
	if ('/' != name[0])
	{
		free(message);
		PyErr_SetString(PyExc_ValueError, "name must start with /");
		return NULL;
	}
	if ( (strlen(name) +1) > NAME_MAX)
	{
		free(message);
		PyErr_SetString(PyExc_ValueError, "the maximum size of name is " Py_STRINGIFY(NAME_MAX));
		return NULL;
	}

	// init attr
	attr.mq_flags=0;
	attr.mq_maxmsg=message->max_messages;
	attr.mq_msgsize = message->max_message_size;
	attr.mq_curmsgs = 0;

	message->mqd = mq_open(name, O_CREAT | O_RDONLY, 0600, &attr);
	if ((mqd_t)-1 == message->mqd)
	{
		free(message);
		PyErr_SetFromErrno(PyExc_OSError);
		return NULL;
	}
	message->max_messages=attr.mq_maxmsg;		// soft values can be ceil the given values so we retrive them. (i don't know if this true, but i keep it)
	message->max_message_size=attr.mq_msgsize;

	pthread_create(&threadholder,NULL,message_listener,message);
	pthread_detach(threadholder);		//we don't need it's return value therefore this is a detached thread, so it will clean up itself
	Py_RETURN_NONE;
}

#endif /*__PYALDRED_POSIX_IPC*/
