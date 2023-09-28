/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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

/**
 * @file
 *   This file implements the handler for data model messages.
 */

#include <plat.h>
#include <ZclCallbacks.h>
#include <AppTask.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>
#include <lib/support/logging/CHIPLogging.h>

extern "C" {
#include "board.h"
#include <bl_gpio.h>
#include <easyflash.h>
#include <hal_gpio.h>
#include <hosal_gpio.h>
#include <demo_pwm.h>
}

using namespace ::chip;
using namespace ::chip::app::Clusters;

void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath & attributePath, uint8_t type, uint16_t size,
                                       uint8_t * value)
{
    uint8_t u8cfg = 255;
    ClusterId clusterId     = attributePath.mClusterId;
    AttributeId attributeId = attributePath.mAttributeId;
    int endpoint = (int)attributePath.mEndpointId;
    ChipLogProgress(Zcl, "Cluster callback: " ChipLogFormatMEI, ChipLogValueMEI(clusterId));
    ChipLogProgress(Zcl, "Cluster callback: mEndpointId=%u" , (int)attributePath.mEndpointId);

    if (clusterId == OnOff::Id && attributeId == OnOff::Attributes::OnOff::Id)
    {
        GetAppTask().PostEvent(AppTask::APP_EVENT_LIGHTING_ONOFF);
        ChipLogProgress(Zcl, "OnOff attribute ID: " ChipLogFormatMEI " Type: %u Value: %u, length %u", ChipLogValueMEI(attributeId),
                type, *value, size);
        GetAppTask().GetU8config(u8cfg);
        ChipLogProgress(Zcl, "u8Config : %u", u8cfg);

        if (*value > 0)
        {
            if (endpoint==1)
            {
                TRRelayControl(TR_RELAY_REL1, TR_RELAY_REL1IN, TR_RELAY_UP);
                GetAppTask().SetLedState(endpoint, true);
                set_color(6, 84, 254);     // led on (green color)
            }
            else if (endpoint==2)
            {
                if (u8cfg==2 || u8cfg==4)
                {
                    TRRelayControl(TR_RELAY_REL2, TR_RELAY_REL2IN, TR_RELAY_UP);
                    GetAppTask().SetLedState(endpoint, true);
                    set_color(6, 84, 254);
                }
                else if (u8cfg==3)
                {
                    TRRelayControl(TR_RELAY_REL3, TR_RELAY_REL3IN, TR_RELAY_UP);
                    GetAppTask().SetLedState(endpoint, true);
                    set_color(6, 84, 254);
                }
            }
            else if (endpoint==3 && u8cfg==4)
            {
                TRRelayControl(TR_RELAY_REL3, TR_RELAY_REL3IN, TR_RELAY_UP);
                GetAppTask().SetLedState(endpoint, true);
                set_color(6, 84, 254);
            }
        }
        else
        {
            if (endpoint==1)
            {
                TRRelayControl(TR_RELAY_REL1, TR_RELAY_REL1IN, TR_RELAY_DOWN);
                if (GetAppTask().SetLedState(endpoint, false)==0)
                    set_color(0, 0, 0);    // led off
            }
            else if (endpoint==2)
            {
                if (u8cfg==2 || u8cfg==4)
                {
                    TRRelayControl(TR_RELAY_REL2, TR_RELAY_REL2IN, TR_RELAY_DOWN);
                    if (GetAppTask().SetLedState(endpoint, false)==0)
                        set_color(0, 0, 0);
                }
                else if (u8cfg==3)
                {
                    TRRelayControl(TR_RELAY_REL3, TR_RELAY_REL3IN, TR_RELAY_DOWN);
                    if (GetAppTask().SetLedState(endpoint, false)==0)
                        set_color(0, 0, 0);
                }
            }
            else if (endpoint==3 && u8cfg==4)
            {
                TRRelayControl(TR_RELAY_REL3, TR_RELAY_REL3IN, TR_RELAY_DOWN);
                if (GetAppTask().SetLedState(endpoint, false)==0)
                    set_color(0, 0, 0);
            }
        }
    }

    /*
    else if (clusterId == LevelControl::Id)
    {
        GetAppTask().PostEvent(AppTask::APP_EVENT_LIGHTING_LEVEL);
        ChipLogProgress(Zcl, "Level Control attribute ID: " ChipLogFormatMEI " Type: %u Value: %u, length %u",
                        ChipLogValueMEI(attributeId), type, *value, size);
    }
    else if (clusterId == ColorControl::Id)
    {
        GetAppTask().PostEvent(AppTask::APP_EVENT_LIGHTING_COLOR);
        ChipLogProgress(Zcl, "Color Control attribute ID: " ChipLogFormatMEI " Type: %u Value: %u, length %u",
                        ChipLogValueMEI(attributeId), type, *value, size);
    }
    else if (clusterId == OnOffSwitchConfiguration::Id)
    {
        GetAppTask().PostEvent(AppTask::APP_EVENT_LIGHTING_ONOFF);
        ChipLogProgress(Zcl, "OnOff Switch Configuration attribute ID: " ChipLogFormatMEI " Type: %u Value: %u, length %u",
                        ChipLogValueMEI(attributeId), type, *value, size);
    }
    */
    else if (clusterId == Identify::Id)
    {
        GetAppTask().PostEvent(AppTask::APP_EVENT_IDENTIFY_IDENTIFY);
        ChipLogProgress(Zcl, "Identify attribute ID: " ChipLogFormatMEI " Type: %u Value: %u, length %u",
                        ChipLogValueMEI(attributeId), type, *value, size);
    }
}

/** @brief OnOff Cluster Init
 *
 * This function is called when a specific cluster is initialized. It gives the
 * application an opportunity to take care of cluster initialization procedures.
 * It is called exactly once for each endpoint where cluster is present.
 *
 * @param endpoint   Ver.: always
 *
 * TODO Issue #3841
 * emberAfOnOffClusterInitCallback happens before the stack initialize the cluster
 * attributes to the default value.
 * The logic here expects something similar to the deprecated Plugins callback
 * emberAfPluginOnOffClusterServerPostInitCallback.
 *
 */
void emberAfOnOffClusterInitCallback(EndpointId endpoint)
{
    // TODO: implement any additional Cluster Server init actions
}

void emberAfColorControlClusterInitCallback(EndpointId endpoint)
{
    GetAppTask().SetEndpointId(endpoint);
}

void TRRelayControl(uint8_t pinOut, uint8_t pinIn, uint8_t upDown)
{
    uint8_t gpioValue = 0xFF;
    bl_gpio_enable_output(pinOut, upDown, !upDown);
    bl_gpio_output_set(pinOut, upDown);
    bl_gpio_enable_input(pinIn, !upDown, upDown);
    bl_gpio_input_get(pinIn, &gpioValue);
    if (upDown)
    {
        if (!gpioValue)
        {
            bl_gpio_output_set(pinOut, upDown);
            bl_gpio_input_get(pinIn, &gpioValue);
            if (!gpioValue)
                ChipLogProgress(Zcl, "abnormal gpio pin : %u _ value : %u", pinIn, gpioValue);
        }
    }
    else if (!upDown)
    {
        if (gpioValue)
        {
            bl_gpio_output_set(pinOut, upDown);
            bl_gpio_input_get(pinIn, &gpioValue);
            if (gpioValue)
                ChipLogProgress(Zcl, "abnormal gpio pin : %u _ value : %u", pinIn, gpioValue);
        }
    }
}
