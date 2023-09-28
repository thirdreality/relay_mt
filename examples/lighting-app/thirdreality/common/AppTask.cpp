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

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/util/attribute-storage.h>

#include <app/server/Dnssd.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <system/SystemClock.h>

#if HEAP_MONITORING
#include "MemMonitoring.h"
#endif

#if CHIP_ENABLE_OPENTHREAD
#include <platform/OpenThread/OpenThreadUtils.h>
#include <platform/ThreadStackManager.h>
#include <platform/bouffalolab/common/ThreadStackManagerImpl.h>
#include <utils_list.h>
#endif

#if CONFIG_ENABLE_CHIP_SHELL
#include <ChipShellCollection.h>
#include <lib/shell/Engine.h>
#endif

#include <LEDWidget.h>
#include <plat.h>

extern "C" {
#include "board.h"
#include "bl_wifi.h"
#include <bl_gpio.h>
#include <easyflash.h>
#include <hal_gpio.h>
#include <hosal_gpio.h>
#include <demo_pwm.h>
}

#include "AppTask.h"

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;

#if CONFIG_ENABLE_CHIP_SHELL
using namespace chip::Shell;
#endif

namespace {

#if defined(BL706_NIGHT_LIGHT) || defined(BL602_NIGHT_LIGHT)
ColorLEDWidget sLightLED;
#else
DimmableLEDWidget sLightLED;
#endif

Identify sIdentify = {
    APP_LIGHT_ENDPOINT_ID,
    AppTask::IdentifyStartHandler,
    AppTask::IdentifyStopHandler,
    Clusters::Identify::IdentifyTypeEnum::kLightOutput,
};

} // namespace

AppTask AppTask::sAppTask;
StackType_t AppTask::appStack[APP_TASK_STACK_SIZE / sizeof(StackType_t)];
StaticTask_t AppTask::appTaskStruct;

uint16_t discriminator_mac_g;
char sn_mac_g[24];

