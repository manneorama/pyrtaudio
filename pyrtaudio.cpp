/* PyRtAudio - Python bindings for the RtAudio realtime audio i/o c++ classes
    Copyright (C) 2012  Emanuel Birge, emanuel.birge@gmail.com

    RtAudio is Copyright (C) 2001-2012 Gary P. Scavone
        http://www.music.mcgill.ca/~gary/rtaudio/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Python.h>
#include <string.h>
#include <stdio.h>

#include "RtAudio.h"
#include "pyrtutils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PYGILSTATE_ACQUIRE PyGILState_STATE gstate; \
    do { gstate = PyGILState_Ensure(); } while (0)

#define PYGILSTATE_RELEASE do { PyGILState_Release(gstate); } while (0)

// pyrtaudio.RtAudio()
typedef struct {
    PyObject_HEAD
    RtAudio *_rt;               // the RtAudio instance
    PyObject *_cb;              // the user defined callback
    // storing buffer pointers here avoids memory allocation inside the callbacks
    Py_buffer *_outputView;     // pre-allocated space for an output buffer
    unsigned long _expectedOutputBufferLength;
    unsigned long _expectedInputBufferLength;
} PyRtAudioObject;

// format flags
static PyObject *PyRtAudio_SINT8;
static PyObject *PyRtAudio_SINT16;
static PyObject *PyRtAudio_SINT24;
static PyObject *PyRtAudio_SINT32;
//static PyObject *PyRtAudio_FLOAT32;
//static PyObject *PyRtAudio_FLOAT64;

// this function is called by RtAudio when operating in render-only mode
static int __pyrtaudio_renderCallback(void *outputBuffer, void *inputBuffer,
        unsigned int frames, double streamTime, RtAudioStreamStatus status,
        void *userData) {
    int retcode = 0;
    PyObject *result;
    Py_buffer *view;

    PYGILSTATE_ACQUIRE;
    PyRtAudioObject *self = (PyRtAudioObject *) userData;
    view = self->_outputView;

    // call the user specified callback
    result = PyEval_CallObject(self->_cb, NULL);
    retcode = isNone(result);
    if (retcode) goto cleanup_none_returned;

    // make sure that the returned object supports the buffer interface
    retcode = isBuffer(result);
    if (retcode) goto cleanup_not_a_buffer;

    // extract the buffer from the object
    retcode = getBuffer(result, &view);
    if (retcode) goto cleanup_could_not_extract;

    // check length sanity
    retcode = checkLength(self->_expectedOutputBufferLength, view->len);
    if (retcode) goto cleanup_unexpected_length;

    // fill output buffer
    memcpy(outputBuffer, view->buf, view->len);

    // cleanup
    cleanup_unexpected_length:
    PyBuffer_Release(view);
    cleanup_could_not_extract:
    cleanup_not_a_buffer:
    cleanup_none_returned:
    Py_DECREF(result);
    PYGILSTATE_RELEASE;
    // no need to free the memory used by view since it's allocated on
    // instantiation of the RtAudio class in Python and freed on deallocation
    return retcode;
}

// this function is called by RtAudio when operating in capture-only mode
static int __pyrtaudio_captureCallback(void *outputBuffer, void *inputBuffer,
        unsigned int frames, double streamTime, RtAudioStreamStatus status,
        void *userData) {
    int retcode = 0;
    PyObject *result = NULL;
    PyObject *bytearray = NULL;
    PyObject *arglist = NULL;

    PYGILSTATE_ACQUIRE;
    PyRtAudioObject *self = (PyRtAudioObject *) userData;
    // creating the byte array with the actual input buffer is
    // safe since PyByteArray_FromStringAndSize performs a memcpy
    // (bytearrayobject.c:147)
    // which also means that DECREF:ing the bytearray won't touch
    // our output buffer
    retcode = getByteArray(&bytearray, inputBuffer, 
            self->_expectedInputBufferLength);
    if (retcode) goto cleanup_no_bytearray;

    // pack the bytearray into an argument list
    retcode = getArgList(&arglist, &bytearray);
    if (retcode) goto cleanup_no_arglist;

    // call the python function
    result = PyEval_CallObject(self->_cb, arglist);
    retcode = isNone(result);

    Py_DECREF(result);
    Py_DECREF(arglist);
    cleanup_no_arglist:
    Py_DECREF(bytearray);
    cleanup_no_bytearray:
    PYGILSTATE_RELEASE;
    return retcode;
}

// this function is called by RtAudio when operating in duplex mode
static int __pyrtaudio_duplexCallback(void *outputBuffer, void *inputBuffer,
        unsigned int frames, double streamTime, RtAudioStreamStatus status,
        void *userData) {
    int retcode = 0;
    PyObject *result = NULL;
    PyObject *bytearray = NULL;
    PyObject *arglist = NULL;
    Py_buffer *view = NULL;

    PYGILSTATE_ACQUIRE;

    PyRtAudioObject *self = (PyRtAudioObject *) userData;
    view = self->_outputView;

    retcode = getByteArray(&bytearray, inputBuffer, 
            self->_expectedInputBufferLength);
    if (retcode) goto cleanup_no_bytearray;

    retcode = getArgList(&arglist, &bytearray);
    if (retcode) goto cleanup_no_arglist;

    result = PyEval_CallObject(self->_cb, arglist);

    retcode = isNone(result);
    if (retcode) goto cleanup_none_returned;

    retcode = isBuffer(result);
    if (retcode) goto cleanup_not_a_buffer;

    retcode = getBuffer(result, &view);
    if (retcode) goto cleanup_could_not_extract;

    retcode = checkLength(self->_expectedOutputBufferLength, view->len);
    if (retcode) goto cleanup_unexpected_length;

    memcpy(outputBuffer, view->buf, view->len);

    cleanup_unexpected_length:
    PyBuffer_Release(view);
    cleanup_could_not_extract:
    cleanup_not_a_buffer:
    cleanup_none_returned:
    Py_DECREF(result);
    Py_DECREF(arglist);
    cleanup_no_arglist:
    Py_DECREF(bytearray);
    cleanup_no_bytearray:
    PYGILSTATE_RELEASE;
    return retcode;
}

// start RtAudio wrap implementation
static void
PyRtAudio_dealloc(PyRtAudioObject *self) {
    if (self->_rt->isStreamRunning()) self->_rt->stopStream();
    if (self->_rt->isStreamOpen()) self->_rt->closeStream();
        delete self->_rt;

    if (self->_outputView) free(self->_outputView);

    Py_XDECREF(self->_cb);
    self->_cb = NULL;

    self->_outputView = NULL;

    self->ob_type->tp_free((PyObject *) self);
}

static PyObject *
PyRtAudio_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyRtAudioObject *self;

    self = (PyRtAudioObject *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->_rt = new RtAudio;
        self->_cb = NULL;
        self->_outputView = NULL;
    }
    
    return (PyObject *) self;
}

static int
PyRtAudio_init(PyRtAudioObject *self, PyObject *args, PyObject *kwds) {
    //nothing here right now
    return 0;
}

static PyObject *
PyRtAudio_getDeviceCount(PyRtAudioObject *self) {
    unsigned int cnt = self->_rt->getDeviceCount();
    return Py_BuildValue("I", cnt);
}

static PyObject *
PyRtAudio_getDeviceInfo(PyRtAudioObject *self, PyObject *args) {
    unsigned int device;
    if (PyArg_ParseTuple(args, "I", &device) == 0)
        return NULL;

    RtAudio::DeviceInfo temp = self->_rt->getDeviceInfo(device);
    
    printf("%d, %s\n", temp.outputChannels, temp.name.c_str());
    return Py_BuildValue("{s:O,s:s,s:I,s:I,s:I,s:O,s:O}", 
            "probed", ((temp.probed) ? (Py_True) : (Py_False)),
            "name", temp.name.c_str(),
            "output_channels", temp.outputChannels,
            "input_channels", temp.inputChannels,
            "duplex_channels", temp.duplexChannels,
            "default_output", ((temp.isDefaultOutput) ? (Py_True) : (Py_False)),
            "default_input", ((temp.isDefaultInput) ? (Py_True) : (Py_False)));
}

static PyObject *
PyRtAudio_getDefaultOutputDevice(PyRtAudioObject *self) {
    unsigned int dev = self->_rt->getDefaultOutputDevice();
    return Py_BuildValue("I", dev);
}

static PyObject *
PyRtAudio_getDefaultInputDevice(PyRtAudioObject *self) {
    unsigned int dev = self->_rt->getDefaultInputDevice();
    return Py_BuildValue("I", dev);
}

static PyObject *
PyRtAudio_isStreamOpen(PyRtAudioObject *self) {
    bool isIt = self->_rt->isStreamOpen();
    if (isIt) return Py_BuildValue("O", Py_True);
    return Py_BuildValue("O", Py_False);
}

static PyObject *
PyRtAudio_isStreamRunning(PyRtAudioObject *self) {
    bool isIt = self->_rt->isStreamRunning();
    if (isIt) return Py_BuildValue("O", Py_True);
    return Py_BuildValue("O", Py_False);
}

static PyObject *
PyRtAudio_getStreamTime(PyRtAudioObject *self) {
    double st = self->_rt->getStreamTime();
    return Py_BuildValue("d", st);
}

static PyObject *
PyRtAudio_getStreamLatency(PyRtAudioObject *self) {
    long l = self->_rt->getStreamLatency();
    return Py_BuildValue("l", l);
}

static PyObject *
PyRtAudio_getStreamSampleRate(PyRtAudioObject *self) {
    unsigned int sr = self->_rt->getStreamSampleRate();
    return Py_BuildValue("I", sr);
}

static PyObject *
PyRtAudio_openStream(PyRtAudioObject *self, PyObject *args) {
    char const *fmt = "OOkIIO";
    PyObject *oparms, *iparms, *callback;
    unsigned int srate, bframes;
    unsigned long format;

    if (!PyArg_ParseTuple(args, fmt, &oparms, &iparms, &format, &srate, &bframes, &callback))
        return NULL;

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "Callback parameter must be callable");
        return NULL;
    }

    Py_XINCREF(callback);
    Py_XDECREF(self->_cb);
    self->_cb = callback;

    int hasOutputParams = PyDict_CheckExact(oparms);
    int hasInputParams = PyDict_CheckExact(iparms);
    if (!hasOutputParams && !hasInputParams) {
        PyErr_SetString(PyExc_TypeError, "No input or output parameters given");
        return NULL;
    }

    RtAudio::StreamParameters *inputParams = NULL;
    RtAudio::StreamParameters *outputParams = NULL;

    self->_outputView = NULL;
    if (hasOutputParams) { 
        outputParams = populateStreamParameters(oparms);
        if (!outputParams) {
            PyErr_SetString(PyExc_AttributeError, "Error in output parameters");
            return NULL;
        }
        self->_expectedOutputBufferLength = widthFromFormat(format);
        self->_expectedOutputBufferLength *= outputParams->nChannels;
        self->_expectedOutputBufferLength *= bframes;
        self->_outputView = (Py_buffer *) malloc(sizeof(*(self->_outputView)));
    }
    if (hasInputParams) {
        inputParams = populateStreamParameters(iparms);
        if (!inputParams) {
            PyErr_SetString(PyExc_AttributeError, "Error in input parameters");
            return NULL;
        }
        self->_expectedInputBufferLength = widthFromFormat(format);
        self->_expectedInputBufferLength *= inputParams->nChannels;
        self->_expectedInputBufferLength *= bframes;
    }

    // decide which callback to use
    RtAudioCallback cb;
    if (outputParams && !inputParams) 
        cb = __pyrtaudio_renderCallback;
    else if (!outputParams && inputParams) 
        cb = __pyrtaudio_captureCallback;
    else if (outputParams && inputParams)
        cb = __pyrtaudio_duplexCallback;

    try {
        self->_rt->openStream(outputParams, inputParams, format, srate, &bframes, cb, (void *) self);
    } catch (RtError &e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
    }

    if (outputParams) delete outputParams;
    if (inputParams)  delete inputParams;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyRtAudio_startStream(PyRtAudioObject *self) {
    if (!self->_rt->isStreamOpen()) {
        PyErr_SetString(PyExc_RuntimeError, "No open streams");
        return NULL; //openStream was not called
    }
    if (self->_rt->isStreamRunning()) {
        PyErr_SetString(PyExc_RuntimeError, "Stream already running");
        return NULL; //stream is already running
    }

    self->_rt->startStream();

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyRtAudio_stopStream(PyRtAudioObject *self) {
    if (!self->_rt->isStreamOpen()) {
        PyErr_SetString(PyExc_RuntimeError, "No open streams");
        return NULL;
    }
    if (!self->_rt->isStreamRunning()) {
        PyErr_SetString(PyExc_RuntimeError, "Stream is not running");
        return NULL;
    }

    self->_rt->stopStream();

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyRtAudio_abortStream(PyRtAudioObject *self) {
    if (!self->_rt->isStreamOpen()) {
        PyErr_SetString(PyExc_RuntimeError, "No open streams");
        return NULL;
    }
    if (!self->_rt->isStreamRunning()) {
        PyErr_SetString(PyExc_RuntimeError, "Stream is not running");
        return NULL;
    }

    self->_rt->abortStream();

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyRtAudio_closeStream(PyRtAudioObject *self) {
    if (!self->_rt->isStreamOpen()) {
        PyErr_SetString(PyExc_RuntimeError, "No open streams");
        return NULL;
    }

    self->_rt->closeStream();

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef PyRtAudioObject_methods[] = {
    {"get_device_count", (PyCFunction) PyRtAudio_getDeviceCount,
        METH_NOARGS, "Return the number of audio devices present"},
    {"get_device_info", (PyCFunction) PyRtAudio_getDeviceInfo,
        METH_VARARGS, "Return the device info for this device index"},
    {"get_default_output_device", (PyCFunction) PyRtAudio_getDefaultOutputDevice,
        METH_NOARGS, "Return the default output device index"},
    {"get_default_input_device", (PyCFunction) PyRtAudio_getDefaultInputDevice,
        METH_NOARGS, "Return the default input device index"},
    {"is_stream_open", (PyCFunction) PyRtAudio_isStreamOpen,
        METH_NOARGS, "Return True if a stream is currently open"},
    {"is_stream_running", (PyCFunction) PyRtAudio_isStreamRunning,
        METH_NOARGS, "Return True is a stream is currently running"},
    {"get_stream_time", (PyCFunction) PyRtAudio_getStreamTime,
        METH_NOARGS, "Return the current stream time"},
    {"get_stream_latency", (PyCFunction) PyRtAudio_getStreamLatency,
        METH_NOARGS, "Return the current stream latency"},
    {"get_stream_sample_rate", (PyCFunction) PyRtAudio_getStreamSampleRate,
        METH_NOARGS, "Return the current stream sample rate"},
    {"open_stream", (PyCFunction) PyRtAudio_openStream,
        METH_VARARGS, "Open an audio stream"},
    {"start_stream", (PyCFunction) PyRtAudio_startStream,
        METH_NOARGS, "Start an open audio stream"},
    {"stop_stream", (PyCFunction) PyRtAudio_stopStream,
        METH_NOARGS, "Stop a running audio stream"},
    {"abort_stream", (PyCFunction) PyRtAudio_abortStream,
        METH_NOARGS, "Abort a running audio stream (do not flush buffers)"},
    {"close_stream", (PyCFunction) PyRtAudio_closeStream,
        METH_NOARGS, "Close a stream. If the stream is running it will be stopped"},
    {NULL}
};

// PyRtAudio type definition
static PyTypeObject pyrtaudio_PyRtAudioType = {
    PyObject_HEAD_INIT(NULL)
    0,                                  //ob_size
    "pyrtaudio.RtAudio",              //tp_name
    sizeof(PyRtAudioObject),            //tp_basicsize
    0,                                  //tp_itemsize
    (destructor) PyRtAudio_dealloc,     //tp_dealloc
    0,                                  //tp_print
    0,                                  //tp_getattr
    0,                                  //tp_setattr
    0,                                  //tp_compare
    0,                                  //tp_repr
    0,                                  //tp_as_number
    0,                                  //tp_as_sequence
    0,                                  //tp_as_mapping
    0,                                  //tp_hash
    0,                                  //tp_call
    0,                                  //tp_str
    0,                                  //tp_getattro
    0,                                  //tp_setattro
    0,                                  //tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, //tp_flags
    "RtAudio",                        //tp_doc
    0,                                  //tp_traverse
    0,                                  //tp_clear
    0,                                  //tp_richcompare
    0,                                  //tp_weaklistoffset
    0,                                  //tp_iter
    0,                                  //tp_iternext
    PyRtAudioObject_methods,            //tp_methods
    0, //PyRtAudioObject_members,            //tp_members
    0,                                  //tp_getset
    0,                                  //tp_base
    0,                                  //tp_dict
    0,                                  //tp_descr_get
    0,                                  //tp_descr_set
    0,                                  //tp_dictoffset
    (initproc) PyRtAudio_init,          //tp_init
    0,                                  //tp_alloc
    PyRtAudio_new,                      //tp_new
};

// end RtAudio implementation

//module functions
static PyMethodDef pyrtaudio_functions[] = {
    {NULL}
};



#ifndef PyMODINIT_FUNC
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC initpyrtaudio(void) {
    PyObject *m;

    // this is necessary for the RtAudio class to be able to
    // create DeviceInfo objects inside getDeviceInfo
    //pyrtaudio_PyRtAudioDeviceInfoType.tp_methods = PyRtAudioDeviceInfoObject_methods;
    
    if (PyType_Ready(&pyrtaudio_PyRtAudioType) < 0) return;

    m = Py_InitModule3("pyrtaudio", pyrtaudio_functions, "RtAudio python bindings");
    if (m == NULL) return;

    PyRtAudio_SINT8 = PyLong_FromUnsignedLong(RTAUDIO_SINT8);
    PyModule_AddObject(m, "RTAUDIO_SINT8", PyRtAudio_SINT8);
    Py_INCREF(PyRtAudio_SINT8);

    PyRtAudio_SINT16 = PyLong_FromUnsignedLong(RTAUDIO_SINT16);
    PyModule_AddObject(m, "RTAUDIO_SINT16", PyRtAudio_SINT16);
    Py_INCREF(PyRtAudio_SINT16);

    PyRtAudio_SINT24 = PyLong_FromUnsignedLong(RTAUDIO_SINT24);
    PyModule_AddObject(m, "RTAUDIO_SINT24", PyRtAudio_SINT24);
    Py_INCREF(PyRtAudio_SINT24);

    PyRtAudio_SINT32 = PyLong_FromUnsignedLong(RTAUDIO_SINT32);
    PyModule_AddObject(m, "RTAUDIO_SINT32", PyRtAudio_SINT32);
    Py_INCREF(PyRtAudio_SINT32);

    //PyRtAudio_FLOAT32 = PyLong_FromUnsignedLong(RTAUDIO_FLOAT32);
    //PyModule_AddObject(m, "RTAUDIO_FLOAT32", PyRtAudio_FLOAT32);
    //Py_INCREF(PyRtAudio_FLOAT32);

    //PyRtAudio_FLOAT64 = PyLong_FromUnsignedLong(RTAUDIO_FLOAT64);
    //PyModule_AddObject(m, "RTAUDIO_FLOAT64", PyRtAudio_FLOAT64);
    //Py_INCREF(PyRtAudio_FLOAT64);

    Py_INCREF(&pyrtaudio_PyRtAudioType);
    PyModule_AddObject(m, "RtAudio", 
            (PyObject *) &pyrtaudio_PyRtAudioType);
}

#ifdef __cplusplus
}
#endif
