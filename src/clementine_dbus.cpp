#include "clementine_dbus.h"

#include <algorithm>
#include <fmt/format.h>

#include <boost/filesystem/convenience.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <systemd/sd-bus.h>

using namespace std;
using namespace boost::filesystem;
using namespace boost::algorithm;

const char* CLEMENTINE_SERVICE_NAME = "org.mpris.clementine";
const char* PLAYER_OBJECT_PATH = "/Player";
const char* TRACK_LIST_OBJECT_PATH = "/TrackList";
const char* MEDIA_PLAYER_INTERFACE_NAME = "org.freedesktop.MediaPlayer";

// $ qdbus org.mpris.clementine /Player
// org.freedesktop.MediaPlayer.GetMetadata
// $ qdbus org.mpris.clementine /Player org.freedesktop.MediaPlayer.Pause
// $ qdbus org.mpris.clementine /Player org.freedesktop.MediaPlayer.VolumeSet 50

ClementineDbus::ClementineDbus()
{
  connection = sdbus::createSessionBusConnection();
  createPlayerProxy();
  createTrackListProxy();
}

ClementineDbus::~ClementineDbus() = default;

void ClementineDbus::createPlayerProxy()
{
  playerProxy = sdbus::createObjectProxy(
    *connection, CLEMENTINE_SERVICE_NAME, PLAYER_OBJECT_PATH);
}

void ClementineDbus::createTrackListProxy()
{
  trackListProxy = sdbus::createObjectProxy(
    *connection, CLEMENTINE_SERVICE_NAME, TRACK_LIST_OBJECT_PATH);
}

void ClementineDbus::playerPlay()
{
  fmt::print("Play\n");
  playerProxy->callMethod("Play").onInterface(MEDIA_PLAYER_INTERFACE_NAME);
}

void ClementineDbus::playerStop()
{
  fmt::print("Stop\n");
  playerProxy->callMethod("Stop").onInterface(MEDIA_PLAYER_INTERFACE_NAME);
}

void ClementineDbus::playerPause()
{
  fmt::print("Pause\n");
  playerProxy->callMethod("Pause").onInterface(MEDIA_PLAYER_INTERFACE_NAME);
}

void ClementineDbus::playerPrev()
{
  fmt::print("Prev\n");
  playerProxy->callMethod("Prev").onInterface(MEDIA_PLAYER_INTERFACE_NAME);
}

void ClementineDbus::playerNext()
{
  fmt::print("Next\n");
  playerProxy->callMethod("Next").onInterface(MEDIA_PLAYER_INTERFACE_NAME);
}

void ClementineDbus::playerMute()
{
  fmt::print("Mute\n");
  playerProxy->callMethod("Mute").onInterface(MEDIA_PLAYER_INTERFACE_NAME);
}

int ClementineDbus::getPlayerPosition()
{
  int pos;
  playerProxy->callMethod("PositionGet")
    .onInterface(MEDIA_PLAYER_INTERFACE_NAME)
    .storeResultsTo(pos);
  return pos;
}

void ClementineDbus::setPlayerPosition(int pos)
{
  playerProxy->callMethod("PositionSet")
    .onInterface(MEDIA_PLAYER_INTERFACE_NAME)
    .withArguments(pos);
}

string ClementineDbus::getPlayerCurrentPath()
{
  string path;

  sd_bus_error error = SD_BUS_ERROR_NULL;
  sd_bus_message* reply = nullptr;
  sd_bus* bus = nullptr;

  // Connect to the Session bus (not System bus)
  checkSdbusCall(sd_bus_open_user(&bus));

  checkSdbusCall(
    sd_bus_call_method(
      bus,
      "org.mpris.clementine", // service to contact
      "/Player", // object path
      "org.freedesktop.MediaPlayer", // interface name
      "GetMetadata", // method name
      &error, // object to return error in
      &reply, // return message on success
      "" // input signature
      // second argument
      ));

  // Parse the response message
  checkSdbusCall(
    sd_bus_message_enter_container(reply, SD_BUS_TYPE_ARRAY, "{sv}"));

  while (sd_bus_message_enter_container(reply, SD_BUS_TYPE_DICT_ENTRY, "sv")
         > 0) {
    const char* key;
    const char* value;
    char type;

    checkSdbusCall(sd_bus_message_read_basic(reply, SD_BUS_TYPE_STRING, &key));
    checkSdbusCall(sd_bus_message_peek_type(reply, &type, &value));

    if (type == SD_BUS_TYPE_VARIANT) {
      checkSdbusCall(
        sd_bus_message_enter_container(reply, SD_BUS_TYPE_VARIANT, value));
      checkSdbusCall(sd_bus_message_peek_type(reply, &type, &value));
      if (type == SD_BUS_TYPE_STRING) {
        checkSdbusCall(sd_bus_message_read_basic(reply, type, &value));
        // std::cout << key << ": " << value << std::endl;
        if (string(key) == "location") {
          path = value;
        }
      }
      else {
        sd_bus_message_skip(reply, nullptr);
      }
      checkSdbusCall(sd_bus_message_exit_container(reply));
    }
    checkSdbusCall(sd_bus_message_exit_container(reply));
  }

  sd_bus_error_free(&error);
  sd_bus_message_unref(reply);
  sd_bus_unref(bus);

  replace_all(path, "file://", "");

  return path;
}