extern "C" void get_mac_init(void)
{
    uint8_t mac[6];
    uint32_t setup_pin_code_mac_g;

    memset(mac, 0, sizeof(mac));
#ifdef BL602_ENABLE
    bl_wifi_mac_addr_get(mac);
#elif defined(BL702_ENABLE)
    bl_wireless_mac_addr_get(mac);
#endif
    // ChipLogProgress(NotSpecified, " get MAC #### %02X:%02X:%02X:%02X:%02X:%02X ####\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    char pre_sn[] = "3RM02-3354-"; // 3RM02-3354-1962c // 3RM + 02(Relay) + 3(Year.2023) + 35(Week.35th) + 4(Thursday.4) + 1962c(mac)
    strncpy(sn_mac_g, pre_sn, 12);
    itoa(mac[5], sn_mac_g+11, 16);
    itoa(mac[4], sn_mac_g+13, 16);
    itoa(mac[3], sn_mac_g+15, 16);
    itoa(mac[2], sn_mac_g+17, 16);
    itoa(mac[1], sn_mac_g+19, 16);
    itoa(mac[0], sn_mac_g+21, 16);
    sn_mac_g[16] = '\0';

    setup_pin_code_mac_g = (((mac[0] << 8) + mac[1]) ^ ((mac[1] << 8) + mac[2])) & 0x7FFF;
    setup_pin_code_mac_g <<= 12;
    setup_pin_code_mac_g += (mac[3] << 4) + ((mac[4] & 0xF0) >> 4) + 1;
    discriminator_mac_g = ((mac[4] & 0x0F) << 8) + mac[5] + 1;

    *((uint8_t*)&discriminator_mac_g+1) ^= *(uint8_t*)&discriminator_mac_g;
    *((uint8_t*)&setup_pin_code_mac_g+1) ^= *(uint8_t*)&setup_pin_code_mac_g;
    *((uint8_t*)&setup_pin_code_mac_g+2) ^= *(uint8_t*)&setup_pin_code_mac_g;
    *((uint8_t*)&setup_pin_code_mac_g+3) ^= *(uint8_t*)&setup_pin_code_mac_g;
    discriminator_mac_g &= 0xFFF;
    setup_pin_code_mac_g &= 0x7FFFFFF;
    while(++discriminator_mac_g<3);
    while(++setup_pin_code_mac_g<3);
    while(--discriminator_mac_g>0xFFF);
    while(--setup_pin_code_mac_g>0x7FFFFFF);
    discriminator_mac_g = (setup_pin_code_mac_g % discriminator_mac_g);
    while(++discriminator_mac_g<3);
    while(--discriminator_mac_g>0xFFF);

    ChipLogProgress(NotSpecified, "========================================");
    ChipLogProgress(NotSpecified, "======Relay sw ver " CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING " built at " __DATE__ " " __TIME__);
    ChipLogProgress(NotSpecified, "======get MAC: %02X:%02X:%02X:%02X:%02X:%02X ", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    ChipLogProgress(NotSpecified, "======discriminator = %03X ", discriminator_mac_g);
    ChipLogProgress(NotSpecified, "========================================");
}

void StartAppTask(void)
{
    get_mac_init();
    GetAppTask().sAppTaskHandle = xTaskCreateStatic(GetAppTask().AppTaskMain, APP_TASK_NAME, ArraySize(GetAppTask().appStack), NULL,
                                                    APP_TASK_PRIORITY, GetAppTask().appStack, &GetAppTask().appTaskStruct);
    if (GetAppTask().sAppTaskHandle == NULL)
    {
        ChipLogError(NotSpecified, "Failed to create app task");
        appError(APP_ERROR_EVENT_QUEUE_FAILED);
    }
}

#if CONFIG_ENABLE_CHIP_SHELL
void AppTask::AppShellTask(void * args)
{
    Engine::Root().RunMainLoop();
}

CHIP_ERROR AppTask::StartAppShellTask()
{
    static TaskHandle_t shellTask;

    Engine::Root().Init();

    cmd_misc_init();

    xTaskCreate(AppTask::AppShellTask, "chip_shell", 1024 / sizeof(configSTACK_DEPTH_TYPE), NULL, APP_TASK_PRIORITY, &shellTask);

    return CHIP_NO_ERROR;
}
#endif

void AppTask::PostEvent(app_event_t event)
{
    if (xPortIsInsideInterrupt())
    {
        BaseType_t higherPrioTaskWoken = pdFALSE;
        xTaskNotifyFromISR(sAppTaskHandle, event, eSetBits, &higherPrioTaskWoken);
    }
    else
    {
        xTaskNotify(sAppTaskHandle, event, eSetBits);
    }
}

void AppTask::AppTaskMain(void * pvParameter)
{
    app_event_t appEvent;
    bool onoff = false;

    sLightLED.Init();

#ifdef BOOT_PIN_RESET
    ButtonInit();
#else
    /** Without RESET PIN defined, factory reset will be executed if power cycle count(resetCnt) >= APP_REBOOT_RESET_COUNT */
    uint32_t resetCnt      = 0;
    size_t saved_value_len = 0;
    ef_get_env_blob(APP_REBOOT_RESET_COUNT_KEY, &resetCnt, sizeof(resetCnt), &saved_value_len);
    ChipLogProgress(NotSpecified, "3R:resetCnt = %d \r\n", (int)resetCnt);

    if (resetCnt >= APP_REBOOT_RESET_COUNT - 1)
    {
        sLightLED.SetColor(34, 0, 254); // Red:(XX, 0, 254)
        resetCnt = 0;
        /** To share with RESET PIN logic, mButtonPressedTime is used to recorded resetCnt increased.
         * +1 makes sure mButtonPressedTime is not zero;
         * a power cycle during factory reset confirm time APP_BUTTON_PRESS_LONG will cancel factoryreset */
        GetAppTask().mButtonPressedTime = System::SystemClock().GetMonotonicMilliseconds64().count() + 1;
    }
    else
    {
        sLightLED.SetColor(34, 170, 254); // Blue:(XX, 170, 254)
        resetCnt++;
        GetAppTask().mButtonPressedTime = 0;
    }
    ef_set_env_blob(APP_REBOOT_RESET_COUNT_KEY, &resetCnt, sizeof(resetCnt));
#endif

    GetAppTask().sTimer = xTimerCreate("lightTmr", pdMS_TO_TICKS(1000), false, NULL, AppTask::TimerCallback);
    if (GetAppTask().sTimer == NULL)
    {
        ChipLogError(NotSpecified, "Failed to create timer task");
        appError(APP_ERROR_EVENT_QUEUE_FAILED);
    }

    ChipLogProgress(NotSpecified, "Starting Platform Manager Event Loop");
    CHIP_ERROR ret = PlatformMgr().StartEventLoopTask();
    if (ret != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "PlatformMgr().StartEventLoopTask() failed");
        appError(ret);
    }

    GetAppTask().PostEvent(APP_EVENT_TIMER);
    GetAppTask().PostEvent(APP_EVENT_LIGHTING_MASK);

    vTaskSuspend(NULL);

    GetAppTask().u8Config = 11;
    GetAppTask().OptionRead();
    ChipLogProgress(NotSpecified, "App Task started, with SRAM heap %d left\r\n", xPortGetFreeHeapSize());

    while (true)
    {
        appEvent                 = APP_EVENT_NONE;
        BaseType_t eventReceived = xTaskNotifyWait(0, APP_EVENT_ALL_MASK, (uint32_t *) &appEvent, portMAX_DELAY);

        if (eventReceived)
        {
            PlatformMgr().LockChipStack();

            if (APP_EVENT_LIGHTING_MASK & appEvent)
            {
                LightingUpdate(appEvent);
            }

            if (APP_EVENT_BTN_SHORT & appEvent)
            {
                if (Server::GetInstance().GetFabricTable().FabricCount())
                {
                    Clusters::OnOff::Attributes::OnOff::Get(GetAppTask().GetEndpointId(), &onoff);
                    onoff = !onoff;
                    Clusters::OnOff::Attributes::OnOff::Set(GetAppTask().GetEndpointId(), onoff);
                }
                else
                {
                    sLightLED.Toggle();
                }
            }

            if (APP_EVENT_IDENTIFY_MASK & appEvent)
            {
                IdentifyHandleOp(appEvent);
            }

            if (APP_EVENT_FACTORY_RESET & appEvent)
            {
                DeviceLayer::ConfigurationMgr().InitiateFactoryReset();
            }

            TimerEventHandler(appEvent);

            PlatformMgr().UnlockChipStack();
        }
    }
}

