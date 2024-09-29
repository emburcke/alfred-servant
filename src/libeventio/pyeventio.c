#define _GNU_SOURCE		// for pipe2, needs to defined here because otherwise the Python.h will define it to a bad value
#define Py_LIMITED_API 3	// nem kell unstable cucc. majd en irok sajat unstable cuccot
#define PY_SSIZE_T_CLEAN
// TODO sort 
#include <sys/types.h>		// for pid_t
#include <sys/wait.h>		// for wait (who got suprised?)
#include <Python.h>		// for python
#include <unistd.h>		// malloc calloc, free, write ...
#include <pthread.h>		// for threading
#include <stdint.h>		// for fancy ints
#include <string.h>		// this is not a friend, but needed for strdup., strlen, memcpy
#include <fcntl.h>		// for file flags
#include <errno.h>		// for errno
#include <sys/stat.h>		// For mode constants
#include <mqueue.h>		// for message queue

// -shared = shared libarivá
// -fPIC == sok dolog de kb csak az hogy a pointerek relativak
// -pthread == tobszalusag libet betolti
// -Wall all warnings
// -lrt link with the real time library. we ned this for mqueue
/*
gcc -Wall -Wextra -I/usr/include/python3.8 --shared -fPIC -pthread pyeventio.c -lrt -o libeventio.so
*/
// a -lrt minden file után kell, because FUCK YOU thats why
//TODO a delete fugevnyt megcsinalni

#include "error_handler.h"		// for handling errors, that are occured in threads
#include "py_blockingfor.h"
#include "py_call_child.h"
#include "py_posix_ipc.h"

static PyMethodDef methodstable[] = {
	{
	"call_child",	/* fuggveny pythobol hivhato neve */
	call_child,	/* fuggveny C neve (fuggvenyre mutato pointer) */
	METH_VARARGS,	/* flag: hogyan menjen a call */
			/* METH_VARARGS: The function expects two PyObject* values. */
			/* METH_FASTCALL: only positional arguments. */
			/* METH_O: Methods with a single object argument */
	"call_child([bytes,bytes,...],data) -> pid\n call a child process then writes all data to it. the process output later can be get with blockingfor." // doksi
	},
	{"blockingfor_pyget",blockingfor_pyget,METH_NOARGS,"blockingfor_pyget() -> (pid,str/int)\nget the next messagge, if there are none, wait for one."},
	{"listen_to_message",listen_to_message,METH_VARARGS,"listen_to_message(name:bytes,[max_messages:int,[max_message_size:int]])-> None\nsign up for the new -or existing- message que with name. the even later can collected with blockingfor."},
	{NULL, NULL, 0, NULL}  /* terminate null record */
};

static void ondelete(void * Py_UNUSED(self)){
//pass az a frozen importlib python objektumra mutato pointer.
	return;
}

static struct PyModuleDef modeuleStruct = {
	PyModuleDef_HEAD_INIT,	/* kotelezoen ez */
	"eventio",			/* modul neve */
	"alfred eventio for fast events", /* doc string */
	-1,				/* Py_ssize_t m_size */
	methodstable,		/* A pointer to a table of module-level functions */
	NULL,			/* tobbfazius inicializalasnal lista */
	NULL,			/* Garbage Collector traversal function */
	NULL,			/* GC clear (free) */
	ondelete		/* whole module GC free */
};

#define INITMUTEX(name) if (pthread_mutex_init(&name,PTHREAD_MUTEX_NORMAL)) {PyErr_SetString(PyExc_OSError, "failed to init " Py_STRINGIFY(name) " mutex");return NULL;}

/* Module initialization */
PyMODINIT_FUNC
PyInit_libeventio(void)
{
	INITMUTEX(child_data_lock);
	INITMUTEX(input_que_lock);
	INITMUTEX(blocking_lock);
	INITMUTEX(is_waiting_for_child);

	return PyModule_Create(&modeuleStruct);
}
