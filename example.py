import pyrtaudio as p
import wave, time

## simple playback (rendering) example
fr = wave.open('somefile.wav', 'r')
bframes = 512

def render_callback():
    frames = fr.readframes(bframes)
    if frames == '':
        return None #stops the stream
    return frames

def format_from_width(w):
    if w == 1:
        return p.RTAUDIO_SINT8
    if w == 2:
        return p.RTAUDIO_SINT16
    if w == 4:
        return p.RTAUDIO_SINT32

r=p.RtAudio()
r.open_stream( {'device_id':0, 'channels':fr.getnchannels(), 'first_channel':0},
        None, #no input parameters -> only playback
        format_from_width(fr.getsampwidth()),
        fr.getframerate(),
        bframes, #buffer size in frames
        render_callback)

r.start_stream() #wooh!
time.sleep(2)
r.close_stream()

fr.close()

## simple recording (capture) example
channels = 1
rate = 44100
width = 2
fw = wave.open('someotherfile.wav', 'w')
fw.setframerate(rate)
fw.setsampwidth(width)
fw.setnchannels(channels)

def capture_callback(frames):
    fw.writeframes(frames)
    return 0 #returning None will stop the stream

r.open_stream(None,
    {'device_id':0, 'channels':channels, 'first_channel':0},
    format_from_width(width),
    rate,
    bframes,
    capture_callback)

r.start_stream()
time.sleep(4)
r.close_stream()

fw.close()
