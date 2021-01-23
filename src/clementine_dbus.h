#pragma once

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <map>

#include "int_types.h"

typedef std::unique_ptr<sdbus::IProxy> IProxyPtr;
typedef std::unique_ptr<sdbus::IConnection> IConnectionPtr;

typedef std::map<std::string, sdbus::Variant> MetadataMap;

// void onSeeked(const int64_t v);
void onSeeked(sdbus::Signal& signal);
// void onTrackMetadataChanged(sdbus::Signal& signal);

class ClementineDbus
{
  public:
  ClementineDbus();
  ~ClementineDbus();
  // Player
  void playerPlay();
  void playerStop();
  void playerPause();
  void playerPrev();
  void playerNext();
  void playerMute();
  void volumeUp();
  void volumeDown();
  double getVolume();
  void setVolume(double vol);
  s64 getPlayerPosition();
  void setPlayerPosition(std::string trackId, s64 pos);
  std::string getPlayerCurrentPath();
  // TrackList
  std::string getCurrentTrackId();
  void removeCurrentTrackFromPlaylist();
  void removeTrackFromPlaylist(std::string trackId);
  //
  private:
  void createPlayerProxy();

  void dump();

  //  void registerSeekedHandler();
  //  void registerTrackMedatadataChangedHandler();

  IProxyPtr playerProxy;
  IProxyPtr trackListProxy;
  IConnectionPtr connection;
  MetadataMap getMetadataMap() const;
};

// class ClemListen
//{
// public:
//  ClemListen();
//  ~ClemListen();
//  void registerSeekedHandler();
//  IConnectionPtr connection;
//};
//
//

void launch_thread();