uint8_t AppTask::SetLedState(uint8_t endpt, bool onoff)
{
    if (onoff)
        u8LedState |= (uint8_t)0x1 << (endpt-1);
    else
        u8LedState &= ~((uint8_t)0x1 << (endpt-1));
    return u8LedState;
}

void AppTask::GetU8config(uint8_t &u8cfg)
{
   u8cfg = u8Config;
}

uint8_t AppTask::OptionRead()
{
    uint8_t v1 = 255, v2 = 255;
    bl_gpio_enable_input(20, 0, 1);
    bl_gpio_enable_input(0, 0, 1);
    bl_gpio_input_get(20, &v1);
    bl_gpio_input_get(0, &v2);
    if (v1==1 && v2==1)
        u8Config = 1;
    else if (v1==1 && v2==0)
        u8Config = 2;
    else if (v1==0 && v2==1)
        u8Config = 3;
    else if (v1==0 && v2==0)
        u8Config = 4;

    PlatformMgr().LockChipStack();
    if (u8Config==1)
    {
        emberAfEndpointEnableDisable((chip::EndpointId)2, false);
        emberAfEndpointEnableDisable((chip::EndpointId)3, false);
    }
    else if (u8Config==2 || u8Config==3)
    {
        emberAfEndpointEnableDisable((chip::EndpointId)3, false);
    }
    PlatformMgr().UnlockChipStack();

    return u8Config;
}

