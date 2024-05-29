#include <string>
#include <sstream>

#include "clementine_dbus.h"

#include <fmt/format.h>
#include <boost/algorithm/string/replace.hpp>
#include <systemd/sd-bus.h>

#include "tag.h"

using namespace std;
using namespace boost::algorithm;

auto CLEMENTINE_SERVICE_NAME = sdbus::ServiceName("org.mpris.MediaPlayer2.clementine");
auto PLAYER_OBJECT_PATH =          sdbus::ObjectPath("/org/mpris/MediaPlayer2");
auto MEDIA_PLAYER_INTERFACE_NAME = sdbus::InterfaceName("org.mpris.MediaPlayer2.Player");
auto TRACK_LIST_INTERFACE_NAME =   sdbus::InterfaceName("org.mpris.MediaPlayer2.TrackList");

auto DBUS_SERVICE_NAME = sdbus::ServiceName("org.freedesktop.DBus");
auto DBUS_OBJECT_PATH = sdbus::ObjectPath("/");


std::string decodeUrl(const std::string& url);

ClementineDbus::ClementineDbus()
{
  connection = sdbus::createSessionBusConnection();
  createPlayerProxy();

  //  registerSeekedHandler();
  //  playerProxy->finishRegistration();

  // After calling enterProcessingLoopAsync(), I get random crashes, even
  // without having registered any handlers.

  // Start separate thread that listens for signals and calls handlers.
  //   connection->enterProcessingLoopAsync();
}

ClementineDbus::~ClementineDbus() = default;

void ClementineDbus::createPlayerProxy()
{
  playerProxy = sdbus::createProxy(*connection, CLEMENTINE_SERVICE_NAME, PLAYER_OBJECT_PATH);
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

void ClementineDbus::playerPlayPause()
{
  fmt::print("PlayPause\n");
  playerProxy->callMethod("PlayPause").onInterface(MEDIA_PLAYER_INTERFACE_NAME);
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
  return static_cast<double>(playerProxy->getProperty("Volume").onInterface(MEDIA_PLAYER_INTERFACE_NAME));
}

void ClementineDbus::setVolume(double vol)
{
  playerProxy->setProperty("Volume").onInterface(MEDIA_PLAYER_INTERFACE_NAME).toValue(vol);
}

s64 ClementineDbus::getPlayerPosition()
{
  return static_cast<s64>(playerProxy->getProperty("Position").onInterface(MEDIA_PLAYER_INTERFACE_NAME)) / 1000;
}

void ClementineDbus::setPlayerPosition(const string& trackId, s64 pos)
{
  playerProxy->callMethod("SetPosition")
    .onInterface(MEDIA_PLAYER_INTERFACE_NAME)
    .withArguments(sdbus::ObjectPath(trackId), pos * 1000);
}

string ClementineDbus::getPlayerCurrentPath()
{
  MetadataMap metadataMap = getMetadataMap();
  auto path = metadataMap["xesam:url"].get<string>();
  replace_first(path, "file://", "");
  return decodeUrl(path);
}

MetadataMap ClementineDbus::getMetadataMap() const
{
  auto metadataMap = playerProxy->getProperty("Metadata").onInterface(MEDIA_PLAYER_INTERFACE_NAME).get<MetadataMap>();
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

void ClementineDbus::removeTrackFromPlaylist(const string& trackId)
{
  fmt::print("Removing from playlist: {}\n", trackId);
  playerProxy->callMethod("RemoveTrack")
    .onInterface(TRACK_LIST_INTERFACE_NAME)
    .withArguments(sdbus::ObjectPath(trackId));
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

void onSeeked3(int64_t v);
// void onTrackMetadataChanged(sdbus::Variant);
void threadFunction();

void launchThread()
{
  auto t = new std::thread(&threadFunction);
}

void onTrackMetadataChanged(sdbus::Variant& m)
{
  // auto p =  getPlayerCurrentPath();
  // auto x = getMyRating("");
  //
  // // fmt::print("###\n");
  //
  // //  fmt::print("*******************************************
  // //  onTrackMetadataChanged {}\n",
  // //             m);// {}\n", v);
  //
  // //  for (auto &v : m) {
  // //    fmt::print("{} - {}\n", v.first, v.second.peekValueType());
  // //    if (v.second.peekValueType() == "i") {
  // //      fmt::print("{} - {}\n", v.first, v.second.get<int>());
  // //    } else if (v.second.peekValueType() == "s") {
  // //      fmt::print("{} - {}\n", v.first, v.second.get<string>());
  // //    }
  // //  }
}

void threadFunction()
{
  fmt::print("threadFunction()\n");

  auto connection = sdbus::createSessionBusConnection();
  auto playerProxy = sdbus::createProxy(*connection, CLEMENTINE_SERVICE_NAME, PLAYER_OBJECT_PATH);

  playerProxy->uponSignal("Seeked").onInterface(MEDIA_PLAYER_INTERFACE_NAME).call([](const int64_t v) {
    onSeeked3(v);
  });

  // playerProxy->uponSignal("PropertiesChanged").onInterface(MEDIA_PLAYER_INTERFACE_NAME).call([](const int64_t v) {
  //     propertiesChanged(v);
  // });

  // signal time=1548026128.798239 sender=:1.18 -> destination=(null
  // destination) serial=1375 path=/org/mpris/MediaPlayer2;
  // interface=org.freedesktop.DBus.Properties; member=PropertiesChanged
  //   string "org.mpris.MediaPlayer2.Player"

  auto propProxy = sdbus::createProxy(*connection, DBUS_SERVICE_NAME, DBUS_OBJECT_PATH);

  //  playerProxy->uponSignal("PropertiesChanged").onInterface("org.freedesktop.DBus.Properties")
  //    .call([](sdbus::Signal& v) { onTrackMetadataChanged(v); });

  playerProxy->uponSignal("PropertiesChanged").onInterface("org.freedesktop.DBus.Properties").call([](sdbus::Variant v) {
    onTrackMetadataChanged(v);
  });

  // playerProxy->finishRegistration();
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
  fmt::print("event: onSeeked3 {}\n", v);
}

std::string decodeUrl(const string& url)
{
  std::ostringstream decodedUrl;
  for (std::size_t i = 0; i < url.size(); ++i) {
    if (url[i] == '%') {
      int value;
      std::istringstream is(url.substr(i + 1, 2));
      is >> std::hex >> value;
      decodedUrl << static_cast<char>(value);
      i += 2;
    }
    else {
      decodedUrl << url[i];
    }
  }
  return decodedUrl.str();
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
