import pyrtaudio as p
import wave


f = wave.open('somefile.wav')

def formatfromwidth(sampwidth):
    if sampwidth == 1:
        return p.RTAUDIO_SINT8
    if sampwidth == 2:
        return p.RTAUDIO_SINT16
    if sampwidth == 4:
        return p.RTAUDIO_SINT32
    #etcetera

def callback():
    frames = f.readframes(512)
    if frames == '':
        return None
    return frames

r=p.RtAudio()
r.openStream( {'device_id':0, 'channels':f.getnchannels(), 'first_channel':0},
        None, #only playback support yet
        formatfromwidth(f.getsampwidth()),
        f.getframerate(),
        512, #buffer size in frames
        callback)
r.startStream() #wooh!


