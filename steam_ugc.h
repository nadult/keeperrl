#pragma once

#include "steam_base.h"
#include "steamworks/public/steam/isteamugc.h"

RICH_ENUM(SteamFindOrder, votes, date, subscriptions, playtime);
RICH_ENUM(SteamItemVisibility, public_, friends, private_);

namespace steam {

using FindOrder = SteamFindOrder;
using ItemId = PublishedFileId_t;
using ItemVisibility = SteamItemVisibility;

struct DownloadInfo {
  unsigned long long bytesDownloaded;
  unsigned long long bytesTotal;
};

struct InstallInfo {
  unsigned long long sizeOnDisk;
  string folder;
  unsigned timeStamp;
};

struct ItemDetailsInfo {
  unsigned playtimeStatsDays = 0;
  bool additionalPreviews = false;
  bool children = false;
  bool keyValueTags = false;
  bool longDescription = false;
  bool metadata = false;
  bool playtimeStats = false;
};

struct FindItemInfo {
  FindOrder order;
  string searchText;
  string tags;
  bool anyTag = false;

  optional<int> maxItemCount;
};

struct UpdateItemInfo {
  optional<ItemId> id;
  optional<string> title;
  optional<string> description;
  optional<string> folder;
  optional<string> previewFile;
  optional<ItemVisibility> visibility;
  optional<vector<string>> tags;
  optional<vector<pair<string, string>>> keyValues;
  optional<string> metadata;
};

struct UpdateItemResult {
  bool valid() const {
    return itemId && !error;
  }

  optional<ItemId> itemId;
  optional<string> error;
  bool requireLegalAgreement;
};

struct ItemInfo {
  ItemId id;
  CSteamID ownerId;
  ItemVisibility visibility;
  int votesUp, votesDown;
  float score;

  string title;
  string description;
  vector<string> tags;
  vector<pair<string, string>> keyValues;
  string metadata;

  // TODO: more flags
};

class UGC {
  STEAM_IFACE_DECL(UGC);

  int numSubscribedItems() const;

  vector<ItemId> subscribedItems() const;

  // TODO: return expected everywhere where something may fail ?
  // maybe just return optional?
  uint32_t state(ItemId) const;
  DownloadInfo downloadInfo(ItemId) const;
  InstallInfo installInfo(ItemId) const;

  using QueryId = int;
  static constexpr int maxItemsPerPage = 50;

  QueryId createDetailsQuery(const ItemDetailsInfo&, vector<ItemId>);
  QueryId createFindQuery(const FindItemInfo&, int pageId);

  void updateQueries();
  void waitForQueries(vector<QueryId>, int maxIters, milliseconds iterMsec = milliseconds(50));

  // TODO: how to report errors? use Expected<>
  bool isQueryValid(QueryId) const;
  QueryStatus queryStatus(QueryId) const;
  string queryError(QueryId, string pendingError = {}) const;

  // Results will only be returned if query is completed
  vector<ItemInfo> finishDetailsQuery(QueryId);
  vector<ItemId> finishFindQuery(QueryId);
  void finishQuery(QueryId);

  // Pass empty itemId to create new item
  void beginUpdateItem(const UpdateItemInfo&);
  optional<UpdateItemResult> tryUpdateItem();
  bool isUpdatingItem();

  // Will remove partially created item
  void cancelUpdateItem();
  void deleteItem(ItemId);

  private:
  using QHandle = UGCQueryHandle_t;

  struct QueryData;
  struct Impl;

  HeapAllocated<Impl> impl;
};
}
