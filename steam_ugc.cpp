#include "steam_internal.h"
#include "steam_ugc.h"
#include "steam_utils.h"
#include "steam_call_result.h"

#define FUNC(name, ...) SteamAPI_ISteamUGC_##name

namespace steam {

using QStatus = QueryStatus;
using QueryCall = CallResult<SteamUGCQueryCompleted_t>;

static const EnumMap<ItemVisibility, ERemoteStoragePublishedFileVisibility> itemVisibilityMap = {
    {ItemVisibility::public_, k_ERemoteStoragePublishedFileVisibilityPublic},
    {ItemVisibility::friends, k_ERemoteStoragePublishedFileVisibilityFriendsOnly},
    {ItemVisibility::private_, k_ERemoteStoragePublishedFileVisibilityPrivate}};

static const EnumMap<QueryOrder, EUGCQuery> queryOrderMap{
    {QueryOrder::votes, k_EUGCQuery_RankedByVote},
    {QueryOrder::date, k_EUGCQuery_RankedByPublicationDate},
    {QueryOrder::subscriptions, k_EUGCQuery_RankedByTotalUniqueSubscriptions},
    {QueryOrder::playtime, k_EUGCQuery_RankedByTotalPlaytime}};

struct UGC::QueryData {
  bool valid() const {
    return handle != k_UGCQueryHandleInvalid;
  }

  QHandle handle = k_UGCQueryHandleInvalid;
  optional<DetailsQueryInfo> detailsInfo;
  optional<FindQueryInfo> findInfo;
  QueryCall call;
};

struct UGC::Impl {
  QueryId allocQuery(QHandle handle, unsigned long long callId) {
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
    query.call = callId;
    return qid;
  }

