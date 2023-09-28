/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    All rights reserved.
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

#pragma once

#define TR_RELAY_REL1 14
#define TR_RELAY_REL2 12
#define TR_RELAY_REL3 4
#define TR_RELAY_REL1IN 17
// #define TR_RELAY_REL1IN 20
#define TR_RELAY_REL2IN 11
#define TR_RELAY_REL3IN 5
#define TR_RELAY_UP 1
#define TR_RELAY_DOWN 0

void TRRelayControl(uint8_t pinOut, uint8_t pinIn, uint8_t upDown);
