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

#ifdef __cplusplus
extern "C" {
#endif

#define PYGILSTATE_ACQUIRE PyGILState_STATE gstate; \
    do { gstate = PyGILState_Ensure(); } while (0)

#define PYGILSTATE_RELEASE do { PyGILState_Release(gstate); } while (0)


typedef struct {
    PyObject_HEAD
    RtAudio *_rt;
    PyObject *_cb;
} PyRtAudioObject;

typedef struct {
    PyObject_HEAD
    RtAudio::DeviceInfo *_info;
} PyRtAudioDeviceInfoObject;

static int __pyrtaudio_renderCallback(void *outputBuffer, void *inputBuffer,
        unsigned int frames, double streamTime, RtAudioStreamStatus status,
        void *userData) {
    PYGILSTATE_ACQUIRE;
    //TODO
    PYGILSTATE_RELEASE;
    return 0;
}

static int __pyrtaudio_captureCallback(void *outputBuffer, void *inputBuffer,
        unsigned int frames, double streamTime, RtAudioStreamStatus status,
        void *userData) {
    PYGILSTATE_ACQUIRE;
    PYGILSTATE_RELEASE;
    return 0;
}

static int __pyrtaudio_duplexCallback(void *outputBuffer, void *inputBuffer,
        unsigned int frames, double streamTime, RtAudioStreamStatus status,
        void *userData) {
    PYGILSTATE_ACQUIRE;
    PYGILSTATE_RELEASE;
    return 0;
}

static void
PyRtAudioDeviceInfo_dealloc(PyRtAudioDeviceInfoObject *self) {
    if (self->_info) delete self->_info;
    self->ob_type->tp_free((PyObject *) self);
}

static PyObject *
PyRtAudioDeviceInfo_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyRtAudioDeviceInfoObject *self;

    self = (PyRtAudioDeviceInfoObject *) type->tp_alloc(type, 0);
    if (self != NULL) {
        ;
    }

    return (PyObject *) self;
}

static int
PyRtAudioDeviceInfo_init(PyRtAudioDeviceInfoObject *self) {
    return 0;
}

static PyTypeObject pyrtaudio_PyRtAudioDeviceInfoType = {
    PyObject_HEAD_INIT(NULL)
    0,                                  //ob_size
    "pyrtaudio.DeviceInfo",              //tp_name
    sizeof(PyRtAudioDeviceInfoObject),            //tp_basicsize
    0,                                  //tp_itemsize
    (destructor) PyRtAudioDeviceInfo_dealloc,     //tp_dealloc
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
    "PyRtAudioDeviceInfo",                        //tp_doc
    0,                                  //tp_traverse
    0,                                  //tp_clear
    0,                                  //tp_richcompare
    0,                                  //tp_weaklistoffset
    0,                                  //tp_iter
    0,                                  //tp_iternext
    0,//    PyRtAudioDeviceInfoObject_methods,            //tp_methods
    0, //PyRtAudioObject_members,            //tp_members
    0,                                  //tp_getset
    0,                                  //tp_base
    0,                                  //tp_dict
    0,                                  //tp_descr_get
    0,                                  //tp_descr_set
    0,                                  //tp_dictoffset
    (initproc) PyRtAudioDeviceInfo_init,          //tp_init
    0,                                  //tp_alloc
    PyRtAudioDeviceInfo_new,                      //tp_new
};

static PyObject *
PyRtAudioDeviceInfo_probed(PyRtAudioDeviceInfoObject *self) {
    if (self->_info->probed) return Py_BuildValue("O", Py_True);
    return Py_BuildValue("O", Py_False);
}

static PyObject *
PyRtAudioDeviceInfo_name(PyRtAudioDeviceInfoObject *self) {
    printf("%s\n", self->_info->name.c_str());
    return Py_BuildValue("s", self->_info->name.c_str());
}

static PyObject *
PyRtAudioDeviceInfo_outputChannels(PyRtAudioDeviceInfoObject *self) {
    return Py_BuildValue("I", self->_info->outputChannels);
}

static PyObject *
PyRtAudioDeviceInfo_inputChannels(PyRtAudioDeviceInfoObject *self) {
    return Py_BuildValue("I", self->_info->inputChannels);
}

static PyObject *
PyRtAudioDeviceInfo_duplexChannels(PyRtAudioDeviceInfoObject *self) {
    return Py_BuildValue("I", self->_info->duplexChannels);
}

static PyObject *
PyRtAudioDeviceInfo_isDefaultOutput(PyRtAudioDeviceInfoObject *self) {
    if (self->_info->isDefaultOutput) return Py_BuildValue("O", Py_True);
    return Py_BuildValue("O", Py_False);
}

static PyObject *
PyRtAudioDeviceInfo_isDefaultInput(PyRtAudioDeviceInfoObject *self) {
    if (self->_info->isDefaultInput) return Py_BuildValue("O", Py_True);
    return Py_BuildValue("O", Py_False);
}

//TODO sample rates and native formats!!
//

static void
PyRtAudio_dealloc(PyRtAudioObject *self) {
    if (self->_rt->isStreamRunning()) self->_rt->stopStream();
    if (self->_rt->isStreamOpen()) self->_rt->closeStream();
    delete self->_rt;
    self->ob_type->tp_free((PyObject *) self);
}

