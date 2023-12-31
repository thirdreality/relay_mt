/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <app-common/zap-generated/cluster-objects.h>
#include <app/util/af-enums.h>

typedef void (*CHIPDefaultSuccessCallbackType)(void *, const chip::app::DataModel::NullObjectType &);
typedef void (*CHIPDefaultWriteSuccessCallbackType)(void *);
typedef void (*CHIPDefaultFailureCallbackType)(void *, CHIP_ERROR);


typedef void (*CHIPFirstClusterSomeIntegerAttributeCallbackType)(void *, chip::app::Clusters::First::Attributes::SomeInteger::TypeInfo::DecodableArgType);
typedef void (*CHIPSecondClusterFabricsAttributeCallbackType)(void *, const chip::app::Clusters::Second::Attributes::Fabrics::TypeInfo::DecodableType &);
typedef void (*CHIPSecondClusterSomeBytesAttributeCallbackType)(void *, chip::app::Clusters::Second::Attributes::SomeBytes::TypeInfo::DecodableArgType);
typedef void (*CHIPThirdClusterSomeEnumAttributeCallbackType)(void *, chip::app::Clusters::Third::Attributes::SomeEnum::TypeInfo::DecodableArgType);
typedef void (*CHIPThirdClusterOptionsAttributeCallbackType)(void *, chip::app::Clusters::Third::Attributes::Options::TypeInfo::DecodableArgType);

