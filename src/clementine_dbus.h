#pragma once

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <map>

#include "int_types.h"

typedef std::unique_ptr<sdbus::IProxy> IProxyPtr;
typedef std::unique_ptr<sdbus::IConnection> IConnectionPtr;

typedef std::map<std::string, sdbus::Variant> MetadataMap;

class ClementineDbus
{
  public:
  ClementineDbus();
  ~ClementineDbus();
  // Player
  void playerPlay();
  void playerStop();
  void playerPause();
  void playerPlayPause();
  void playerPrev();
  void playerNext();
  void playerMute();
  void volumeUp();
  void volumeDown();
  double getVolume();
  void setVolume(double vol);
  s64 getPlayerPosition();
  void setPlayerPosition(const std::string& trackId, s64 pos);
  std::string getPlayerCurrentPath();
  // TrackList
  std::string getCurrentTrackId();
  void removeCurrentTrackFromPlaylist();
  void removeTrackFromPlaylist(const std::string& trackId);

  private:
  void createPlayerProxy();

  IProxyPtr playerProxy;
  IConnectionPtr connection;
  MetadataMap getMetadataMap() const;
};

void launchThread();
