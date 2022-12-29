# Tommy
Turning the **Korg Nu:Tekt NTS-1** into a sampler using a combination of a custom oscillator and modfx. 

This requires a combination of modfx and oscillator to retrieve both note information and access external audio. Incoming note pitch is used by the oscillator to generate a pulse corresponding to the (note) frequency, which is then available to the modfx slot (as part of the audio channel input). The modfx samples external audio (left channel) to the sd ram buffer, and triggers playback (using the pitch information recieved from the oscillator).

## Usage
Make sure an audio source is plugged into the _left input channel_ of the NTS-1 (**the right channel must be muted**)

1. Select [oscillator/tommy.ntkdigunit](oscillator/tommy.ntkdigunit) as oscillator (make sure *filter is off* and *the envelope generator is set to open*)
2. Select [modfx/tommy.ntkdigunit](modfx/tommy.ntkdigunit) as modfx (or [modfx/tommy_pingpong.ntkdigunit](modfx/tommy_pingpong.ntkdigunit) with stereo ping-pong effect enabled)
3. Select modfx and use the **B** encoder to arm / set sample mode

### Single trigger mode
1. Turn the **B encoder to the left** to enable **single trigger** sample mode - **the further away from 12 o'clock the higher sample frequency** (48k - 4k) _When armed in this mode, the incoming audio is passed through the left channel_
2. Press a note to sample the incoming audio (triggered when the signal is slightly above or below zero)
3. The sample can now be played using the keyboard (where the **A encoder** is used to set the release time)

### Re-trigger mode
1. Turn the **B encoder to the right** to enable **re-trigger** sample mode - **the further away from 12 o'clock the higher sample frequency** (48k - 4k)
2. Press a note to sample the incoming audio (triggered when the signal is slightly above or below zero)
3. The NTS-1 will now **re-sample everytime the initial note is triggered** (e.g. using the arpeggiator) so ideally have a constant external audio feed (_a change to the encoder will reset / re-apply the note value_)

The pre-build binaries can be uploaded using the NTS-1 digital Librarian application.

A short demonstration can be viewed here:

[![](http://img.youtube.com/vi/hxtuTzcXitw/0.jpg)](http://www.youtube.com/watch?v=hxtuTzcXitw)
