#pragma once

#include "steam_base.h"

// TODO: remove it, EFriendFlag
#include "steamworks/public/steam/isteamfriends.h"

RICH_ENUM(SteamAvatarSize, small, medium, large);

namespace steam {
using AvatarSize = SteamAvatarSize;

class Friends {
  STEAM_IFACE_DECL(Friends)

  int count(unsigned flags = k_EFriendFlagAll) const;
  vector<UserId> ids(unsigned flags = k_EFriendFlagAll) const;

  void requestUserInfo(UserId, bool nameOnly);
  optional<string> retrieveUserName(UserId);
  optional<int> retrieveUserAvatar(UserId, AvatarSize);

  string name() const;

  private:
  struct Impl;
  HeapAllocated<Impl> impl;
};
}
