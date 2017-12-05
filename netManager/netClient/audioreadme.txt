Project: Beaming Audio-recording Client Application

Audio recording and replay are handled by Portaudio and libsndfile. 

The idea is that the audio client records the loud-speaker output from all clients connected in a skype/googletalk conference call.
1. Connect all participants in a conference call session (skype/googletalk)
2. Connect your microphone into the default input device of the PC recording the audio.

In the cave, the loud speaker is recorded on the microphone input being used by the local cave client. This way all clients are recorded in the same audio file i.e. one audio file is synced with the beaming replayer.

Alternatively each client can connect with seperate audio clients resulting in different audio files that can be synced in the beaming replayer.