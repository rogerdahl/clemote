#include <iostream>
#include <fmt/format.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "clementine_dbus.h"
#include "tag.h"

#include <sys/stat.h>

using namespace std;
using namespace boost::filesystem;
using namespace boost::algorithm;

int remoteControl(const string& device);

string eventCodeToString(int eventCode);
void touchParentDir(const path& filePath);
void setRatingOnCurrent(ClementineDbus& clem, int ratingInt, bool skipToNext);
void syncMyRatingToPopularityMeterRecursive(const path& rootDir);

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

int remoteControl(const string& device)
{
  int fevdev = open(device.c_str(), O_RDONLY);

  char name[256] = "Unknown";
  ioctl(fevdev, EVIOCGNAME(sizeof(name)), name);
  fmt::print("Reading From: {} ({})\n", name, device);

  int result = ioctl(fevdev, EVIOCGRAB, 1);
  if (result) {
    fmt::print(
      "Error: Unable to get exclusive access to device. error={}", result);
    return result;
  }

  ClementineDbus clem;

  while (true) {
    struct input_event ev;
    read(fevdev, &ev, sizeof(struct input_event));

    if (ev.type != 1 || ev.value != 1) {
      continue;
    }

    fmt::print("code={}({}) value={}\n",
               eventCodeToString(ev.code), ev.code, ev.value
    );

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
    case KEY_PAUSE:
      clem.playerPause();
      break;
    case KEY_STOP:
      clem.playerStop();
      break;
    case KEY_PREVIOUS:
//    case KEY_SUBTITLE:
    case KEY_EXIT:
      clem.playerPrev();
      break;
    case KEY_NEXT:
    case KEY_INFO:
      clem.playerNext();
      break;
    case KEY_REWIND: {
      // skip back 30 sec
      int pos = clem.getPlayerPosition();
      int newPos = pos - 30 * 1000;
      clem.setPlayerPosition(newPos);
      pos /= 1000;
      newPos /= 1000;
      fmt::print(
        "Skip back: {}:{:02} -> {}:{:02}\n", pos / 60, pos % 60, newPos / 60,
        newPos % 60);
      break;
    }
    case KEY_FASTFORWARD: {
      // skip forward 30 sec
      int pos = clem.getPlayerPosition();
      int newPos = pos + 30 * 1000;
      clem.setPlayerPosition(newPos);
      pos /= 1000;
      newPos /= 1000;
      fmt::print(
        "Skip forward: {}:{:02} -> {}:{:02}\n", pos / 60, pos % 60, newPos / 60,
        newPos % 60);
      break;
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
    case KEY_NUMERIC_3:
      setRatingOnCurrent(clem, 1, true);
      break;

    case KEY_NUMERIC_6:
      setRatingOnCurrent(clem, 2, true);
      break;

    case KEY_NUMERIC_9:
      setRatingOnCurrent(clem, 3, true);
      break;

    case KEY_NUMERIC_POUND:
      setRatingOnCurrent(clem, 4, true);
      break;

    case KEY_ENTER:
      setRatingOnCurrent(clem, 5, true);
      break;

    // System volume
    //    case KEY_CHANNELUP:
    //      clem.getPlayerMetadata();

    case KEY_CHANNELDOWN: {
      string p = clem.getPlayerCurrentPath();
      fmt::print("\n~~\nCurrently playing: {}\n", p);
      dump(p);
      break;
    }

    //    KEY_VOLUMEUP / KEY_CHANNELUP
    //    KEY_VOLUMEDOWN / KEY_CHANNELDOWN

    //
    // File operations
    //

    //    case KEY_RED:
    case KEY_RECORD: {
      // Delete immediately, with extreme prejudice
      string path = clem.getPlayerCurrentPath();
      fmt::print("Deleting file:\n{}\n", path);
      int trackIdx = clem.getTrackListCurrentTrack();
      clem.playerNext();
      remove(path);
      clem.removeTrackListTrack(trackIdx);
      break;
    }

    case KEY_BLUE:
    case KEY_SLEEP: {
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
      fmt::print(
        "Unhandled: code={}({}) value={}\n", eventCodeToString(ev.code),
        ev.code, ev.value);
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
    syncMyRatingToPopularityMeter(iter->path().string().c_str());
  }
}

void setRatingOnCurrent(ClementineDbus& clem, int ratingInt, bool skipToNext)
{
  auto filePath = clem.getPlayerCurrentPath();
  setMyRating(filePath, ratingInt);
  // Update rating display in Clementine by forcing a library refresh
  // For this to work, Preferences > Music Library > Monitor library for changes
  // must be enabled
  touchParentDir(filePath);
  if (skipToNext) {
    clem.playerNext();
  }
}

void touchParentDir(const path& filePath)
{
  int f = open(filePath.parent_path().c_str(), O_NOCTTY | O_NONBLOCK, 0666);
  if (f >= 0) {
    futimens(f, nullptr);
    close(f);
  }
}

string eventCodeToString(int eventCode)
{
  switch (eventCode) {
  case 28:
    return "KEY_ENTER";
  case 103:
    return "KEY_UP";
  case 105:
    return "KEY_LEFT";
  case 106:
    return "KEY_RIGHT";
  case 108:
    return "KEY_DOWN";
  case 111:
    return "KEY_DELETE";
  case 113:
    return "KEY_MUTE";
  case 114:
    return "KEY_VOLUMEDOWN";
  case 115:
    return "KEY_VOLUMEUP";
  case 119:
    return "KEY_PAUSE";
  case 128:
    return "KEY_STOP";
  case 142:
    return "KEY_SLEEP";
  case 161:
    return "KEY_EJECTCD";
  case 164:
    return "KEY_PLAYPAUSE";
  case 167:
    return "KEY_RECORD";
  case 168:
    return "KEY_REWIND";
  case 174:
    return "KEY_EXIT";
  case 207:
    return "KEY_PLAY";
  case 208:
    return "KEY_FASTFORWARD";
  case 210:
    return "KEY_PRINT";
  case 212:
    return "KEY_CAMERA";
  case 224:
    return "KEY_BRIGHTNESSDOWN";
  case 225:
    return "KEY_BRIGHTNESSUP";
  case 226:
    return "KEY_MEDIA";
  case 352:
    return "KEY_OK";
  case 356:
    return "KEY_POWER2";
  case 358:
    return "KEY_INFO";
  case 365:
    return "KEY_EPG";
  case 366:
    return "KEY_PVR";
  case 368:
    return "KEY_LANGUAGE";
  case 369:
    return "KEY_TITLE";
  case 370:
    return "KEY_SUBTITLE";
  case 372:
    return "KEY_ZOOM";
  case 373:
    return "KEY_MODE";
  case 377:
    return "KEY_TV";
  case 385:
    return "KEY_RADIO";
  case 386:
    return "KEY_TUNER";
  case 387:
    return "KEY_PLAYER";
  case 389:
    return "KEY_DVD";
  case 392:
    return "KEY_AUDIO";
  case 393:
    return "KEY_VIDEO";
  case 398:
    return "KEY_RED";
  case 399:
    return "KEY_GREEN";
  case 400:
    return "KEY_YELLOW";
  case 401:
    return "KEY_BLUE";
  case 402:
    return "KEY_CHANNELUP";
  case 403:
    return "KEY_CHANNELDOWN";
  case 407:
    return "KEY_NEXT";
  case 412:
    return "KEY_PREVIOUS";
  case 425:
    return "KEY_PRESENTATION";
  case 512:
    return "KEY_NUMERIC_0";
  case 513:
    return "KEY_NUMERIC_1";
  case 514:
    return "KEY_NUMERIC_2";
  case 515:
    return "KEY_NUMERIC_3";
  case 516:
    return "KEY_NUMERIC_4";
  case 517:
    return "KEY_NUMERIC_5";
  case 518:
    return "KEY_NUMERIC_6";
  case 519:
    return "KEY_NUMERIC_7";
  case 520:
    return "KEY_NUMERIC_8";
  case 521:
    return "KEY_NUMERIC_9";
  case 522:
    return "KEY_NUMERIC_STAR";
  case 523:
    return "KEY_NUMERIC_POUND";
  default:
    return "<UNKNOWN>";
  }
}