void AppTask::LightingUpdate(app_event_t status)
{
    uint8_t hue, sat;
    bool onoff;
    bool onoff2 = false;
    bool onoff3 = false;

    DataModel::Nullable<uint8_t> v(0);
    EndpointId endpoint = GetAppTask().GetEndpointId();
    uint8_t u8cfg=0;
    GetAppTask().GetU8config(u8cfg);

    if (APP_EVENT_LIGHTING_MASK & status)
    {

        if (Server::GetInstance().GetFabricTable().FabricCount())
        {
            do
            {
                if (u8cfg==2 || u8cfg==3)
                {
                    if (EMBER_ZCL_STATUS_SUCCESS != Clusters::OnOff::Attributes::OnOff::Get((EndpointId)2, &onoff2))
                        break;
                }
                else if (u8cfg==4)
                {
                    if (EMBER_ZCL_STATUS_SUCCESS != Clusters::OnOff::Attributes::OnOff::Get((EndpointId)2, &onoff2))
                        break;
                    if (EMBER_ZCL_STATUS_SUCCESS != Clusters::OnOff::Attributes::OnOff::Get((EndpointId)3, &onoff3))
                        break;
                }

                if (EMBER_ZCL_STATUS_SUCCESS != Clusters::OnOff::Attributes::OnOff::Get(endpoint, &onoff))
                {
                    break;
                }

                if (EMBER_ZCL_STATUS_SUCCESS != Clusters::LevelControl::Attributes::CurrentLevel::Get(endpoint, v))
                {
                    break;
                }

                if (EMBER_ZCL_STATUS_SUCCESS != Clusters::ColorControl::Attributes::CurrentHue::Get(endpoint, &hue))
                {
                    break;
                }

                if (EMBER_ZCL_STATUS_SUCCESS != Clusters::ColorControl::Attributes::CurrentSaturation::Get(endpoint, &sat))
                {
                    break;
                }

                if (!onoff && !onoff2 && !onoff3)
                {
                    sLightLED.SetColor(0, 0, 0); // LED_OFF:(0, XX, XX)
                }
                else
                {
#if defined(BL706_NIGHT_LIGHT) || defined(BL602_NIGHT_LIGHT)
                    sLightLED.SetColor(36, 84, 254);  // Green:(XX, 84, 254)
#else
                    if (v.IsNull())
                    {
                        // Just pick something.
                        sLightLED.SetColor(36, 170, 254); // Blue:(XX, 170, 254)
                    }
                    sLightLED.SetLevel(v.Value());
#endif
                }
            } while (0);
        }
        else
        {
#if defined(BL706_NIGHT_LIGHT) || defined(BL602_NIGHT_LIGHT)
            /** show blue to indicate not-provision state for extended color light */
            sLightLED.SetColor(36, 170, 254);  // Blue:(XX, 170, 254)
#else
            /** show 30% brightness to indicate not-provision state */
            // sLightLED.SetLevel(25);
#endif
        }
    }
}

bool AppTask::StartTimer(void)
{
    if (xTimerIsTimerActive(GetAppTask().sTimer))
    {
        CancelTimer();
    }

    if (GetAppTask().mTimerIntvl == 0)
    {
        GetAppTask().mTimerIntvl = 1000;
    }

    if (xTimerChangePeriod(GetAppTask().sTimer, pdMS_TO_TICKS(GetAppTask().mTimerIntvl), pdMS_TO_TICKS(100)) != pdPASS)
    {
        ChipLogProgress(NotSpecified, "Failed to access timer with 100 ms delay.");
    }

    return true;
}

void AppTask::CancelTimer(void)
{
    xTimerStop(GetAppTask().sTimer, 0);
}

void AppTask::TimerCallback(TimerHandle_t xTimer)
{
    GetAppTask().PostEvent(APP_EVENT_TIMER);
}

