#include "pulse_audio.h"
#include "pulse/volume.h"

// pa_simple *s;
// pa_sample_spec ss;
// ss.format = PA_SAMPLE_S16NE;
// ss.channels = 2;
// ss.rate = 44100;
// s = pa_simple_new(NULL,               // Use the default server.
//                  "Fooapp",           // Our application's name.
//                  PA_STREAM_PLAYBACK,
//                  NULL,               // Use the default device.
//                  "Music",            // Description of our stream.
//                  &ss,                // Our sample format.
//                  NULL,               // Use default channel map
//                  NULL,               // Use default buffering attributes.
//                  NULL,               // Ignore error code.
//);

PulseAudio::PulseAudio() = default;
PulseAudio::~PulseAudio() = default;

void PulseAudio::volumeUp()
{
  pa_cvolume a;
  pa_cvolume_init(&a);
  //  pa_cvolume_mute(&a, 2);
  pa_cvolume_set(&a, 2, 0);
  //  pa_context_set_sink_input_volume();
  //  v.channels = 2;
  //  v.values[0] = 50;
  //  v.values[1] = 50;
  //  pa_cvolume_set(v, 2, );
}

void PulseAudio::volumeDown()
{
  //  pa_cvolume v;
  //  v.channels = 2;
  //  v.values[2] = {0, 0};
  //  pa_cvolume_set(v);
}
