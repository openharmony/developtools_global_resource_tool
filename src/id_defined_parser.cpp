/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "id_defined_parser.h"
#include "file_entry.h"
#include "resource_util.h"

namespace OHOS {
namespace Global {
namespace Restool {
using namespace std;
const int32_t IdDefinedParser::START_SYS_ID = 0x07800000;
IdDefinedParser::IdDefinedParser(const PackageParser &packageParser, const ResourceIdCluster &type)
    : packageParser_(packageParser), type_(type), root_(nullptr)
{
}

IdDefinedParser::~IdDefinedParser()
{
    if (root_) {
        cJSON_Delete(root_);
    }
}

uint32_t IdDefinedParser::Init()
{
    InitParser();
    string idDefinedInput = packageParser_.GetIdDefinedInputPath();
    int32_t startId = packageParser_.GetStartId();
    bool combine = packageParser_.GetCombine();
    bool isSys = type_ == ResourceIdCluster::RES_ID_SYS;
    for (const auto &inputPath : packageParser_.GetInputs()) {
        string idDefinedPath;
        if (combine) {
            idDefinedPath = FileEntry::FilePath(inputPath).Append(ID_DEFINED_FILE).GetPath();
        } else {
            idDefinedPath = ResourceUtil::GetBaseElementPath(inputPath).Append(ID_DEFINED_FILE).GetPath();
        }
        if (ResourceUtil::FileExist(idDefinedPath) && startId > 0) {
            cerr << "Error: the set start_id and id_defined.json cannot be used together." << endl;
            return RESTOOL_ERROR;
        }
        if (Init(idDefinedPath, isSys) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    }
    if (isSys) {
        return RESTOOL_SUCCESS;
    }
    if (!idDefinedInput.empty()) {
        appDefinedIds_.clear();
        string idDefinedPath = FileEntry::FilePath(idDefinedInput).GetPath();
        if (Init(idDefinedPath, false) != RESTOOL_SUCCESS) {
            return RESTOOL_ERROR;
        }
    }
    string sysIdDefinedPath = FileEntry::FilePath(packageParser_.GetRestoolPath())
        .GetParent().Append(ID_DEFINED_FILE).GetPath();
    if (Init(sysIdDefinedPath, true) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

uint32_t IdDefinedParser::Init(const string &filePath, bool isSystem)
{
    if (!ResourceUtil::FileExist(filePath)) {
        return RESTOOL_SUCCESS;
    }

    if (!ResourceUtil::OpenJsonFile(filePath, &root_)) {
        return RESTOOL_ERROR;
    }

    if (!root_ || !cJSON_IsObject(root_)) {
        cerr << "Error: JSON file parsing failed, please check the JSON file.";
        cerr << NEW_LINE_PATH << filePath << endl;
        return RESTOOL_ERROR;
    }

    cJSON *recordNode = cJSON_GetObjectItem(root_, "record");
    if (!recordNode || !cJSON_IsArray(recordNode)) {
        cerr << "Error: 'record' node is not an array, please check the JSON file.";
        cerr << NEW_LINE_PATH << filePath << endl;
        return RESTOOL_ERROR;
    }
    if (cJSON_GetArraySize(recordNode) == 0) {
        cerr << "Error: 'record' node is empty, please check the JSON file.";
        cerr << NEW_LINE_PATH << filePath << endl;
        return RESTOOL_ERROR;
    }
    int32_t startSysId = 0;
    if (isSystem) {
        startSysId = GetStartId();
        if (startSysId < 0) {
            return RESTOOL_ERROR;
        }
    }

    if (IdDefinedToResourceIds(recordNode, isSystem, startSysId) != RESTOOL_SUCCESS) {
        return RESTOOL_ERROR;
    }
    return RESTOOL_SUCCESS;
}

void IdDefinedParser::InitParser()
{
    using namespace placeholders;
    handles_.emplace("type", bind(&IdDefinedParser::ParseType, this, _1, _2));
    handles_.emplace("name", bind(&IdDefinedParser::ParseName, this, _1, _2));
    handles_.emplace("id", bind(&IdDefinedParser::ParseId, this, _1, _2));
    handles_.emplace("order", bind(&IdDefinedParser::ParseOrder, this, _1, _2));
}

uint32_t IdDefinedParser::IdDefinedToResourceIds(const cJSON *record, bool isSystem, const int32_t startSysId)
{
    int32_t index = -1;
    for (cJSON *item = record->child; item; item = item->next) {
        index++;
        if (!cJSON_IsObject(item)) {
            return RESTOOL_ERROR;
        }
        ResourceId resourceId;
        resourceId.seq = index;
        resourceId.id = startSysId;
        for (const auto &handle : handles_) {
            if ((handle.first == "id" && isSystem) || (handle.first == "order" && !isSystem)) {
                continue;
            }
            if (!handle.second(cJSON_GetObjectItem(item, handle.first.c_str()), resourceId)) {
                return RESTOOL_ERROR;
            }
        }
        if (!PushResourceId(resourceId, isSystem)) {
            return RESTOOL_ERROR;
        }
    }
    return RESTOOL_SUCCESS;
}

bool IdDefinedParser::PushResourceId(const ResourceId &resourceId, bool isSystem)
{
    ResType resType = ResourceUtil::GetResTypeFromString(resourceId.type);
    auto ret = idDefineds_.emplace(resourceId.id, resourceId);
    if (!ret.second) {
        cerr << "Error: '" << ret.first->second.name << "' and '" << resourceId.name << "' defind the same ID." << endl;
        return false;
    }
    if (isSystem) {
        auto ret1 = sysDefinedIds_.emplace(make_pair(resType, resourceId.name), resourceId);
        if (!ret1.second) {
            cerr << "Error: the same type of '" << resourceId.name << "' exists in the id_defined.json. " << endl;
            return false;
        }
    } else {
        auto ret2 = appDefinedIds_.emplace(make_pair(resType, resourceId.name), resourceId);
        if (!ret2.second) {
            cerr << "Error: the same type of '" << resourceId.name << "' exists in the id_defined.json. " << endl;
            return false;
        }
    }
    return true;
}

bool IdDefinedParser::ParseId(const cJSON *origId, ResourceId &resourceId)
{
    if (!origId) {
        cerr << "Error: id_defined.json seq =" << resourceId.seq << " id empty." << endl;
        return false;
    }
    if (!cJSON_IsString(origId)) {
        cerr << "Error: id_defined.json seq =" << resourceId.seq << " id not string." << endl;
        return false;
    }
    string idStr = origId->valuestring;
    if (!ResourceUtil::CheckHexStr(idStr)) {
        cerr << "Error: id_defined.json seq =" << resourceId.seq;
        cerr << " id must be a hex string, eg:^0[xX][0-9a-fA-F]{8}" << endl;
        return false;
    }
    int32_t id = strtol(idStr.c_str(), nullptr, 16);
    if (id < 0x01000000 || (id >= 0x06FFFFFF && id < 0x08000000) || id >= 0x41FFFFFF) {
        cerr << "Error: id_defined.json seq = "<< resourceId.seq;
        cerr << " id must in [0x01000000,0x06FFFFFF),[0x08000000,0x41FFFFFF)." << endl;
        return false;
    }
    resourceId.id = id;
    return true;
}

bool IdDefinedParser::ParseType(const cJSON *type, ResourceId &resourceId)
{
    if (!type) {
        cerr << "Error: id_defined.json seq =" << resourceId.seq << " type empty." << endl;
        return false;
    }
    if (!cJSON_IsString(type)) {
        cerr << "Error: id_defined.json seq =" << resourceId.seq << " type not string." << endl;
        return false;
    }
    if (ResourceUtil::GetResTypeFromString(type->valuestring) == ResType::INVALID_RES_TYPE) {
        cerr << "Error: id_defined.json seq =" << resourceId.seq << " type '";
        cerr << type->valuestring << "' invalid." << endl;
        return false;
    }
    resourceId.type = type->valuestring;
    return true;
}

bool IdDefinedParser::ParseName(const cJSON *name, ResourceId &resourceId)
{
    if (!name) {
        cerr << "Error: id_defined.json seq =" << resourceId.seq << " name empty." << endl;
        return false;
    }
    if (!cJSON_IsString(name)) {
        cerr << "Error: id_defined.json seq =" << resourceId.seq << " name not string." << endl;
        return false;
    }
    resourceId.name = name->valuestring;
    if (type_ == ResourceIdCluster::RES_ID_SYS &&
        (resourceId.id & START_SYS_ID) == START_SYS_ID && !ResourceUtil::IsValidName(resourceId.name)) {
        cerr << "Error: id_defined.json."<< endl;
        return false;
    }
    return true;
}

bool IdDefinedParser::ParseOrder(const cJSON *order, ResourceId &resourceId)
{
    if (!order) {
        cerr << "Error: id_defined.json seq =" << resourceId.seq << " order empty." << endl;
        return false;
    }
    if (!ResourceUtil::IsIntValue(order)) {
        cerr << "Error: id_defined.json seq =" << resourceId.seq << " order not int." << endl;
        return false;
    }
    int32_t orderId = order->valueint;
    if (orderId != resourceId.seq) {
        cerr << "Error: id_defined.json seq =" << resourceId.seq << " order value ";
        cerr << orderId << " vs expect " << resourceId.seq << endl;
        return false;
    }
    resourceId.id = resourceId.id + orderId;
    return true;
}

int32_t IdDefinedParser::GetStartId() const
{
    cJSON *startIdNode = cJSON_GetObjectItem(root_, "startId");
    if (!startIdNode) {
        cerr << "Error: id_defined.json 'startId' empty." << endl;
        return -1;
    }

    if (!cJSON_IsString(startIdNode)) {
        cerr << "Error: id_defined.json 'startId' not string." << endl;
        return -1;
    }

    int32_t id = strtol(startIdNode->valuestring, nullptr, 16);
    return id;
}

}
}
}