void AppTask::TimerEventHandler(app_event_t event)
{
    static uint8_t fctRst = 0;
    static uint8_t gpioCount = 0;
    uint8_t gpioValue = 255;
    uint8_t u8cfg = 0;

    if (GetAppTask().mButtonPressedTime)
    {
#ifdef BOOT_PIN_RESET
        if (System::SystemClock().GetMonotonicMilliseconds64().count() - GetAppTask().mButtonPressedTime >= APP_BUTTON_PRESS_SHORT)
        {
#if defined(BL602_NIGHT_LIGHT) || defined(BL706_NIGHT_LIGHT)
            /** change color to indicate to wait factory reset confirm */
            // set_color(6, 35, 254);
#else
            /** toggle led to indicate to wait factory reset confirm */
            // sLightLED.Toggle();
#endif
        }
#else
        if (System::SystemClock().GetMonotonicMilliseconds64().count() - GetAppTask().mButtonPressedTime > APP_BUTTON_PRESS_LONG)
        {
            /** factory reset confirm timeout */
            GetAppTask().mButtonPressedTime = 0;
            GetAppTask().PostEvent(APP_EVENT_FACTORY_RESET);
        }
        else
        {
#if defined(BL602_NIGHT_LIGHT) || defined(BL706_NIGHT_LIGHT)
            /** change color to indicate to wait factory reset confirm */
            sLightLED.SetColor(254, 0, 210);
#else
            /** toggle led to indicate to wait factory reset confirm */
            sLightLED.Toggle();
#endif
        }
#endif
    }

    if(1)
    {
        if (gpioCount < 120)
        {
            bl_gpio_enable_input(2, 1, 0);
            bl_gpio_input_get(2, &gpioValue);
            // ChipLogProgress(Zcl, "gpio_2_value : %u", gpioValue);

            if (0 == Server::GetInstance().GetFabricTable().FabricCount())
            {
                if (gpioCount%2 == 0)
                    sLightLED.SetColor(36, 170, 254);  // Blue:(XX, 170, 254)
                else
                    sLightLED.SetColor(0, 170, 254);  // LED_OFF:(0, XX, XX)
            }
            else if (gpioCount > 10)
            {
                bool onoff = false;
                bool onoff2 = false;
                bool onoff3 = false;
                GetAppTask().GetU8config(u8cfg);

                Clusters::OnOff::Attributes::OnOff::Get(GetAppTask().GetEndpointId(), &onoff);

                if (u8cfg == 2 || u8cfg == 3)
                    Clusters::OnOff::Attributes::OnOff::Get((EndpointId)2, &onoff2);
                else if (u8cfg==4)
                {
                    Clusters::OnOff::Attributes::OnOff::Get((EndpointId)2, &onoff2);
                    Clusters::OnOff::Attributes::OnOff::Get((EndpointId)3, &onoff3);
                }

                Clusters::OnOff::Attributes::OnOff::Set(GetAppTask().GetEndpointId(), onoff);
                if (u8cfg == 2 || u8cfg == 3)
                    Clusters::OnOff::Attributes::OnOff::Set((EndpointId)2, onoff2);
                else if (u8cfg == 4)
                {
                    Clusters::OnOff::Attributes::OnOff::Set((EndpointId)2, onoff2);
                    Clusters::OnOff::Attributes::OnOff::Set((EndpointId)3, onoff3);
                }

                if (!onoff && !onoff2 && !onoff3)
                    sLightLED.SetColor(0, 84, 254);  // Green
                else
                    sLightLED.SetColor(36, 84, 254); // LED_OFF:(0, XX, XX)
            }

            if ((gpioCount < 10) & (!gpioValue))
            {
                gpioCount = 0;
                if (++fctRst>6)
                {
                    sLightLED.SetColor(0, 0, 254);  // LED_OFF:(0, XX, XX)
                    vTaskDelay(500);
                    sLightLED.SetColor(36, 0, 254); // Red:(XX, 0, 254)
                    vTaskDelay(500);
                    DeviceLayer::ConfigurationMgr().InitiateFactoryReset();
                }
            }
            if (gpioCount == 10)
            {
                uint32_t resetCnt = 0;
                ef_set_env_blob(APP_REBOOT_RESET_COUNT_KEY, &resetCnt, sizeof(resetCnt));
                ChipLogProgress(NotSpecified, "3R:APP_REBOOT_RESET_COUNT_KEY reset 0.");
            }
            ++gpioCount;
        }
    }
    StartTimer();
}

