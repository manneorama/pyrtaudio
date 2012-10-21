from distutils.core import setup, Extension

module = Extension('pyrtaudio', sources=['pyrtaudio.cpp', 'RtAudio.cpp'],
        define_macros=[('__LINUX_ALSA__', '')],
        libraries=['asound','pthread'])

setup(name='pyrtaudio',
        version='0.1',
        description='RtAudio python bindings',
        ext_modules=[module],
        language='c++')

