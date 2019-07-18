#pragma once

#include <steamworks/public/steam/steamclientpublic.h>
#include "util.h"

// Notes:
//
// - Whole interface is NOT thread safe, it should be used on a single thread only
//
// - Most big classes match steamworks interfaces (Client, Friends, etc.) but some
//   interfaces (like UserStats) are merged into Client class (for convenience).
//
// - Client interface is a singleton and holds all the other interfaces within itself
//   user only has to create Client to get access to all the other interfaces.

// TODO: namespace
RICH_ENUM(QueryStatus, invalid, pending, completed, failed);

namespace steam {

#define STEAM_IFACE_DECL(name)                                                                                         \
  intptr_t ptr;                                                                                                        \
  name(intptr_t);                                                                                                      \
  friend class Client;                                                                                                 \
                                                                                                                       \
  public:                                                                                                              \
  name(const name&) = delete;                                                                                          \
  void operator=(const name&) = delete;                                                                                \
  ~name();                                                                                                             \
                                                                                                                       \
  static name& instance();

#define STEAM_IFACE_IMPL(name)                                                                                         \
  name::name(intptr_t ptr) : ptr(ptr) {                                                                                \
  }                                                                                                                    \
  name::~name() = default;

class Client;
class Friends;
class UGC;
class Utils;
class User;

bool initAPI();
void runCallbacks();

string formatError(int value, const pair<int, const char*>* strings, int count);
string errorText(EResult);
string itemStateText(unsigned bits);

template <class T> class CallResult;
}
