/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_RESTOOL_RESOURCE_TABLE_H
#define OHOS_RESTOOL_RESOURCE_TABLE_H

#include <fstream>
#include <memory>
#include <sstream>
#include "resource_item.h"
#include "restool_errors.h"

namespace OHOS {
namespace Global {
namespace Restool {
class ResourceTable {
public:
    ResourceTable();
    virtual ~ResourceTable();
    uint32_t CreateResourceTable();
    uint32_t CreateResourceTable(const std::map<int64_t, std::vector<std::shared_ptr<ResourceItem>>> &items);
    static uint32_t LoadResTable(const std::string path, std::map<int64_t, std::vector<ResourceItem>> &resInfos);
    static uint32_t LoadResTable(std::basic_istream<char> &in, std::map<int64_t, std::vector<ResourceItem>> &resInfos);
private:
    struct TableData {
        uint32_t id;
        ResourceItem resourceItem;
    };

    struct IndexHeader {
        int8_t version[VERSION_MAX_LEN];
        uint32_t fileSize;
        uint32_t limitKeyConfigSize;
    };

    struct LimitKeyConfig {
        int8_t keyTag[TAG_LEN] = {'K', 'E', 'Y', 'S'};
        uint32_t offset; // IdSet file address offset
        uint32_t keyCount; // KeyParam count
        std::vector<int32_t> data;
    };

    struct IdSet {
        int8_t idTag[TAG_LEN] = {'I', 'D', 'S', 'S'};
        uint32_t idCount;
        std::map<uint32_t, uint32_t> data; // pair id and offset
    };

    struct RecordItem {
        uint32_t size;
        int32_t resType;
        uint32_t id;
    };
    uint32_t SaveToResouorceIndex(const std::map<std::string, std::vector<TableData>> &configs) const;
    uint32_t CreateIdDefined(const std::map<int64_t, std::vector<ResourceItem>> &allResource) const;
    bool InitIndexHeader(IndexHeader &indexHeader, uint32_t count) const;
    bool Prepare(const std::map<std::string, std::vector<TableData>> &configs,
                 std::map<std::string, LimitKeyConfig> &limitKeyConfigs,
                 std::map<std::string, IdSet> &idSets, uint32_t &pos) const;
    bool SaveRecordItem(const std::map<std::string, std::vector<TableData>> &configs, std::ostringstream &out,
                        std::map<std::string, IdSet> &idSets, uint32_t &pos) const;
    void SaveHeader(const IndexHeader &indexHeader, std::ostringstream &out) const;
    void SaveLimitKeyConfigs(const std::map<std::string, LimitKeyConfig> &limitKeyConfigs,
                             std::ostringstream &out) const;
    void SaveIdSets(const std::map<std::string, IdSet> &idSets, std::ostringstream &out) const;
    static bool ReadFileHeader(std::basic_istream<char> &in, IndexHeader &indexHeader, uint64_t &pos, uint64_t length);
    static bool ReadLimitKeys(std::basic_istream<char> &in, std::map<int64_t, std::vector<KeyParam>> &limitKeys,
                       uint32_t count, uint64_t &pos, uint64_t length);
    static bool ReadIdTables(std::basic_istream<char> &in, std::map<int64_t, std::pair<int64_t, int64_t>> &datas,
                      uint32_t count, uint64_t &pos, uint64_t length);
    static bool ReadDataRecordPrepare(std::basic_istream<char> &in, RecordItem &record, uint64_t &pos, uint64_t length);
    static bool ReadDataRecordStart(std::basic_istream<char> &in, RecordItem &record,
                             const std::map<int64_t, std::vector<KeyParam>> &limitKeys,
                             const std::map<int64_t, std::pair<int64_t, int64_t>> &datas,
                             std::map<int64_t, std::vector<ResourceItem>> &resInfos);
    std::string indexFilePath_;
    std::string idDefinedPath_;
};
}
}
}
#endif