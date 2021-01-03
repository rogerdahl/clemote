#include "clementine_dbus.h"

#include <algorithm>
#include <chrono>
#include <thread>

#include <boost/filesystem/convenience.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <fmt/format.h>
#include <systemd/sd-bus.h>

using namespace std;
using namespace boost::filesystem;
using namespace boost::algorithm;

const char* CLEMENTINE_SERVICE_NAME = "org.mpris.MediaPlayer2.clementine";
const char* PLAYER_OBJECT_PATH = "/org/mpris/MediaPlayer2";
const char* MEDIA_PLAYER_INTERFACE_NAME = "org.mpris.MediaPlayer2.Player";
const char* TRACK_LIST_INTERFACE_NAME = "org.mpris.MediaPlayer2.TrackList";

ClementineDbus::ClementineDbus()
{
  connection = sdbus::createSessionBusConnection();
  createProxy();

  waitForClementine();

  //  registerSeekedHandler();
  //  playerProxy->finishRegistration();

  // After calling enterProcessingLoopAsync(), I get random crashes, even
  // without having registered any handlers.

  // Start separate thread that listens for signals and calls handlers.
  //   connection->enterProcessingLoopAsync();
}

ClementineDbus::~ClementineDbus() = default;

void ClementineDbus::createProxy()
{
  playerProxy =
    sdbus::createProxy(*connection, CLEMENTINE_SERVICE_NAME, PLAYER_OBJECT_PATH);
}

// Wait for Clementine to show up on the D-Bus
void ClementineDbus::waitForClementine()
{
  while (true) {
    try {
      getPlayerPosition();
    } catch (std::exception&) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }
    break;
  }
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

// WORKS
// void ClementineDbus::registerSeekedHandler() {
//  playerProxy->uponSignal("Seeked").onInterface(MEDIA_PLAYER_INTERFACE_NAME)
//    .call([](const int64_t v) { onSeeked2(v); });
//}

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

////////////////////////////////

#include <thread>

void onSeeked3(const int64_t v);
// void onTrackMetadataChanged(sdbus::Variant);
void thread_function();

void launch_thread()
{
  std::thread* t = new std::thread(&thread_function);
}

void onTrackMetadataChanged(sdbus::Variant& m)
{
  //  fmt::print("*******************************************
  //  onTrackMetadataChanged {}\n",
  //             m);// {}\n", v);
  fmt::print("###\n");

  //  for (auto &v : m) {
  //    fmt::print("{} - {}\n", v.first, v.second.peekValueType());
  //    if (v.second.peekValueType() == "i") {
  //      fmt::print("{} - {}\n", v.first, v.second.get<int>());
  //    } else if (v.second.peekValueType() == "s") {
  //      fmt::print("{} - {}\n", v.first, v.second.get<string>());
  //    }
  //  }
}

void thread_function()
{
  fmt::print("thread_function()\n");

  auto connection = sdbus::createSessionBusConnection();
  auto playerProxy =
    sdbus::createProxy(*connection, CLEMENTINE_SERVICE_NAME, PLAYER_OBJECT_PATH);

  playerProxy->uponSignal("Seeked")
    .onInterface(MEDIA_PLAYER_INTERFACE_NAME)
    .call([](const int64_t v) { onSeeked3(v); });

  // signal time=1548026128.798239 sender=:1.18 -> destination=(null
  // destination) serial=1375 path=/org/mpris/MediaPlayer2;
  // interface=org.freedesktop.DBus.Properties; member=PropertiesChanged
  //   string "org.mpris.MediaPlayer2.Player"

  auto propProxy = sdbus::createProxy(
    *connection, "org.freedesktop.DBus", "org.freedesktop.DBus");

  //  playerProxy->uponSignal("PropertiesChanged").onInterface("org.freedesktop.DBus.Properties")
  //    .call([](sdbus::Signal& v) { onTrackMetadataChanged(v); });

  playerProxy->uponSignal("PropertiesChanged")
    .onInterface("org.freedesktop.DBus.Properties")
    .call([](sdbus::Variant v) { onTrackMetadataChanged(v); });

  playerProxy->finishRegistration();
  connection->enterEventLoop();
}

