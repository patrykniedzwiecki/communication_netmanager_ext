/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "ethernet_service.h"

#include <new>
#include <sys/time.h>

#include "ethernet_constants.h"
#include "ethernet_management.h"
#include "interface_configuration.h"
#include "iremote_object.h"
#include "net_ethernet_base_service.h"
#include "net_manager_center.h"
#include "netmanager_base_permission.h"
#include "netmgr_ext_log_wrapper.h"
#include "netsys_controller.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace NetManagerStandard {
namespace {
constexpr uint16_t DEPENDENT_SERVICE_NET_CONN_MANAGER = 0x0001;
constexpr uint16_t DEPENDENT_SERVICE_COMMON_EVENT = 0x0002;
constexpr uint16_t DEPENDENT_SERVICE_All = 0x0003;
const bool REGISTER_LOCAL_RESULT_ETH =
    SystemAbility::MakeAndRegisterAbility(DelayedSingleton<EthernetService>::GetInstance().get());
} // namespace

EthernetService::EthernetService() : SystemAbility(COMM_ETHERNET_MANAGER_SYS_ABILITY_ID, true) {}

EthernetService::~EthernetService() = default;

void EthernetService::OnStart()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    NETMGR_EXT_LOG_D("EthernetService::OnStart begin");
    if (state_ == STATE_RUNNING) {
        NETMGR_EXT_LOG_D("EthernetService the state is already running");
        return;
    }
    if (!Init()) {
        NETMGR_EXT_LOG_E("EthernetService init failed");
        return;
    }
    state_ = STATE_RUNNING;
    gettimeofday(&tv, NULL);
    NETMGR_EXT_LOG_D("EthernetService::OnStart end");
}

void EthernetService::OnStop()
{
    state_ = STATE_STOPPED;
    registerToService_ = false;
}

int32_t EthernetService::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    NETMGR_EXT_LOG_D("Start Dump, fd: %{public}d", fd);
    std::string result;
    if (ethManagement_ == nullptr) {
        return ETHERNET_ERROR;
    }
    ethManagement_->GetDumpInfo(result);
    int32_t ret = dprintf(fd, "%s\n", result.c_str());
    return ret < 0 ? ETHERNET_ERROR : ETHERNET_SUCCESS;
}

void EthernetService::OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    switch (systemAbilityId) {
        case COMM_NET_CONN_MANAGER_SYS_ABILITY_ID:
            NETMGR_EXT_LOG_D("EthernetService::OnAddSystemAbility Conn");
            dependentServiceState_ |= DEPENDENT_SERVICE_NET_CONN_MANAGER;
            break;
        case COMMON_EVENT_SERVICE_ID:
            NETMGR_EXT_LOG_D("EthernetService::OnAddSystemAbility CES");
            dependentServiceState_ |= DEPENDENT_SERVICE_COMMON_EVENT;
            break;
        default:
            NETMGR_EXT_LOG_D("EthernetService::OnAddSystemAbility unhandled sysabilityId:%{public}d", systemAbilityId);
            break;
    }
    if (dependentServiceState_ == DEPENDENT_SERVICE_All) {
        InitManagement();
    }
}

bool EthernetService::Init()
{
    if (!REGISTER_LOCAL_RESULT_ETH) {
        NETMGR_EXT_LOG_E("EthernetService Register to local sa manager failed");
        return false;
    }
    if (!registerToService_) {
        if (!Publish(DelayedSingleton<EthernetService>::GetInstance().get())) {
            NETMGR_EXT_LOG_E("EthernetService Register to sa manager failed");
            return false;
        }
        registerToService_ = true;
    }
    AddSystemAbilityListener(COMM_NET_CONN_MANAGER_SYS_ABILITY_ID);
    AddSystemAbilityListener(COMMON_EVENT_SERVICE_ID);
    serviceComm_ = new (std::nothrow) EthernetServiceCommon();
    if (serviceComm_ == nullptr) {
        NETMGR_EXT_LOG_E("serviceComm_ is nullptr");
        return false;
    }
    NetManagerCenter::GetInstance().RegisterEthernetService(serviceComm_);
    return true;
}

