#include "clementine_dbus.h"

#include <algorithm>
#include <fmt/format.h>

#include <boost/filesystem/convenience.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <systemd/sd-bus.h>

using namespace std;
using namespace boost::filesystem;
using namespace boost::algorithm;

// Ubuntu 16.04

// const char* CLEMENTINE_SERVICE_NAME = "org.mpris.clementine";
// const char* PLAYER_OBJECT_PATH = "/Player";
// const char* TRACK_LIST_OBJECT_PATH = "/TrackList";
// const char* MEDIA_PLAYER_INTERFACE_NAME = "org.freedesktop.MediaPlayer";

// Ubuntu 18.04

const char* CLEMENTINE_SERVICE_NAME = "org.mpris.MediaPlayer2.clementine";
const char* PLAYER_OBJECT_PATH = "/org/mpris/MediaPlayer2";
const char* MEDIA_PLAYER_INTERFACE_NAME = "org.mpris.MediaPlayer2.Player";
const char* TRACK_LIST_INTERFACE_NAME = "org.mpris.MediaPlayer2.TrackList";

ClementineDbus::ClementineDbus()
{
  connection = sdbus::createSessionBusConnection();
  createPlayerProxy();

  //  playerProxy->uponSignal("Seeked").onInterface(
  //    MEDIA_PLAYER_INTERFACE_NAME
  //  ).call([](const int64_t v){ onS(v); });

  //    registerSeekedHandler();
  //    playerProxy->finishRegistration();

  // After calling enterProcessingLoopAsync(), I get random crashes, even
  // without having registered any handlers.

  // Start separate thread that listens for signals and calls handlers.
  //   connection->enterProcessingLoopAsync();
}

ClementineDbus::~ClementineDbus() = default;

void ClementineDbus::createPlayerProxy()
{
  playerProxy = sdbus::createObjectProxy(
    *connection, CLEMENTINE_SERVICE_NAME, PLAYER_OBJECT_PATH);
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
  playerProxy->callMethod("Previous").onInterface(MEDIA_PLAYER_INTERFACE_NAME);
}

void ClementineDbus::playerNext()
{
  fmt::print("Next\n");

  auto m = getMetadataMap();

  for (auto& v : m) {
    fmt::print("{} - {}\n", v.first, v.second.peekValueType());
    if (v.second.peekValueType() == "i") {
      fmt::print("{} - {}\n", v.first, v.second.get<int>());
    }
    else if (v.second.peekValueType() == "s") {
      fmt::print("{} - {}\n", v.first, v.second.get<string>());
    }
  }

  playerProxy->callMethod("Next").onInterface(MEDIA_PLAYER_INTERFACE_NAME);
}

void ClementineDbus::playerMute()
{
  fmt::print("Mute\n");
  setVolume(0.0);
}

void ClementineDbus::volumeUp()
{
  fmt::print("Clem vol up\n");
  setVolume(getVolume() + 0.05);
}

void ClementineDbus::volumeDown()
{
  fmt::print("Clem vol down\n");
  setVolume(getVolume() - 0.05);
}

double ClementineDbus::getVolume()
{
  return static_cast<double>(playerProxy->getProperty("Volume").onInterface(
    MEDIA_PLAYER_INTERFACE_NAME));
}

void ClementineDbus::setVolume(double vol)
{
  playerProxy->setProperty("Volume")
    .onInterface(MEDIA_PLAYER_INTERFACE_NAME)
    .toValue(vol);
}

s64 ClementineDbus::getPlayerPosition()
{
  return static_cast<s64>(playerProxy->getProperty("Position")
                            .onInterface(MEDIA_PLAYER_INTERFACE_NAME))
         / 1000;
}

void ClementineDbus::setPlayerPosition(string trackId, s64 pos)
{
  playerProxy->callMethod("SetPosition")
    .onInterface(MEDIA_PLAYER_INTERFACE_NAME)
    .withArguments(sdbus::ObjectPath(trackId.c_str()), pos * 1000);
}

string ClementineDbus::getPlayerCurrentPath()
{
  MetadataMap metadataMap = getMetadataMap();
  auto path = metadataMap["xesam:url"].get<string>();
  replace_first(path, "file://", "");
  return path;
}

MetadataMap ClementineDbus::getMetadataMap() const
{
  MetadataMap metadataMap = playerProxy->getProperty("Metadata")
                              .onInterface(MEDIA_PLAYER_INTERFACE_NAME);
  return metadataMap;
}

string ClementineDbus::getCurrentTrackId()
{
  MetadataMap metadataMap = getMetadataMap();
  return metadataMap["mpris:trackid"].get<string>();
}

void ClementineDbus::removeCurrentTrackFromPlaylist()
{
  auto trackId = getCurrentTrackId();
  removeTrackFromPlaylist(trackId);
}

void ClementineDbus::removeTrackFromPlaylist(string trackId)
{
  fmt::print("Removing from playlist: {}\n", trackId);
  playerProxy->callMethod("RemoveTrack")
    .onInterface(TRACK_LIST_INTERFACE_NAME)
    .withArguments(sdbus::ObjectPath(trackId.c_str()));
}

// void ClementineDbus::registerSeekedHandler()
//{
//  playerProxy->registerSignalHandler(
//    MEDIA_PLAYER_INTERFACE_NAME, "Seeked", &onSeeked);
//}
//
// void ClementineDbus::registerTrackMedatadataChangedHandler()
//{
//    trackListProxy->registerSignalHandler(
//      TRACK_LIST_INTERFACE_NAME, "TrackMetadataChanged",
//      &onTrackMetadataChanged);
//}
//
// void onSeeked(sdbus::Signal& signal)
// void onSeeked(const int64_t v)
//{
//  std::cout << "Signal: Seeked:" << std::endl;
//  int64_t x;
//  signal >> x;
//  std::cout << v << std::endl;
//}
//
// void onTrackMetadataChanged(sdbus::Signal& signal)
//{
//  std::cout << "Signal: TrackMetadataChanged:" << std::endl;
//  //  signal.closeVariant();
//  //  signal.clearFlags();
//
//  //  std::map<std::string, sdbus::Variant> metadataMap;
//  //  auto path = metadataMap["xesam:url"].get<string>();
//  //  replace_first(path, "file://", "");
//  //  return path;
//
//  //  int64_t x;
//  //  std::cout << signal.getMemberName() << std::endl; // >> metadataMap;
//  //  std::cout << "Signal: TrackMetadataChanged: " << x << std::endl;
//}