//  std:vector<sdbus::Variant> t = signal;

//  sdbus::Variant m = signal;//.openContainer();
//  MetadataMap x = m.get();
//  auto path = metadataMap["xesam:url"].get<string>();
//  replace_first(path, "file://", "");

//  MetadataMap metadataMap = playerProxy->getProperty("Metadata")
//    .onInterface(MEDIA_PLAYER_INTERFACE_NAME);
//  return metadataMap;
//
//
//  auto m = getMetadataMap();
//
//  for (auto& v : m) {
//    fmt::print("{} - {}\n", v.first, v.second.peekValueType());
//    if (v.second.peekValueType() == "i") {
//      fmt::print("{} - {}\n", v.first, v.second.get<int>());
//    }
//    else if (v.second.peekValueType() == "s") {
//      fmt::print("{} - {}\n", v.first, v.second.get<string>());
//    }
//  }

//  signal.closeVariant();
//  signal.clearFlags();

//    std::map<std::string, sdbus::Variant> metadataMap;
//    auto path = metadataMap["xesam:url"].get<string>();
//  replace_first(path, "file://", "");
//  return path;

//  int64_t x;
//    std::cout << signal.getMemberName() << std::endl; // >> metadataMap;
//  std::cout << "Signal: TrackMetadataChanged: " << x << std::endl;

void onSeeked3(const int64_t v)
{
  fmt::print("******************************************* onSeeked3 {}\n", v);
}

