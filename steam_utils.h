#pragma once

#include "steam_base.h"
#include "steamworks/public/steam/isteamutils.h"

namespace steam {

class Utils {
  STEAM_IFACE_DECL(Utils);
  friend struct CallResultBase;

  // TODO: return expected ?
  pair<int, int> imageSize(int image_id) const;
  vector<uint8> imageData(int image_id) const;

  unsigned appId() const;
};
}
