#include <fmt/format.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "clementine_dbus.h"
#include "tag.h"
#include "alsa_volume.h"
#include "event_code_to_str.h"

// Couldn't find example for changing volume.
//#include <soundio/soundio.h>

using namespace std;
using namespace boost::filesystem;
using namespace boost::algorithm;

int remoteControl(const string& device);

void touchParentDir(const path& filePath);
void touchCurrentParentDir(ClementineDbus& clem);
void setRatingOnCurrent(ClementineDbus& clem, int ratingInt, bool skipToNext);
void syncMyRatingToPopularityMeterRecursive(const path& rootDir);

const int VOLUME_ADJ_STEP = 5;

int main(int argc, char** argv)
{
  auto progName = path(argv[0]).filename().string();

  if (argc != 3) {
    fmt::print("Usage: {} remote </dev/input/eventX>\n", progName);
    fmt::print("Or:    {} refresh <root-music-dir>\n", progName);
    return 1;
  }

  auto mode = string(argv[1]);

  if (mode == "remote") {
    return remoteControl(argv[2]);
  }
  else if (mode == "refresh") {
    syncMyRatingToPopularityMeterRecursive(argv[2]);
  }

  return 0;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

#pragma ide diagnostic ignored "EndlessLoop"
int remoteControl(const string& device)
{
  int fevdev = open(device.c_str(), O_RDONLY);

  char name[256] = "Unknown";
  ioctl(fevdev, EVIOCGNAME(sizeof(name)), name);
  fmt::print("Reading From: {} ({})\n", name, device);

  int result = ioctl(fevdev, EVIOCGRAB, 1);
  if (result) {
    fmt::print("Error: Unable to get exclusive access to device. error={}\n", result);
    return result;
  }

  ClementineDbus clem;

  launchThread();

  while (true) {
    struct input_event ev = {};
    auto size = read(fevdev, &ev, sizeof(struct input_event));

    if (size == -1) {
      fmt::print("event read() failed\n");
      continue;
    }
    if (size == 0) {
      fmt::print("event read() returned EOF\n");
      continue;
    }
    if (ev.type != 1 || ev.value != 1) {
      continue;
    }

    //    fmt::print("code={}({}) value={}\n",
    //               eventCodeToString(ev.code), ev.code, ev.value
    //    );

    switch (ev.code) {

      //
      // Direct to player mappings
      //

    case KEY_MUTE:
      clem.playerMute();
      break;
    case KEY_PLAY:
      clem.playerPlay();
      break;

    case KEY_COMPOSE:
    case KEY_PAUSE:
    case KEY_SUBTITLE:
      clem.playerPause();
      break;
    case KEY_PLAYPAUSE:
      clem.playerPlayPause();
      break;
    case KEY_STOP:
      clem.playerStop();
      break;
    case KEY_UP:
    case KEY_PREVIOUS:
    case KEY_PREVIOUSSONG:
      //    case KEY_SUBTITLE:
      // case KEY_EXIT:
      clem.playerPrev();
      break;
    case KEY_DOWN:
    case KEY_NEXT:
    case KEY_INFO:
    case KEY_NEXTSONG:
      clem.playerNext();
      break;
    case KEY_LEFT:
    case KEY_REWIND: {
      // skip back 30 sec
      auto pos = clem.getPlayerPosition();
      string trackId = clem.getCurrentTrackId();
      //      fmt::print("trackId: {}\n", trackId);
      auto newPos = pos - 30 * 1000;
      clem.setPlayerPosition(trackId, newPos);
      pos /= 1000;
      newPos /= 1000;
      fmt::print("Skip back: {}:{:02} -> {}:{:02}\n", pos / 60, pos % 60, newPos / 60, newPos % 60);
      break;
    }
    case KEY_RIGHT:
    case KEY_FASTFORWARD: {
      // skip forward 30 sec
      auto pos = clem.getPlayerPosition();
      string trackId = clem.getCurrentTrackId();
      fmt::print("trackId: {}\n", trackId);
      auto newPos = pos + 30 * 1000;
      clem.setPlayerPosition(trackId, newPos);
      pos /= 1000;
      newPos /= 1000;
      fmt::print("Skip forward: {}:{:02} -> {}:{:02}\n", pos / 60, pos % 60, newPos / 60, newPos % 60);
      break;
    }
    case KEY_YELLOW:
    case KEY_DVD:
    case KEY_0: {
      string path = clem.getPlayerCurrentPath();
      fmt::print("~~~~\n\nCurrently playing:\n{}\n\n", path);
      dump(path);
      break;
    }

    case KEY_PVR: {
      clem.removeCurrentTrackFromPlaylist();
      touchCurrentParentDir(clem);
    }
      //
      // Tag and keep playing, score 1 - 5
      //

    case KEY_NUMERIC_1:
      setRatingOnCurrent(clem, 1, false);
      break;

    case KEY_NUMERIC_4:
      setRatingOnCurrent(clem, 2, false);
      break;

    case KEY_NUMERIC_7:
      setRatingOnCurrent(clem, 3, false);
      break;

    case KEY_NUMERIC_STAR:
      setRatingOnCurrent(clem, 4, false);
      break;

    case KEY_DELETE:
      setRatingOnCurrent(clem, 5, false);
      break;

    //
    // Tag and skip to next, score 1 - 5
    //
    case KEY_1:
    case KEY_NUMERIC_3:
      setRatingOnCurrent(clem, 1, true);
      break;

    case KEY_2:
    case KEY_NUMERIC_6:
      setRatingOnCurrent(clem, 2, true);
      break;

    case KEY_3:
    case KEY_NUMERIC_9:
      setRatingOnCurrent(clem, 3, true);
      break;

    case KEY_4:
    case KEY_NUMERIC_POUND:
      setRatingOnCurrent(clem, 4, true);
      break;

    case KEY_5:
    case KEY_ENTER:
      setRatingOnCurrent(clem, 5, true);
      break;

      // System volume

    case KEY_VOLUMEUP: {
      adjust_volume(VOLUME_ADJ_STEP);
      break;
    }
    case KEY_VOLUMEDOWN: {
      adjust_volume(-VOLUME_ADJ_STEP);
      break;
    }

    // Internal Clementine volume
    case KEY_PAGEUP:
    case KEY_CHANNELUP: {
      clem.volumeUp();
      break;
    }
    case KEY_PAGEDOWN:
    case KEY_CHANNELDOWN: {
      clem.volumeDown();
      break;
    }

      //
      // File operations
      //

      // case KEY_RECORD: {
      //   // Delete immediately, with extreme prejudice
      //   string path = clem.getPlayerCurrentPath();
      //   fmt::print("Deleting file:\n{}\n", path);
      //   string trackId = clem.getCurrentTrackId();
      //   clem.playerNext();
      //   remove(path);
      //   clem.removeTrackFromPlaylist(trackId);
      //   break;
      // }

    case KEY_BLUE: {
      // Tag for delete by renaming file
      string path = clem.getPlayerCurrentPath();
      auto newPath = path;
      replace_all(newPath, ".mp3", ".delete.mp3");
      // TODO: Replace with regex
      fmt::print("Renaming file:\n{} ->\n{}\n", path, newPath);
      clem.playerNext();
      rename(path, newPath);
      break;
    }

    default:
      fmt::print("Unhandled: code={}(0x{:x}) symbol={}\n", ev.code, ev.code, eventCodeToString(ev.code));
      break;
    }
  }

  return 0;
}

#pragma clang diagnostic pop

// Initial scan to make ratings from old library visible in Clementine
void syncMyRatingToPopularityMeterRecursive(const path& rootDir)
{
  for (recursive_directory_iterator iter(rootDir), end; iter != end; ++iter) {
    syncMyRatingToPopularityMeter(iter->path().string());
  }
}

void setRatingOnCurrent(ClementineDbus& clem, int ratingInt, bool skipToNext)
{
  auto filePath = clem.getPlayerCurrentPath();
  setMyRating(filePath, ratingInt);
  // Update rating display in Clementine by forcing a library refresh
  // For this to work,
  // Clementine > Tools > Preferences > Music Library > Monitor library for
  // changes must be enabled
  touchParentDir(filePath);
  if (skipToNext) {
    clem.playerNext();
  }
}

void touchCurrentParentDir(ClementineDbus& clem)
{
  auto filePath = clem.getPlayerCurrentPath();
  touchParentDir(filePath);
}

void touchParentDir(const path& filePath)
{
  int f = open(filePath.parent_path().c_str(), O_NOCTTY | O_NONBLOCK, 0666);
  if (f >= 0) {
    futimens(f, nullptr);
    close(f);
  }
}