// signal time=1548026128.798239 sender=:1.18 -> destination=(null destination)
// serial=1375 path=/org/mpris/MediaPlayer2;
// interface=org.freedesktop.DBus.Properties; member=PropertiesChanged
//   string "org.mpris.MediaPlayer2.Player"
//   array [
//      dict entry(
//         string "Metadata"
//         variant             array [
//               dict entry(
//                  string "bitrate"
//                  variant                      int32 275
//               )
//               dict entry(
//                  string "mpris:length"
//                  variant                      int64 312000000
//               )
//               dict entry(
//                  string "mpris:trackid"
//                  variant                      string
//                  "/org/mpris/MediaPlayer2/Track/70"
//               )
//               dict entry(
//                  string "xesam:album"
//                  variant                      string "Supposed Former
//                  Infatuation Junkie"
//               )
//               dict entry(
//                  string "xesam:artist"
//                  variant                      array [
//                        string "Alanis Morissette"
//                     ]
//               )
//               dict entry(
//                  string "xesam:comment"
//                  variant                      array [
//                        string "ED10DA11"
//                     ]
//               )
//               dict entry(
//                  string "xesam:contentCreated"
//                  variant                      string "2019-01-15T17:42:28"
//               )
//               dict entry(
//                  string "xesam:genre"
//                  variant                      array [
//                        string "Pop"
//                     ]
//               )
//               dict entry(
//                  string "xesam:title"
//                  variant                      string "Sympathetic Character"
//               )
//               dict entry(
//                  string "xesam:trackNumber"
//                  variant                      int32 5
//               )
//               dict entry(
//                  string "xesam:url"
//                  variant                      string
//                  "file:///mnt/int60/Music/00__UNSORTED_SCORE_ALL_FIRST_THEN_DELETE_AND_TIDY/Alanis
//                  Morissette - 1998 - Supposed Former Infatuation Junkie - 05
//                  - Sympathetic Character.mp3"
//               )
//               dict entry(
//                  string "year"
//                  variant                      int32 1998
//               )
//            ]
//      )
//   ]
//   array [
//   ]
// signal time=1548026128.798586 sender=:1.18 -> destination=(null destination)
// serial=1376 path=/org/mpris/MediaPlayer2;
// interface=org.freedesktop.DBus.Properties; member=PropertiesChanged
//   string "org.mpris.MediaPlayer2.Player"
//   array [
//      dict entry(
//         string "CanGoNext"
//         variant             boolean true
//      )
//   ]
//   array [
//   ]
// signal time=1548026128.798670 sender=:1.18 -> destination=(null destination)
// serial=1377 path=/org/mpris/MediaPlayer2;
// interface=org.freedesktop.DBus.Properties; member=PropertiesChanged
//   string "org.mpris.MediaPlayer2.Player"
//   array [
//      dict entry(
//         string "CanGoPrevious"
//         variant             boolean true
//      )
//   ]
//   array [
//   ]
// signal time=1548026128.798711 sender=:1.18 -> destination=(null destination)
// serial=1378 path=/org/mpris/MediaPlayer2;
// interface=org.freedesktop.DBus.Properties; member=PropertiesChanged
//   string "org.mpris.MediaPlayer2.Player"
//   array [
//      dict entry(
//         string "CanSeek"
//         variant             boolean true
//      )
//   ]
//   array [
//   ]
// signal time=1548026128.832641 sender=:1.18 -> destination=(null destination)
// serial=1379 path=/org/mpris/MediaPlayer2;
// interface=org.freedesktop.DBus.Properties; member=PropertiesChanged
//   string "org.mpris.MediaPlayer2.Player"
//   array [
//      dict entry(
//         string "PlaybackStatus"
//         variant             string "Playing"
//      )
//   ]
//   array [
//   ]
// signal time=1548026128.832668 sender=:1.18 -> destination=(null destination)
// serial=1380 path=/org/mpris/MediaPlayer2;
// interface=org.freedesktop.DBus.Properties; member=PropertiesChanged
//   string "org.mpris.MediaPlayer2.Player"
//   array [
//      dict entry(
//         string "CanSeek"
//         variant             boolean true
//      )
//   ]
//   array [
//   ]
// signal time=1548026128.835316 sender=:1.18 -> destination=(null destination)
// serial=1381 path=/org/mpris/MediaPlayer2;
// interface=org.freedesktop.DBus.Properties; member=PropertiesChanged
//   string "org.mpris.MediaPlayer2.Player"
//   array [
//      dict entry(
//         string "Metadata"
//         variant             array [
//               dict entry(
//                  string "bitrate"
//                  variant                      int32 275
//               )
//               dict entry(
//                  string "mpris:artUrl"
//                  variant                      string
//                  "file:///tmp/clementine-art-KC2303.jpg"
//               )
//               dict entry(
//                  string "mpris:length"
//                  variant                      int64 312000000
//               )
//               dict entry(
//                  string "mpris:trackid"
//                  variant                      string
//                  "/org/mpris/MediaPlayer2/Track/70"
//               )
//               dict entry(
//                  string "xesam:album"
//                  variant                      string "Supposed Former
//                  Infatuation Junkie"
//               )
//               dict entry(
//                  string "xesam:artist"
//                  variant                      array [
//                        string "Alanis Morissette"
//                     ]
//               )
//               dict entry(
//                  string "xesam:comment"
//                  variant                      array [
//                        string "ED10DA11"
//                     ]
//               )
//               dict entry(
//                  string "xesam:contentCreated"
//                  variant                      string "2019-01-15T17:42:28"
//               )
//               dict entry(
//                  string "xesam:genre"
//                  variant                      array [
//                        string "Pop"
//                     ]
//               )
//               dict entry(
//                  string "xesam:title"
//                  variant                      string "Sympathetic Character"
//               )
//               dict entry(
//                  string "xesam:trackNumber"
//                  variant                      int32 5
//               )
//               dict entry(
//                  string "xesam:url"
//                  variant                      string
//                  "file:///mnt/int60/Music/00__UNSORTED_SCORE_ALL_FIRST_THEN_DELETE_AND_TIDY/Alanis
//                  Morissette - 1998 - Supposed Former Infatuation Junkie - 05
//                  - Sympathetic Character.mp3"
//               )
//               dict entry(
//                  string "year"
//                  variant                      int32 1998
//               )
//            ]
//      )
//   ]
//   array [
//   ]
//
