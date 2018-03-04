#pragma once

#include <sdbus-c++/sdbus-c++.h>
#include <string>

typedef std::unique_ptr<sdbus::IObjectProxy> IProxyPtr;
typedef std::unique_ptr<sdbus::IConnection> IConnectionPtr;

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
  int getPlayerPosition();
  void setPlayerPosition(int pos);
  void getPlayerMetadata();
  std::string getPlayerCurrentPath();
  // TrackList
  int getTrackListCurrentTrack();
  void removeTrackListCurrentTrack();
  void removeTrackListTrack(int trackIdx);
  //
  private:
  void createPlayerProxy();
  void createTrackListProxy();
  void checkSdbusCall(int);

  IProxyPtr playerProxy;
  IProxyPtr trackListProxy;
  IConnectionPtr connection;
};
