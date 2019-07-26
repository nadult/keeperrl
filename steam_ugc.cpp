#include "steam_internal.h"
#include "steam_ugc.h"
#include "steam_utils.h"
#include "steam_call_result.h"

#define FUNC(name, ...) SteamAPI_ISteamUGC_##name

namespace steam {

using QStatus = QueryStatus;
using QueryCall = CallResult<SteamUGCQueryCompleted_t>;

struct UGC::QueryData {
  bool valid() const {
    return handle != k_UGCQueryHandleInvalid;
  }

  QHandle handle = k_UGCQueryHandleInvalid;
  QueryInfo info;
  QueryCall call;
};

struct UGC::Impl {
  QueryId allocQuery(QHandle handle, const QueryInfo& info, unsigned long long callId) {
    int qid = -1;
    for (int n = 0; n < queries.size(); n++)
      if (!queries[n].valid()) {
        qid = n;
        break;
      }
    if (qid == -1) {
      qid = queries.size();
      queries.emplace_back();
    }

    auto& query = queries[qid];
    query.handle = handle;
    query.info = info;
    query.call = callId;
    return qid;
  }

  vector<QueryData> queries;
  optional<ItemInfo> createItemInfo;
  CallResult<CreateItemResult_t> createItem;
  CallResult<SubmitItemUpdateResult_t> updateItem;
  QueryDetails queryDetails;
};

UGC::UGC(intptr_t ptr) : ptr(ptr) {
  static_assert(maxItemsPerPage <= (int)kNumUGCResultsPerPage, "");
}
UGC::~UGC() = default;

int UGC::numSubscribedItems() const {
  return (int)FUNC(GetNumSubscribedItems)(ptr);
}

vector<ItemId> UGC::subscribedItems() const {
  vector<ItemId> out(numSubscribedItems());
  INFO << "STEAM: num: " << out.size();
  int result = FUNC(GetSubscribedItems)(ptr, out.data(), out.size());
  out.resize(result);
  return out;
}

uint32_t UGC::state(ItemId id) const {
  return FUNC(GetItemState)(ptr, id);
}

DownloadInfo UGC::downloadInfo(ItemId id) const {
  uint64 downloaded = 0, total = 0;
  auto result = FUNC(GetItemDownloadInfo)(ptr, id, &downloaded, &total);
  // TODO: handle result
  return {downloaded, total};
}

InstallInfo UGC::installInfo(ItemId id) const {
  uint64 size_on_disk;
  uint32 time_stamp;
  char buffer[4096];
  auto result = FUNC(GetItemInstallInfo)(ptr, id, &size_on_disk, buffer, sizeof(buffer) - 1, &time_stamp);
  buffer[sizeof(buffer) - 1] = 0;
  return {size_on_disk, buffer, time_stamp};
}

void UGC::setupQuery(QHandle handle, const QueryInfo& info) {
#define SET_VAR(var, func)                                                                                             \
  if (info.var)                                                                                                        \
    FUNC(SetReturn##func)(ptr, handle, true);
  SET_VAR(additionalPreviews, AdditionalPreviews)
  SET_VAR(children, Children)
  SET_VAR(keyValueTags, KeyValueTags)
  SET_VAR(longDescription, LongDescription)
  SET_VAR(metadata, Metadata)
  SET_VAR(onlyIds, OnlyIDs)
  SET_VAR(playtimeStats, PlaytimeStats)
  SET_VAR(totalOnly, TotalOnly)
#undef SET
  if (!info.searchText.empty())
    FUNC(SetSearchText)(ptr, handle, info.searchText.c_str());
}

UGC::QueryId UGC::createDetailsQuery(const QueryInfo& info, vector<ItemId> items) {
  CHECK(items.size() >= 1 && items.size() < maxItemsPerPage);

  auto handle = FUNC(CreateQueryUGCDetailsRequest)(ptr, items.data(), items.size());
  CHECK(handle != k_UGCQueryHandleInvalid);
  // TODO: properly handle errors

  setupQuery(handle, info);
  auto callId = FUNC(SendQueryUGCRequest)(ptr, handle);
  return impl->allocQuery(handle, info, callId);
}

static const EnumMap<QueryOrder, EUGCQuery> queryOrderMap{
    {QueryOrder::votes, k_EUGCQuery_RankedByVote},
    {QueryOrder::date, k_EUGCQuery_RankedByPublicationDate},
    {QueryOrder::subscriptions, k_EUGCQuery_RankedByTotalUniqueSubscriptions},
    {QueryOrder::playtime, k_EUGCQuery_RankedByTotalPlaytime}};

UGC::QueryId UGC::createQuery(const QueryInfo& info, QueryOrder order, int pageId) {
  CHECK(pageId >= 1);
  auto appId = Utils::instance().appId();

  //auto match = k_EUGCMatchingUGCType_Items;
  auto match = k_EUGCMatchingUGCType_All;
  auto handle = FUNC(CreateQueryAllUGCRequest)(ptr, queryOrderMap[order], match, appId, appId, pageId);
  CHECK(handle != k_UGCQueryHandleInvalid);
  // TODO: properly handle errors

  setupQuery(handle, info);
  auto callId = FUNC(SendQueryUGCRequest)(ptr, handle);
  return impl->allocQuery(handle, info, callId);
}

void UGC::updateQueries() {
  for (auto& query : impl->queries)
    if (query.call.status == QStatus::pending)
      query.call.update();
}

void UGC::finishQuery(QueryId qid) {
  CHECK(isQueryValid(qid));
  auto& query = impl->queries[qid];
  FUNC(ReleaseQueryUGCRequest)(ptr, query.handle);
  query.handle = k_UGCQueryHandleInvalid;
}

bool UGC::isQueryValid(QueryId qid) const {
  return impl->queries[qid].valid();
}

QueryStatus UGC::queryStatus(QueryId qid) const {
  CHECK(isQueryValid(qid));
  return impl->queries[qid].call.status;
}

const QueryInfo& UGC::queryInfo(QueryId qid) const {
  CHECK(isQueryValid(qid));
  return impl->queries[qid].info;
}

QueryResults UGC::queryResults(QueryId qid) const {
  CHECK(queryStatus(qid) == QStatus::completed);
  auto& result = impl->queries[qid].call.result();
  return {(int)result.m_unNumResultsReturned, (int)result.m_unTotalMatchingResults};
}

vector<ItemId> UGC::queryIds(QueryId qid) {
  vector<ItemId> out;
  auto results = queryResults(qid);
  INFO << "STEAM: results: " << results.count << " " << results.total;
  out.reserve(results.count);
  for (int n = 0; n < results.count; n++)
    out.emplace_back(queryDetails(qid, n).m_nPublishedFileId);
  return out;
}

string UGC::queryError(QueryId qid) const {
  CHECK(isQueryValid(qid));
  auto& call = impl->queries[qid].call;
  if (call.status == QStatus::failed)
    return call.failText();
  return "";
}

const QueryDetails& UGC::queryDetails(QueryId qid, int index) {
  CHECK(queryStatus(qid) == QStatus::completed);
  auto& query = impl->queries[qid];

  auto result = FUNC(GetQueryUGCResult)(ptr, query.handle, index, &impl->queryDetails);
  CHECK(result);
  // TODO: properly handle errors
  return impl->queryDetails;
}

string UGC::queryMetadata(QueryId qid, int index) {
  CHECK(queryStatus(qid) == QStatus::completed);
  auto& query = impl->queries[qid];
  CHECK(query.info.metadata);

  char buffer[4096];
  auto result = FUNC(GetQueryUGCMetadata)(ptr, query.handle, index, buffer, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = 0;
  CHECK(result);
  return buffer;
}

vector<pair<string, string>> UGC::queryKeyValueTags(QueryId qid, int index) {
  vector<pair<string, string>> out;

  CHECK(queryStatus(qid) == QStatus::completed);
  auto& query = impl->queries[qid];
  CHECK(query.info.keyValueTags);

  char buf1[4096], buf2[4096];
  auto count = FUNC(GetQueryUGCNumKeyValueTags)(ptr, query.handle, index);

  out.resize(count);
  for (unsigned n = 0; n < count; n++) {
    auto result =
        FUNC(GetQueryUGCKeyValueTag)(ptr, query.handle, index, n, buf1, sizeof(buf1) - 1, buf2, sizeof(buf2) - 1);
    CHECK(result);
    buf1[sizeof(buf1) - 1] = 0;
    buf2[sizeof(buf2) - 1] = 0;
    out[n] = make_pair(buf1, buf2);
  }
  return out;
}

static const EnumMap<ItemVisibility, ERemoteStoragePublishedFileVisibility> itemVisibilityMap = {
    {ItemVisibility::public_, k_ERemoteStoragePublishedFileVisibilityPublic},
    {ItemVisibility::friends, k_ERemoteStoragePublishedFileVisibilityFriendsOnly},
    {ItemVisibility::private_, k_ERemoteStoragePublishedFileVisibilityPrivate}};

void UGC::updateItem(const ItemInfo& info) {
  CHECK(!isUpdatingItem());
  auto appId = Utils::instance().appId();

  if (info.id) {
    auto handle = FUNC(StartItemUpdate)(ptr, appId, *info.id);

    if (info.title)
      FUNC(SetItemTitle)(ptr, handle, info.title->c_str());
    if (info.description)
      FUNC(SetItemDescription)(ptr, handle, info.description->c_str());
    if (info.folder)
      FUNC(SetItemContent)(ptr, handle, info.folder->c_str());
    if (info.preview)
      FUNC(SetItemPreview)(ptr, handle, info.preview->c_str());
    if (info.visibility)
      FUNC(SetItemVisibility)(ptr, handle, itemVisibilityMap[*info.visibility]);
    if (info.tags) {
      vector<const char*> buffer(info.tags->size());
      for (auto& tag : *info.tags)
        buffer.emplace_back(tag.c_str());

      SteamParamStringArray_t strings;
      strings.m_nNumStrings = info.tags->size();
      strings.m_ppStrings = buffer.data();
      FUNC(SetItemTags)(ptr, handle, &strings);
    }

    impl->updateItem = FUNC(SubmitItemUpdate)(ptr, handle, nullptr);
  } else {
    impl->createItem = FUNC(CreateItem)(ptr, appId, k_EWorkshopFileTypeCommunity);
    impl->createItemInfo = info;
  }
}

optional<UpdateItemInfo> UGC::tryUpdateItem() {
  if (impl->createItem) {
    impl->createItem.update();
    if (impl->createItem.isCompleted()) {
      auto& out = impl->createItem.result();
      impl->createItem.clear();

      if (out.m_eResult == k_EResultOK) {
        impl->createItemInfo->id = out.m_nPublishedFileId;
        updateItem(*impl->createItemInfo);
        return none;
      } else {
        return UpdateItemInfo{k_PublishedFileIdInvalid, out.m_eResult, true, false};
      }
    }
    return none;
  }

  impl->updateItem.update();
  if (impl->updateItem.isCompleted()) {
    auto& out = impl->updateItem.result();
    impl->updateItem.clear();
    impl->createItemInfo = none;
    return UpdateItemInfo{out.m_nPublishedFileId, out.m_eResult, false, out.m_bUserNeedsToAcceptWorkshopLegalAgreement};
  }
  return none;
}

bool UGC::isUpdatingItem() {
  return !!impl->createItem || !!impl->updateItem;
}

void UGC::cancelUpdateItem() {
  auto& itemInfo = impl->createItemInfo;
  if (itemInfo && itemInfo->id && impl->updateItem) {
    // Update was cancelled, let's remove partially updated item
    FUNC(DeleteItem)(ptr, *itemInfo->id);
  }

  impl->createItem.clear();
  itemInfo = none;
  impl->updateItem.clear();
}
}
