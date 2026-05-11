#include <string>
#include <sstream>

#include "clementine_dbus.h"

#include <fmt/format.h>

#include "tag.h"

using namespace std;

const std::string CLEMENTINE_SERVICE_NAME = "org.mpris.MediaPlayer2.clementine";
const std::string PLAYER_OBJECT_PATH = "/org/mpris/MediaPlayer2";
const std::string MEDIA_PLAYER_INTERFACE_NAME = "org.mpris.MediaPlayer2.Player";
const std::string TRACK_LIST_INTERFACE_NAME = "org.mpris.MediaPlayer2.TrackList";

std::string decodeUrl(const std::string& url);

ClementineDbus::ClementineDbus()
{
  connection = sdbus::createSessionBusConnection();
  createPlayerProxy();
}

ClementineDbus::~ClementineDbus() = default;

void ClementineDbus::createPlayerProxy()
{
  playerProxy = sdbus::createProxy(*connection, sdbus::ServiceName{CLEMENTINE_SERVICE_NAME}, sdbus::ObjectPath{PLAYER_OBJECT_PATH});
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
  if (auto pos = path.find("file://"); pos != string::npos)
    path.erase(pos, 7);
  return decodeUrl(path);
}

MetadataMap ClementineDbus::getMetadataMap() const
{
  return playerProxy->getProperty("Metadata").onInterface(MEDIA_PLAYER_INTERFACE_NAME).get<MetadataMap>();
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

#include <thread>

void onSeeked3(int64_t v);
void threadFunction();

void launchThread()
{
  std::thread(&threadFunction).detach();
}

void onTrackMetadataChanged([[maybe_unused]] sdbus::Variant& m)
{
}

void threadFunction()
{
  fmt::print("threadFunction()\n");

  auto connection = sdbus::createSessionBusConnection();
  auto playerProxy = sdbus::createProxy(*connection, sdbus::ServiceName{CLEMENTINE_SERVICE_NAME}, sdbus::ObjectPath{PLAYER_OBJECT_PATH});

  playerProxy->uponSignal("Seeked").onInterface(MEDIA_PLAYER_INTERFACE_NAME).call([](const int64_t v) {
    onSeeked3(v);
  });

  playerProxy->uponSignal("PropertiesChanged").onInterface("org.freedesktop.DBus.Properties").call([](sdbus::Variant v) {
    onTrackMetadataChanged(v);
  });

  connection->enterEventLoop();
}

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

