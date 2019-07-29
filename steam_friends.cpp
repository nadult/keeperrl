#include "steam_internal.h"
#include "steam_friends.h"
#include <map>

#define FUNC(name, ...) SteamAPI_ISteamFriends_##name

namespace steam {

struct Friends::Impl {
  intptr_t ptr;
  std::map<CSteamID, string> userNames;

  optional<string> updateUserName(CSteamID userID) {
    string name = FUNC(GetFriendPersonaName)(ptr, userID);
    if (name.empty() || name == "[unknown]")
      return none;
    userNames[userID] = name;
    return name;
  }

  // TODO: is it ok on windows?
  STEAM_CALLBACK_MANUAL(Friends::Impl, onPersonaStateChange, PersonaStateChange_t, personaStateChange);
};

void Friends::Impl::onPersonaStateChange(PersonaStateChange_t* result) {
}

Friends::Friends(intptr_t ptr) : ptr(ptr) {
  impl->ptr = ptr;
  impl->personaStateChange.Register(impl.get(), &Impl::onPersonaStateChange);
}
Friends::~Friends() {
  impl->personaStateChange.Unregister();
}

int Friends::count(unsigned flags) const {
  return FUNC(GetFriendCount)(ptr, flags);
}

vector<CSteamID> Friends::ids(unsigned flags) const {
  int count = this->count(flags);
  vector<CSteamID> out;
  out.reserve(count);
  for (int n = 0; n < count; n++)
    out.emplace_back(CSteamID(FUNC(GetFriendByIndex)(ptr, n, flags)));
  return out;
}

void Friends::requestUserInfo(CSteamID userId, bool nameOnly) {
  FUNC(RequestUserInformation)(ptr, userId, nameOnly);
}

optional<string> Friends::retrieveUserName(CSteamID userID) {
  auto it = impl->userNames.find(userID);
  if (it != impl->userNames.end())
    return it->second;
  return impl->updateUserName(userID);
}

string Friends::name() const {
  return FUNC(GetPersonaName)(ptr);
}

optional<int> Friends::retrieveUserAvatar(CSteamID friend_id, AvatarSize size) {
  int value = 0;
  if (size == AvatarSize::small)
    value = FUNC(GetSmallFriendAvatar)(ptr, friend_id);
  else if (size == AvatarSize::medium)
    value = FUNC(GetMediumFriendAvatar)(ptr, friend_id);
  else if (size == AvatarSize::large)
    value = FUNC(GetLargeFriendAvatar)(ptr, friend_id);
  return value ? value : optional<int>();
}
}