static PyObject *
PyRtAudio_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyRtAudioObject *self;

    self = (PyRtAudioObject *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->_rt = new RtAudio;
        self->_cb = NULL;
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
    
    printf("1\n");
    PyObject *obj = _PyObject_New(&pyrtaudio_PyRtAudioDeviceInfoType);
    printf("2\n");
    PyObject_Init(obj, &pyrtaudio_PyRtAudioDeviceInfoType);
    printf("3\n");
    PyRtAudioDeviceInfoObject *o = (PyRtAudioDeviceInfoObject *) obj;
    printf("4\n");

    RtAudio::DeviceInfo *info = new RtAudio::DeviceInfo;
    *info = temp;
    o->_info = info;
    printf("5\n");
    return obj;
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
    if (isIt) return Py_BuildValue("O", Py_False);
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


//instance variables
//static PyMemberDef PyRtAudioObject_members[] = {
//    {NULL}
//};


//instance methods
static PyMethodDef PyRtAudioDeviceInfoObject_methods[] = {
    {"probed", (PyCFunction) PyRtAudioDeviceInfo_probed,
        METH_NOARGS, "Return True if this device's capabilities were successfully probed"},
    {"name", (PyCFunction) PyRtAudioDeviceInfo_name,
        METH_NOARGS, "The device identifier"},
    {"outputChannels", (PyCFunction) PyRtAudioDeviceInfo_outputChannels,
        METH_NOARGS, "Maximum number of output channels supported"},
    {"inputChannels", (PyCFunction) PyRtAudioDeviceInfo_inputChannels,
        METH_NOARGS, "Maximum number of input channels supported"},
    {"duplexChannels", (PyCFunction) PyRtAudioDeviceInfo_duplexChannels,
        METH_NOARGS, "Maximum number of duplex channels supported"},
    {"isDefaultOutput", (PyCFunction) PyRtAudioDeviceInfo_isDefaultOutput,
        METH_NOARGS, "Return True if this device is the default output device"},
    {"isDefaultInput", (PyCFunction) PyRtAudioDeviceInfo_isDefaultInput,
        METH_NOARGS, "Return True if this device is the default input device"},
    {NULL}
};

static PyMethodDef PyRtAudioObject_methods[] = {
    {"getDeviceCount", (PyCFunction) PyRtAudio_getDeviceCount,
        METH_NOARGS, "Return the number of audio devices present"},
    {"getDeviceInfo", (PyCFunction) PyRtAudio_getDeviceInfo,
        METH_VARARGS, "Return the device info for this device index"},
    {"getDefaultOutputDevice", (PyCFunction) PyRtAudio_getDefaultOutputDevice,
        METH_NOARGS, "Return the default output device index"},
    {"getDefaultInputDevice", (PyCFunction) PyRtAudio_getDefaultInputDevice,
        METH_NOARGS, "Return the default input device index"},
    {"isStreamOpen", (PyCFunction) PyRtAudio_isStreamOpen,
        METH_NOARGS, "Return True if a stream is currently open"},
    {"isStreamRunning", (PyCFunction) PyRtAudio_isStreamRunning,
        METH_NOARGS, "Return True is a stream is currently running"},
    {"getStreamTime", (PyCFunction) PyRtAudio_getStreamTime,
        METH_NOARGS, "Return the current stream time"},
    {"getStreamLatency", (PyCFunction) PyRtAudio_getStreamLatency,
        METH_NOARGS, "Return the current stream latency"},
    {"getStreamSampleRate", (PyCFunction) PyRtAudio_getStreamSampleRate,
        METH_NOARGS, "Return the current stream sample rate"},
    {NULL}
};


//module functions
static PyMethodDef pyrtaudio_functions[] = {
    {NULL}
};

//type definition
static PyTypeObject pyrtaudio_PyRtAudioType = {
    PyObject_HEAD_INIT(NULL)
    0,                                  //ob_size
    "pyrtaudio.PyRtAudio",              //tp_name
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
    "PyRtAudio",                        //tp_doc
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


#ifndef PyMODINIT_FUNC
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC initpyrtaudio(void) {
    PyObject *m;

    pyrtaudio_PyRtAudioDeviceInfoType.tp_methods = PyRtAudioDeviceInfoObject_methods;
    if (PyType_Ready(&pyrtaudio_PyRtAudioType) < 0) return;
    if (PyType_Ready(&pyrtaudio_PyRtAudioDeviceInfoType) < 0) return;

    m = Py_InitModule3("pyrtaudio", pyrtaudio_functions, "RtAudio python bindings");
    if (m == NULL) return;

    Py_INCREF(&pyrtaudio_PyRtAudioType);
    Py_INCREF(&pyrtaudio_PyRtAudioDeviceInfoType);
    PyModule_AddObject(m, "PyRtAudio", (PyObject *) &pyrtaudio_PyRtAudioType);
    PyModule_AddObject(m, "DeviceInfo", (PyObject *) &pyrtaudio_PyRtAudioDeviceInfoType);
}

#ifdef __cplusplus
}
#endif
