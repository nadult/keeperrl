#pragma once

#include "steam_base.h"
#include "steamworks/public/steam/isteamugc.h"

RICH_ENUM(SteamQueryOrder, votes, date, subscriptions, playtime);
RICH_ENUM(SteamItemVisibility, public_, friends, private_);

namespace steam {

using QueryOrder = SteamQueryOrder;
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

struct DetailsQueryInfo {
  unsigned playtimeStatsDays = 0;
  bool additionalPreviews = false;
  bool children = false;
  bool keyValueTags = false;
  bool longDescription = false;
  bool metadata = false;
  bool playtimeStats = false;
};

struct FindQueryInfo {
  QueryOrder order;
  string searchText;
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
    return result == k_EResultOK;
  }

  ItemId itemId;
  EResult result;
  bool failedWhenCreating;
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

  QueryId createDetailsQuery(const DetailsQueryInfo&, vector<ItemId>);
  QueryId createFindQuery(const FindQueryInfo&, int pageId);

  void updateQueries();
  void waitForQueries(vector<QueryId>, int maxIters, int iterMsec = 50);

  // TODO: how to report errors? use Expected<>
  bool isQueryValid(QueryId) const;
  QueryStatus queryStatus(QueryId) const;
  string queryError(QueryId, string pendingError = {}) const;

  // Results will only be returned if query is completed
  vector<ItemInfo> finishDetailsQuery(QueryId);
  vector<ItemId> finishFindQuery(QueryId);
  void finishQuery(QueryId);

  // Pass empty itemId to create new item
  void updateItem(const UpdateItemInfo&);
  optional<UpdateItemResult> tryUpdateItem();
  bool isUpdatingItem();
  void cancelUpdateItem();

  private:
  using QHandle = UGCQueryHandle_t;

  struct QueryData;
  struct Impl;

  HeapAllocated<Impl> impl;
};
}