void AppTask::IdentifyStartHandler(Identify *)
{
    GetAppTask().PostEvent(APP_EVENT_IDENTIFY_START);
}

void AppTask::IdentifyStopHandler(Identify *)
{
    GetAppTask().PostEvent(APP_EVENT_IDENTIFY_STOP);
}

void AppTask::IdentifyHandleOp(app_event_t event)
{
    static uint32_t identifyState = 0;

    if (APP_EVENT_IDENTIFY_START & event)
    {
        identifyState = 1;
        ChipLogProgress(NotSpecified, "identify start");
    }

    if ((APP_EVENT_IDENTIFY_IDENTIFY & event) && identifyState)
    {
        sLightLED.Toggle();
        ChipLogProgress(NotSpecified, "identify");
    }

    if (APP_EVENT_IDENTIFY_STOP & event)
    {
        identifyState = 0;
        GetAppTask().PostEvent(APP_EVENT_LIGHTING_MASK);
        ChipLogProgress(NotSpecified, "identify stop");
    }
}

void AppTask::ButtonEventHandler(uint8_t btnIdx, uint8_t btnAction)
{
    GetAppTask().PostEvent(APP_EVENT_FACTORY_RESET);
}

#ifdef BOOT_PIN_RESET
hosal_gpio_dev_t gpio_key = { .port = BOOT_PIN_RESET, .config = INPUT_HIGH_IMPEDANCE, .priv = NULL };

void AppTask::ButtonInit(void)
{
    GetAppTask().mButtonPressedTime = 0;

    hosal_gpio_init(&gpio_key);
    hosal_gpio_irq_set(&gpio_key, HOSAL_IRQ_TRIG_POS_PULSE, GetAppTask().ButtonEventHandler, NULL);
}

bool AppTask::ButtonPressed(void)
{
    uint8_t val = 1;
    hosal_gpio_input_get(&gpio_key, &val);
    return val == 1;
}

void AppTask::ButtonEventHandler(void * arg)
{
    uint32_t presstime;
    if (ButtonPressed())
    {
#ifdef BL702L_ENABLE
        bl_set_gpio_intmod(gpio_key.port, HOSAL_IRQ_TRIG_NEG_LEVEL);
#else
        bl_set_gpio_intmod(gpio_key.port, 1, HOSAL_IRQ_TRIG_NEG_LEVEL);
#endif
        GetAppTask().mButtonPressedTime = System::SystemClock().GetMonotonicMilliseconds64().count();
        GetAppTask().mTimerIntvl        = APP_BUTTON_PRESS_JITTER;
        GetAppTask().PostEvent(APP_EVENT_TIMER);
    }
    else
    {
#ifdef BL702L_ENABLE
        bl_set_gpio_intmod(gpio_key.port, HOSAL_IRQ_TRIG_POS_PULSE);
#else
        bl_set_gpio_intmod(gpio_key.port, 1, HOSAL_IRQ_TRIG_POS_PULSE);
#endif
        if (GetAppTask().mButtonPressedTime)
        {
            presstime = System::SystemClock().GetMonotonicMilliseconds64().count() - GetAppTask().mButtonPressedTime;
            if (presstime >= APP_BUTTON_PRESS_LONG)
            {
                GetAppTask().PostEvent(APP_EVENT_FACTORY_RESET);
            }
            else if (presstime <= APP_BUTTON_PRESS_SHORT && presstime >= APP_BUTTON_PRESS_JITTER)
            {
                GetAppTask().mTimerIntvl = 1000;
                GetAppTask().PostEvent(APP_EVENT_BTN_SHORT);
            }
            else
            {
                GetAppTask().mTimerIntvl = 1000;
                GetAppTask().PostEvent(APP_EVENT_LIGHTING_MASK);
            }
        }

        GetAppTask().mButtonPressedTime = 0;
    }
}
#endif
