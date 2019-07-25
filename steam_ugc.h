#pragma once

#include "steam_base.h"
#include "steamworks/public/steam/isteamugc.h"

RICH_ENUM(SteamQueryOrder, votes, date, subscriptions, playtime);

namespace steam {

using QueryOrder = SteamQueryOrder;
using ItemId = PublishedFileId_t;

struct DownloadInfo {
  unsigned long long bytesDownloaded;
  unsigned long long bytesTotal;
};

struct InstallInfo {
  unsigned long long sizeOnDisk;
  string folder;
  unsigned timeStamp;
};

struct QueryInfo {
  string searchText;
  unsigned playtimeStatsDays = 0;
  // TODO: flags
  bool additionalPreviews = false;
  bool children = false;
  bool keyValueTags = false;
  bool longDescription = false;
  bool metadata = false;
  bool onlyIds = false;
  bool playtimeStats = false;
  bool totalOnly = false;
};

struct UpdateItemInfo {
  bool valid() const {
    return result == k_EResultOK;
  }

  ItemId itemId;
  EResult result;
  bool failedWhenCreating;
  bool requireLegalAgreement;
};

struct ItemInfo {
  optional<ItemId> id; // if empty, new item will be created
  optional<string> title;
  optional<string> description;
  optional<string> folder;
  optional<string> preview;
  optional<int> version;
  optional<ERemoteStoragePublishedFileVisibility> visibility;
};

struct QueryResults {
  int count, total;
};

using QueryDetails = SteamUGCDetails_t;

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

  QueryId createDetailsQuery(const QueryInfo&, vector<ItemId>);
  QueryId createQuery(const QueryInfo&, QueryOrder, int pageId);

  void updateQueries();
  void finishQuery(QueryId);

  // TODO: how to report errors?
  bool isQueryValid(QueryId) const;
  QueryStatus queryStatus(QueryId) const;
  const QueryInfo& queryInfo(QueryId) const;
  QueryResults queryResults(QueryId) const;
  string queryError(QueryId) const;
  void queryDetails(QueryId, int index, QueryDetails&) const;
  string queryMetadata(QueryId, int index);
  vector<pair<string, string>> queryKeyValueTags(QueryId, int index);
  vector<ItemId> queryIds(QueryId) const;

  void updateItem(const ItemInfo&);
  optional<UpdateItemInfo> tryUpdateItem();
  bool isUpdatingItem();
  void cancelUpdateItem();

  private:
  using QHandle = UGCQueryHandle_t;

  void setupQuery(QHandle, const QueryInfo&);

  struct QueryData;
  struct Impl;

  HeapAllocated<Impl> impl;
};
}
