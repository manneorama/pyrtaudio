import pyrtaudio as pyrt


p = pyrt.RtAudio()
print dir(p)

d=p.getDeviceInfo()
print d

#playback / recording not yet supported :)
