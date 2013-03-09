#ifndef _PYRTUTILS_
#define _PYRTUTILS_

#include <Python.h>
#include "RtAudio.h"

void setRuntimeExceptionWithMessage(char const *message) {
    PyErr_SetString(PyExc_RuntimeError, message);
}


inline unsigned int widthFromFormat(RtAudioFormat fmt) {
    unsigned int w = 1;
    switch (fmt) {
        case RTAUDIO_SINT8:
            w = 1;
            break;
        case RTAUDIO_SINT16:
            w = 2;
            break;
        case RTAUDIO_SINT24:
        case RTAUDIO_SINT32:
        case RTAUDIO_FLOAT32:
            w = 4;
            break;
        case RTAUDIO_FLOAT64:
            w = 8;
            break;
        default:
            w = 1;
            break;
    }
    return w;
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

#endif
