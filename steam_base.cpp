#include "steam_internal.h"
#include "steam_base.h"

namespace steam {

bool initAPI() {
  return SteamAPI_Init();
}

void runCallbacks() {
  SteamAPI_RunCallbacks();
}

string formatError(int value, const pair<int, const char*>* strings, int count) {
  for (int n = 0; n < count; n++)
    if (strings[n].first == value)
      return strings[n].second;
  char buffer[64];
  sprintf(buffer, "unknown (%d)", value);
  return buffer;
}

static const pair<int, const char*> results[] = {
#define CASE(item)                                                                                                     \
  { k_EResult##item, #item }
    CASE(Fail),
    CASE(NoConnection),
    CASE(InvalidPassword),
    CASE(InvalidParam),
    CASE(InsufficientPrivilege),
    CASE(Banned),
    CASE(Timeout),
    CASE(NotLoggedOn),
    CASE(ServiceUnavailable),
    CASE(InvalidParam),
    CASE(AccessDenied),
    CASE(LimitExceeded),
    CASE(FileNotFound),
    CASE(DuplicateRequest),
    CASE(DuplicateName),
    CASE(ServiceReadOnly),
    CASE(LockingFailed),
#undef CASE
};

string errorText(EResult value) {
  if (value == k_EResultOK)
    return "";
  return formatError(value, results, arraySize(results));
}

string itemStateText(unsigned bits) {
  static const char* names[] = {"subscribed",   "legacy_item", "installed",
                                "needs_update", "downloading", "download_pending"};

  if (bits == k_EItemStateNone)
    return "none";

  string out;
  for (int n = 0; n < arraySize(names); n++)
    if (bits & (1 << n)) {
      if (!out.empty())
        out += ' ';
      out += names[n];
    }
  return out;
}

vector<string> validTags() {
  return {"Alpha 28", "Alpha 29", "Mod", "Dungeon"};
}
}
