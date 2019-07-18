#pragma once

#include "steam_base.h"
#include <steamworks/public/steam/isteamfriends.h>

namespace steam {
class Friends {
  STEAM_IFACE_DECL(Friends)

  // TODO: wrap CSteamID somehow?
  // TODO: add tagged id type?

  int count(unsigned flags = k_EFriendFlagAll) const;
  vector<CSteamID> ids(unsigned flags = k_EFriendFlagAll) const;

  string name(CSteamID friend_id) const;

  // TODO: name it differently to signify that it involves a callback?
  int avatar(CSteamID friend_id, int size) const;
};
}
