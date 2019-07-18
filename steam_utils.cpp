#include "steam_internal.h"
#include "steam_utils.h"

#define FUNC(name, ...) SteamAPI_ISteamUtils_##name

namespace steam {

Utils::Utils(intptr_t ptr) : ptr(ptr) {
}
Utils::~Utils() = default;

pair<int, int> Utils::imageSize(int image_id) const {
  uint32_t w = 0, h = 0;
  if (!FUNC(GetImageSize)(ptr, image_id, &w, &h))
    CHECK(false);
  return {w, h};
}

vector<uint8> Utils::imageData(int image_id) const {
  auto size = imageSize(image_id);
  vector<uint8> out(size.first * size.second * 4);
  if (!FUNC(GetImageRGBA)(ptr, image_id, out.data(), out.size()))
    CHECK(false);
  return out;
}
unsigned Utils::appId() const {
  return FUNC(GetAppID)(ptr);
}
}
