#include <fmt/format.h>
#include "alsa_volume.h"
#include <alsa/asoundlib.h>

void adjust_volume_channel(int adj_vol, snd_mixer_selem_channel_id_t channel_id, const char* channel_name);

void adjust_volume(int adj_vol)
{
  adjust_volume_channel(adj_vol, snd_mixer_selem_channel_id_t::SND_MIXER_SCHN_FRONT_RIGHT, "Front Right");
  adjust_volume_channel(adj_vol, snd_mixer_selem_channel_id_t::SND_MIXER_SCHN_FRONT_LEFT, "Front Left");
}

void adjust_volume_channel(int adj_vol, snd_mixer_selem_channel_id_t channel_id, const char* channel_name)
{
  snd_mixer_t* handlev;
  snd_mixer_selem_id_t* sid = {};

  const char* card = "default";
  const char* selem_name = "Master";

  snd_mixer_open(&handlev, 0);
  snd_mixer_attach(handlev, card);
  snd_mixer_selem_register(handlev, nullptr, nullptr);
  snd_mixer_load(handlev);

  snd_mixer_selem_id_set_index(sid, 0);
  snd_mixer_selem_id_set_name(sid, selem_name);
  snd_mixer_elem_t* elem = snd_mixer_find_selem(handlev, sid);

  long min_vol, max_vol;
  snd_mixer_selem_get_playback_volume_range(elem, &min_vol, &max_vol);

  long old_vol;
  snd_mixer_selem_get_playback_volume(elem, channel_id, &old_vol);
  old_vol = old_vol * 100 / max_vol + min_vol;
  auto new_vol = old_vol += adj_vol;

  if (new_vol < min_vol) {
    new_vol = min_vol;
  }
  if (new_vol > max_vol) {
    new_vol = max_vol;
  }

  fmt::print("Volume @ {}: {} -> {}\n", channel_name, old_vol, new_vol);

  snd_mixer_selem_set_playback_volume(elem, channel_id, old_vol * max_vol / 100 + min_vol);
  snd_mixer_close(handlev);
}