void EthernetService::InitManagement()
{
    NETMGR_EXT_LOG_D("EthernetService::InitManagement Enter");
    if (ethManagement_ == nullptr) {
        ethManagement_ = std::make_unique<EthernetManagement>();
        ethManagement_->Init();
    }
}

int32_t EthernetService::SetIfaceConfig(const std::string &iface, sptr<InterfaceConfiguration> &ic)
{
    NETMGR_EXT_LOG_D("Set iface: %{public}s config", iface.c_str());
    if (!NetManagerPermission::CheckPermission(Permission::CONNECTIVITY_INTERNAL)) {
        NETMGR_EXT_LOG_E("EthernetService SetIfaceConfig no js permission");
        return ETHERNET_ERROR;
    }

    if (ethManagement_ != nullptr) {
        return ethManagement_->UpdateDevInterfaceCfg(iface, ic);
    } else {
        return ETHERNET_ERROR;
    }
}

sptr<InterfaceConfiguration> EthernetService::GetIfaceConfig(const std::string &iface)
{
    NETMGR_EXT_LOG_D("Get iface: %{public}s config", iface.c_str());
    if (!NetManagerPermission::CheckPermission(Permission::GET_NETWORK_INFO)) {
        NETMGR_EXT_LOG_E("EthernetService GetIfaceConfig no js permission");
        return nullptr;
    }

    if (ethManagement_ != nullptr) {
        return ethManagement_->GetDevInterfaceCfg(iface);
    } else {
        return nullptr;
    }
}

int32_t EthernetService::IsIfaceActive(const std::string &iface)
{
    NETMGR_EXT_LOG_D("Get iface: %{public}s is active", iface.c_str());
    if (!NetManagerPermission::CheckPermission(Permission::GET_NETWORK_INFO)) {
        NETMGR_EXT_LOG_E("EthernetService IsIfaceActive no js permission");
        return ETHERNET_ERROR;
    }

    if (ethManagement_ != nullptr) {
        return ethManagement_->IsIfaceActive(iface);
    } else {
        return ETHERNET_ERROR;
    }
}

std::vector<std::string> EthernetService::GetAllActiveIfaces()
{
    if (!NetManagerPermission::CheckPermission(Permission::GET_NETWORK_INFO)) {
        NETMGR_EXT_LOG_E("EthernetService GetAllActiveIfaces no js permission");
        return {};
    }

    if (ethManagement_ != nullptr) {
        return ethManagement_->GetAllActiveIfaces();
    } else {
        return {};
    }
}

int32_t EthernetService::ResetFactory()
{
    if (ethManagement_ != nullptr) {
        return ethManagement_->ResetFactory();
    } else {
        return ETHERNET_ERROR;
    }
}

int32_t EthernetService::SetInterfaceUp(const std::string &iface)
{
    NETMGR_EXT_LOG_D("Set interface: %{public}s up", iface.c_str());
    return NetsysController::GetInstance().SetInterfaceUp(iface);
}

int32_t EthernetService::SetInterfaceDown(const std::string &iface)
{
    NETMGR_EXT_LOG_D("Set interface: %{public}s down", iface.c_str());
    return NetsysController::GetInstance().SetInterfaceDown(iface);
}

bool EthernetService::GetInterfaceConfig(const std::string &iface, OHOS::nmd::InterfaceConfigurationParcel &config)
{
    NETMGR_EXT_LOG_D("Get interface: %{public}s config", iface.c_str());
    config.ifName = iface;
    int32_t ret = NetsysController::GetInstance().InterfaceGetConfig(config);
    if (ret != NO_ERROR) {
        return false;
    }
    return true;
}
} // namespace NetManagerStandard
} // namespace OHOS