  vector<QueryData> queries;
  optional<UpdateItemInfo> createItemInfo;
  CallResult<CreateItemResult_t> createItem;
  CallResult<SubmitItemUpdateResult_t> updateItem;
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

UGC::QueryId UGC::createDetailsQuery(const DetailsQueryInfo& info, vector<ItemId> items) {
  CHECK(items.size() >= 1 && items.size() < maxItemsPerPage);

  auto handle = FUNC(CreateQueryUGCDetailsRequest)(ptr, items.data(), items.size());
  CHECK(handle != k_UGCQueryHandleInvalid);
  // TODO: properly handle errors

#define SET_VAR(var, func)                                                                                             \
  if (info.var)                                                                                                        \
    FUNC(SetReturn##func)(ptr, handle, true);
  SET_VAR(additionalPreviews, AdditionalPreviews)
  SET_VAR(children, Children)
  SET_VAR(keyValueTags, KeyValueTags)
  SET_VAR(longDescription, LongDescription)
  SET_VAR(metadata, Metadata)
  SET_VAR(playtimeStats, PlaytimeStats)
#undef SET

  auto callId = FUNC(SendQueryUGCRequest)(ptr, handle);
  auto qid = impl->allocQuery(handle, callId);
  impl->queries[qid].detailsInfo = info;
  return qid;
}

UGC::QueryId UGC::createFindQuery(const FindQueryInfo& info, int pageId) {
  CHECK(pageId >= 1);
  auto appId = Utils::instance().appId();

  //auto match = k_EUGCMatchingUGCType_Items;
  auto match = k_EUGCMatchingUGCType_All;
  auto handle = FUNC(CreateQueryAllUGCRequest)(ptr, queryOrderMap[info.order], match, appId, appId, pageId);
  CHECK(handle != k_UGCQueryHandleInvalid);
  // TODO: properly handle errors

  FUNC(SetReturnOnlyIDs)(ptr, handle, true);
  if (!info.searchText.empty())
    FUNC(SetSearchText)(ptr, handle, info.searchText.c_str());

  auto callId = FUNC(SendQueryUGCRequest)(ptr, handle);
  auto qid = impl->allocQuery(handle, callId);
  impl->queries[qid].findInfo = info;
  return qid;
}

void UGC::updateQueries() {
  for (auto& query : impl->queries)
    if (query.call.status == QStatus::pending)
      query.call.update();
}

void UGC::waitForQueries(vector<QueryId> ids, int maxIters, milliseconds iterMsec) {
  auto allFinished = [&]() {
    for (auto qid : ids)
      if (queryStatus(qid) == QStatus::pending)
        return false;
    return true;
  };
  sleepUntil(allFinished, maxIters, iterMsec);
}

bool UGC::isQueryValid(QueryId qid) const {
  return impl->queries[qid].valid();
}

QueryStatus UGC::queryStatus(QueryId qid) const {
  CHECK(isQueryValid(qid));
  return impl->queries[qid].call.status;
}

string UGC::queryError(QueryId qid, string pendingError) const {
  CHECK(isQueryValid(qid));
  auto& call = impl->queries[qid].call;
  if (call.status == QStatus::failed)
    return call.failText();
  if (call.status == QStatus::pending)
    return pendingError;
  return "";
}

vector<ItemInfo> UGC::finishDetailsQuery(QueryId qid) {
  vector<ItemInfo> out;
  CHECK(isQueryValid(qid) && impl->queries[qid].detailsInfo);
  auto& query = impl->queries[qid];
  auto& info = impl->queries[qid].detailsInfo;

  if (query.call.status == QStatus::completed) {
    int numResults = (int)query.call.result().m_unNumResultsReturned;
    out.reserve(numResults);

    SteamUGCDetails_t details;
    for (int n = 0; n < numResults; n++) {
      auto ret = FUNC(GetQueryUGCResult)(ptr, query.handle, n, &details);
      CHECK(ret);
      ItemInfo newItem;
      newItem.id = details.m_nPublishedFileId;
      newItem.ownerId = details.m_ulSteamIDOwner;
      newItem.visibility = ItemVisibility::public_;
      for (auto vis : ENUM_ALL(ItemVisibility))
        if (itemVisibilityMap[vis] == details.m_eVisibility)
          newItem.visibility = vis;
      newItem.votesUp = details.m_unVotesUp;
      newItem.votesDown = details.m_unVotesDown;
      newItem.score = details.m_flScore;
      newItem.title = details.m_rgchTitle;
      newItem.description = details.m_rgchDescription;
      newItem.tags = split(details.m_rgchTags, {','});

      constexpr int bufSize = 4096;
      if (info->keyValueTags) {
        char buf1[bufSize + 1], buf2[bufSize + 1];
        int count = (int)FUNC(GetQueryUGCNumKeyValueTags)(ptr, query.handle, n);

        newItem.keyValues.resize(count);
        for (int i = 0; i < count; i++) {
          auto ret = FUNC(GetQueryUGCKeyValueTag)(ptr, query.handle, n, i, buf1, bufSize, buf2, bufSize);
          CHECK(ret);
          buf1[bufSize] = buf2[bufSize] = 0;
          newItem.keyValues[n] = make_pair(buf1, buf2);
        }
      }
      if (info->metadata) {
        char buffer[bufSize + 1];
        auto result = FUNC(GetQueryUGCMetadata)(ptr, query.handle, n, buffer, bufSize);
        buffer[bufSize] = 0;
        CHECK(result);
        newItem.metadata = buffer;
      }

      out.emplace_back(newItem);
    }
  }

  FUNC(ReleaseQueryUGCRequest)(ptr, query.handle);
  query.handle = k_UGCQueryHandleInvalid;
  return out;
}

vector<ItemId> UGC::finishFindQuery(QueryId qid) {
  vector<ItemId> out;
  CHECK(isQueryValid(qid) && impl->queries[qid].findInfo);
  auto& query = impl->queries[qid];

  if (query.call.status == QStatus::completed) {
    int numResults = (int)query.call.result().m_unNumResultsReturned;
    out.reserve(numResults);

    SteamUGCDetails_t details;
    for (int n = 0; n < numResults; n++) {
      auto ret = FUNC(GetQueryUGCResult)(ptr, query.handle, n, &details);
      CHECK(ret);
      out.emplace_back(details.m_nPublishedFileId);
    }
  }

  FUNC(ReleaseQueryUGCRequest)(ptr, query.handle);
  query.handle = k_UGCQueryHandleInvalid;
  return out;
}

void UGC::finishQuery(QueryId qid) {
  CHECK(isQueryValid(qid));
  auto& query = impl->queries[qid];
  FUNC(ReleaseQueryUGCRequest)(ptr, query.handle);
  query.handle = k_UGCQueryHandleInvalid;
}

void UGC::beginUpdateItem(const UpdateItemInfo& info) {
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
    if (info.previewFile)
      FUNC(SetItemPreview)(ptr, handle, info.previewFile->c_str());
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

    // TODO: metadata & key-value tags

    impl->updateItem = FUNC(SubmitItemUpdate)(ptr, handle, nullptr);
  } else {
    impl->createItem = FUNC(CreateItem)(ptr, appId, k_EWorkshopFileTypeCommunity);
    impl->createItemInfo = info;
  }
}

optional<UpdateItemResult> UGC::tryUpdateItem() {
  if (impl->createItem) {
    impl->createItem.update();
    if (impl->createItem.isCompleted()) {
      auto& out = impl->createItem.result();
      impl->createItem.clear();

      if (out.m_eResult == k_EResultOK) {
        impl->createItemInfo->id = out.m_nPublishedFileId;
        beginUpdateItem(*impl->createItemInfo);
        return none;
      } else {
        auto error = string("Error while creating item: ") + errorText(out.m_eResult);
        return UpdateItemResult{none, error, false};
      }
    }
    return none;
  }

  impl->updateItem.update();
  if (impl->updateItem.isCompleted()) {
    auto& out = impl->updateItem.result();
    impl->updateItem.clear();
    impl->createItemInfo = none;
    optional<string> error;
    if (out.m_eResult != k_EResultOK)
      error = string("Error while updating item: ") + errorText(out.m_eResult);
    return UpdateItemResult{out.m_nPublishedFileId, error, out.m_bUserNeedsToAcceptWorkshopLegalAgreement};
  }
  return none;
}

bool UGC::isUpdatingItem() {
  return !!impl->createItem || !!impl->updateItem;
}

void UGC::cancelUpdateItem() {
  auto& itemInfo = impl->createItemInfo;
  if (itemInfo && itemInfo->id && impl->updateItem)
    deleteItem(*itemInfo->id);
  impl->createItem.clear();
  itemInfo = none;
  impl->updateItem.clear();
}

void UGC::deleteItem(ItemId id) {
  FUNC(DeleteItem)(ptr, id);
}
}
