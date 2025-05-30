/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef OHOS_RESTOOL_RESOURCE_DIRECTORY
#define OHOS_RESTOOL_RESOURCE_DIRECTORY

#include <functional>
#include <string>
#include <map>
#include "key_parser.h"

namespace OHOS {
namespace Global {
namespace Restool {
class ResourceDirectory {
public:
    ResourceDirectory() {};
    virtual ~ResourceDirectory() {};
    bool ScanResources(const std::string &resourcesDir, std::function<bool(const DirectoryInfo&)> callback) const;
private:
    bool ScanResourceLimitKeyDir(const std::string &resourceTypeDir, const std::string &limitKey,
        std::function<bool(const DirectoryInfo&)> callback) const;
};
}
}
}
#endif