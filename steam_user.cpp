#include "steam_internal.h"
#include "steam_user.h"

#define FUNC(name, ...) SteamAPI_ISteamUser_##name

namespace steam {

User::User(intptr_t ptr) : ptr(ptr) {
}
User::~User() = default;

CSteamID User::id() const {
  return FUNC(GetSteamID)(ptr);
}

bool User::isLoggedOn() const {
  return FUNC(BLoggedOn)(ptr);
}
}
