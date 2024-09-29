#ifndef __PYALFRED_BLOCKINGFOR
#define __PYALFRED_BLOCKINGFOR 0x001000		// mayor minor patch size=2

typedef struct stringorint_s // if the type is 0 then the data is string else the data is an fd stored in the len attribute, and the pid is stored in the type attribute
{
	char *data;
	Py_ssize_t len;
	pid_t type;
}stringorint_t;

typedef struct input_que_s
{
	struct input_que_s *next;
	stringorint_t *data;
} input_que_t;

input_que_t *input_que_start=NULL;
input_que_t *input_que_end=NULL;
pthread_mutex_t input_que_lock;
pthread_mutex_t blocking_lock;

static PyObject* blockingfor_pyget()
{
	stringorint_t *out;
	input_que_t *item;
	PyObject *pyout;

	Py_BEGIN_ALLOW_THREADS
	pthread_mutex_lock(&input_que_lock);
	if (NULL == input_que_end)
	{
		pthread_mutex_lock(&blocking_lock);
		pthread_mutex_unlock(&input_que_lock);
		pthread_mutex_lock(&blocking_lock);
		pthread_mutex_unlock(&blocking_lock);
		pthread_mutex_lock(&input_que_lock);
	}
	Py_END_ALLOW_THREADS
	if (NULL == input_que_start)
	{
		Py_UNREACHABLE();
	}
	item=input_que_start;
	input_que_start=item->next;
	if (NULL == input_que_start)
	{
		input_que_end=NULL;
	}
	pthread_mutex_unlock(&input_que_lock);
	out=item->data;
	free(item);
	if (!out->type)
	{
		pyout=Py_BuildValue("sy#",NULL,out->data,out->len); // None ,bytes(data)
		free(out->data);
	}
	else
	{
		pyout=Py_BuildValue("in",out->type,out->len);	// pid,fd
	}
	free(out);
	return pyout;
}
int blockingfor_add_str(char *data,Py_ssize_t len)	// returns 0:ok 1:malloc memory error 2:malloc memory error no2
{
	input_que_t *item;
	item=malloc(sizeof(input_que_t));
	if (NULL == item)
	{
		return 1;
	}
	item->data=malloc(sizeof(stringorint_t));
	if (NULL == item->data)
	{
		return 2;
	}
	item->data->data=data;
	item->data->len=len;
	item->data->type=0;
	item->next=NULL;

	pthread_mutex_lock(&input_que_lock);
	if (NULL == input_que_end)
	{
		input_que_start=item;
		input_que_end=item;
		pthread_mutex_unlock(&input_que_lock);
		pthread_mutex_trylock(&blocking_lock);
		pthread_mutex_unlock(&blocking_lock);
	}
	else
	{
		input_que_end->next=item;
		input_que_end=item;
		pthread_mutex_unlock(&input_que_lock);
	}

	return 0;
}
int blockingfor_add_pidfd(pid_t pid,int fd)
{
	input_que_t *item;
	item=malloc(sizeof(input_que_t));
	if (NULL == item)
	{
		return 1;
	}
	item->data=malloc(sizeof(stringorint_t));
	if (NULL == item->data)
	{
		return 2;
	}
	item->data->data=NULL;
	item->data->len=fd;
	item->data->type=pid;
	item->next=NULL;

	pthread_mutex_lock(&input_que_lock);
	if (NULL == input_que_end)
	{
		input_que_start=item;
		input_que_end=item;
		pthread_mutex_unlock(&input_que_lock);
		pthread_mutex_trylock(&blocking_lock);
		pthread_mutex_unlock(&blocking_lock);
	}
	else
	{
		input_que_end->next=item;
		input_que_end=item;
		pthread_mutex_unlock(&input_que_lock);
	}

	return 0;
}

#endif /* __PYALFRED_BLOCKINGFOR */
