#include <string>
#include <sstream>
#include <vector>
#include <functional>

#include "clementine_dbus.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

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

static string variantToString(sdbus::Variant& value)
{
  const string type = value.peekValueType();
  if (type == "i") return fmt::format("{}", value.get<int32_t>());
  if (type == "x") return fmt::format("{}", value.get<int64_t>());
  if (type == "d") return fmt::format("{}", value.get<double>());
  if (type == "s") return value.get<string>();
  if (type == "as") { auto v = value.get<vector<string>>(); return fmt::format("[{}]", fmt::join(v, ", ")); }
  return fmt::format("<{}>", type);
}

void ClementineDbus::playerNext()
{
  fmt::print("Next\n");

  auto m = getMetadataMap();

  for (auto& [key, value] : m) {
    fmt::print("{} - {}\n", key, variantToString(value));
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
#include <chrono>

void onSeeked3(int64_t v);
void threadFunction();

static std::function<void(const MetadataMap&)> g_onTrackChanged;

void ClementineDbus::setOnTrackChanged(std::function<void(const MetadataMap&)> cb)
{
  g_onTrackChanged = std::move(cb);
}

void ClementineDbus::dumpTagsOnTrackChange()
{
  setOnTrackChanged([](const MetadataMap& metadata) {
    try {
      auto urlIt = metadata.find("xesam:url");
      if (urlIt == metadata.end()) {
        fmt::print("dumpTagsOnTrackChange: no xesam:url in metadata\n");
        return;
      }
      auto path = urlIt->second.get<std::string>();
      if (auto pos = path.find("file://"); pos != std::string::npos)
        path.erase(pos, 7);
      path = decodeUrl(path);
      fmt::print("--- Tags for: {} ---\n", path);
      dump(path);
    } catch (const std::exception& e) {
      fmt::print("dumpTagsOnTrackChange error: {}\n", e.what());
    }
  });
}

void launchThread()
{
  std::thread(&threadFunction).detach();
}

void onTrackMetadataChanged(const MetadataMap& metadata)
{
  if (g_onTrackChanged)
    g_onTrackChanged(metadata);
}

void threadFunction()
{
  while (true) {
    try {
      fmt::print("Signal thread: connecting...\n");
      auto connection = sdbus::createSessionBusConnection();
      auto playerProxy = sdbus::createProxy(
          *connection,
          sdbus::ServiceName{CLEMENTINE_SERVICE_NAME},
          sdbus::ObjectPath{PLAYER_OBJECT_PATH}
      );

      playerProxy->uponSignal("Seeked").onInterface(MEDIA_PLAYER_INTERFACE_NAME).call([](const int64_t v) {
        onSeeked3(v);
      });

      static std::string lastTrackId;

      playerProxy->uponSignal("PropertiesChanged")
        .onInterface("org.freedesktop.DBus.Properties")
        .call([](const std::string& /*interfaceName*/,
                 std::map<std::string, sdbus::Variant> changedProperties,
                 const std::vector<std::string>& /*invalidated*/) {
          auto it = changedProperties.find("Metadata");
          if (it == changedProperties.end())
            return;
          auto metadata = it->second.get<MetadataMap>();
          auto tidIt = metadata.find("mpris:trackid");
          if (tidIt == metadata.end())
            return;
          std::string newTrackId = tidIt->second.get<std::string>();
          if (newTrackId == lastTrackId)
            return;
          lastTrackId = newTrackId;
          fmt::print("Track changed: {}\n", newTrackId);
          onTrackMetadataChanged(metadata);
        });

      connection->enterEventLoop();
      fmt::print("Signal thread: event loop exited, reconnecting...\n");
    } catch (const sdbus::Error& e) {
      fmt::print("Signal thread error: {}\n", e.what());
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
  }
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

