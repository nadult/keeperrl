#pragma once

#include "steam_base.h"
#include "steamworks/public/steam/isteamfriends.h"

RICH_ENUM(SteamAvatarSize, small, medium, large);

namespace steam {
using AvatarSize = SteamAvatarSize;

class Friends {
  STEAM_IFACE_DECL(Friends)

  // TODO: wrap CSteamID somehow?
  // TODO: add tagged id type?

  int count(unsigned flags = k_EFriendFlagAll) const;
  vector<CSteamID> ids(unsigned flags = k_EFriendFlagAll) const;

  void requestUserInfo(CSteamID, bool nameOnly);
  optional<string> retrieveUserName(CSteamID);
  optional<int> retrieveUserAvatar(CSteamID, AvatarSize);

  string name() const;

  private:
  struct Impl;
  HeapAllocated<Impl> impl;
};
}
