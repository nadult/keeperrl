#pragma once

#include "steam_base.h"
#include <steamworks/public/steam/isteamuser.h>

namespace steam {
class User {
  STEAM_IFACE_DECL(User)

  CSteamID id() const;
};
}