void ClementineDbus::getPlayerMetadata()
{
  fmt::print("0\n");

  std::map<std::string, sdbus::Variant> r;
  playerProxy->callMethod("GetMetadata")
    .onInterface(MEDIA_PLAYER_INTERFACE_NAME)
    .storeResultsTo(r);

  fmt::print("1\n");
}

int ClementineDbus::getTrackListCurrentTrack()
{
  int trackIdx;
  trackListProxy->callMethod("GetCurrentTrack")
    .onInterface(MEDIA_PLAYER_INTERFACE_NAME)
    .storeResultsTo(trackIdx);
  return trackIdx;
}

void ClementineDbus::removeTrackListCurrentTrack()
{
  auto trackIdx = getTrackListCurrentTrack();
  removeTrackListTrack(trackIdx);
}

void ClementineDbus::removeTrackListTrack(int trackIdx)
{
  trackListProxy->callMethod("DelTrack")
    .onInterface(MEDIA_PLAYER_INTERFACE_NAME)
    .withArguments(trackIdx);
}

void ClementineDbus::checkSdbusCall(int r)
{
  if (r < 0) {
    fprintf(stderr, "SD_BUS call failed: %s\n", strerror(-r));
    exit(r);
  }
}

void KonsoleTabName()
{
  //
  ////  const char* clementineServiceName = "org.kde.konsole";
  ////  const char* playerObjectPath = "/Sessions/1";
  ////  auto playerProxy = sdbus::createObjectProxy(*c.release(),
  /// clementineServiceName, playerObjectPath);
  ////
  ////  const char* mediaPlayerInterfaceName = "org.kde.konsole.Session";
  ////
  ////  playerProxy->callMethod("setHistorySize")
  ////    .onInterface(mediaPlayerInterfaceName).withArguments(1000);
}

void experiments1()
{
  //
  ////  const char* clementineServiceName = "org.mpris.MediaPlayer2.clementine";
  ////  const char* playerObjectPath = "/org/mpris/MediaPlayer2";
  ////  auto playerProxy = sdbus::createObjectProxy(clementineServiceName,
  /// playerObjectPath);
  ////
  ////  auto i = playerProxy.get();
  ////  i->callMethod(
  ////      "Pause"
  ////  ).onInterface("org.freedesktop.MediaPlayer");
  ////
  ////  return 0;
  ////
  ////
  ////  playerProxy->callMethod(
  ////    "Pause"
  ////  ).onInterface(mediaPlayerInterfaceName);
  ////  .withArguments().onInterface(mediaPlayerInterfaceName);
  //
  ////  std::string concatenatedString;
  ////  playerProxy->callMethod("org.freedesktop.MediaPlayer.Pause")
  ////      .onInterface(mediaPlayerInterfaceName).withArguments()
  ////      .storeResultsTo(concatenatedString);
  //
  ////    assert(concatenatedString == "1:2:3");
  //
  //
  ////      .withArguments(numbers,
  /// separator).storeResultsTo(concatenatedString);
  //
  ////  );
  //
  ////
  /// playerProxy->uponSignal("concatenated").onInterface(mediaPlayerInterfaceName).call(
  ////      [this](const std::string& str){ onConcatenated(str); }
  ////  );
  ////  playerProxy->finishRegistration();
  ////
  ////  std::vector<int> numbers = {1, 2, 3};
  ////  std::string separator = ":";
  ////
  ////  // Invoke concatenate on given interface of the object
  ////  {
  ////    std::string concatenatedString;
  ////
  /// playerProxy->callMethod("concatenate").onInterface(mediaPlayerInterfaceName).withArguments(numbers,
  /// separator).storeResultsTo(concatenatedString);
  ////    assert(concatenatedString == "1:2:3");
  ////  }
}

void SystemVolumeUp()
{
  //  const char* CLEMENTINE_SERVICE_NAME = "com.ubuntu.SystemService";
  ////  const char* playerObjectPath = "/";
  ////  auto playerProxy = sdbus::createObjectProxy(*c.release(),
  /// clementineServiceName, playerObjectPath);
  ////
  ////  const char* mediaPlayerInterfaceName = "com.ubuntu.SystemService";
  ////
  ////  playerProxy->callMethod("is_reboot_required")
  ////      .onInterface(mediaPlayerInterfaceName);
  ////
  ////  return 0;
}
