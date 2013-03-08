#ifndef _PYRTUTILS_
#define _PYRTUTILS_

#include <Python.h>
#include "RtAudio.h"

static char *sint8 = "b";
static char *sint16 = "h";
static char *sint32 = "l";
static char *float32 = "f";
static char *float64 = "d";
static char *dflt = "b";

void setRuntimeExceptionWithMessage(char const *message) {
    PyErr_SetString(PyExc_RuntimeError, message);
}

char *getTypecodeAsString(RtAudioFormat format) {
    char *typecode;
    switch (format) {
        case RTAUDIO_SINT8:
            typecode = sint8;
            break;
        case RTAUDIO_SINT16:
            typecode = sint16;
            break;
        case RTAUDIO_SINT24:
        case RTAUDIO_SINT32:
            typecode = sint32;
            break;
        case RTAUDIO_FLOAT32:
            typecode = float32;
            break;
        case RTAUDIO_FLOAT64:
            typecode = float64;
            break;
        default:
            typecode = dflt;
            break;
    }
    return typecode;
}

Py_ssize_t getSizeFromFormat(RtAudioFormat format) {
    Py_ssize_t size;
    switch (format) {
        case RTAUDIO_SINT8:
            size = 1;
            break;
        case RTAUDIO_SINT16:
            size = 2;
            break;
        case RTAUDIO_SINT24:
        case RTAUDIO_SINT32:
        case RTAUDIO_FLOAT32:
            size = 4;
            break;
        case RTAUDIO_FLOAT64:
            size = 8;
            break;
        default:
            size = 1;
            break;
    }
    return size;
}


Py_buffer *getReadWriteBufferObject(int nitems, RtAudioFormat format) {
    Py_buffer *buf = (Py_buffer *) malloc(sizeof(*buf));
    char *typecode = getTypecodeAsString(format);
    Py_ssize_t itemsize = getSizeFromFormat(format);
    buf->obj = NULL;
    buf->buf = malloc(nitems * itemsize);
    buf->len = nitems * itemsize;
    buf->readonly = 0;
    buf->itemsize = itemsize;
    buf->format = typecode;
    buf->ndim = 1;
    buf->shape = NULL;
    buf->strides = NULL;
    buf->suboffsets = NULL;
    buf->internal = NULL;
    return buf;
}

char getTypecodeForPythonArray(RtAudioFormat format) {
    char typecode;
    switch (format) {
        case RTAUDIO_SINT8:
            typecode = 'b';
            break;
        case RTAUDIO_SINT16:
            typecode = 'h';
            break;
        case RTAUDIO_SINT24:
        case RTAUDIO_SINT32:
            typecode = 'l';
            break;
        case RTAUDIO_FLOAT32:
            typecode = 'f';
            break;
        case RTAUDIO_FLOAT64:
            typecode = 'd';
            break;
        default:
            typecode = 'b';
            break;
    }
    return typecode;
}

RtAudio::StreamParameters *populateStreamParameters(PyObject *dict) {
    PyObject *device = PyDict_GetItemString(dict, "device_id");
    PyObject *channels = PyDict_GetItemString(dict, "channels");
    PyObject *first = PyDict_GetItemString(dict, "first_channel");
    if (!device || !channels || !first)
        return NULL;
    if (!PyInt_Check(device) || !PyInt_Check(channels) || !PyInt_Check(first))
        return NULL;

    RtAudio::StreamParameters *params = new RtAudio::StreamParameters;
    params->deviceId = PyInt_AsLong(device);
    params->nChannels = PyInt_AsLong(channels);
    params->firstChannel = PyInt_AsLong(first);

    return params;
}

long getReturnValue(PyObject *returnValue) {
    if (!PyInt_CheckExact(returnValue)) {
        setRuntimeExceptionWithMessage("Callback returned non-integer type");
        return -1;
    }

    return PyInt_AsLong(returnValue); 
}


#endif
