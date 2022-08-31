/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef NETWORKSHARE_MAIN_STATEMACHINE_H
#define NETWORKSHARE_MAIN_STATEMACHINE_H

#include <map>
#include <any>

#include "networkshare_sub_statemachine.h"
#include "networkshare_upstreammonitor.h"

namespace OHOS {
namespace NetManagerStandard {
struct MessageIfaceActive {
    int value_;
    std::shared_ptr<NetworkShareSubStateMachine> subsm_;
};

struct MessageUpstreamInfo {
    int cmd_;
    std::shared_ptr<UpstreamNetworkInfo> upstreamInfo_;
};

class NetworkShareMainStateMachine {
    using HandleFunc = int (NetworkShareMainStateMachine::*)(const std::any &messageObj);

public:
    explicit NetworkShareMainStateMachine(std::shared_ptr<NetworkShareUpstreamMonitor> &networkmonitor);
    ~NetworkShareMainStateMachine() = default;

    /**
     * switch to error state when error occur
     */
    void SwitcheToErrorState(const int &errType);

    /**
     * execute state switch
     */
    void MainSmStateSwitch(int newState);

    /**
     * execute event
     */
    void MainSmEventHandle(int eventId, const std::any &messageObj);

private:
    bool TurnOnMainShareSettings();
    bool TurnOffMainShareSettings();
    int HandleInitInterfaceStateActive(const std::any &messageObj);
    int HandleInitInterfaceStateInactive(const std::any &messageObj);
    int HandleAliveInterfaceStateActive(const std::any &messageObj);
    int HandleAliveInterfaceStateInactive(const std::any &messageObj);
    int HandleAliveUpstreamMonitorCallback(const std::any &messageObj);
    int HandleErrorInterfaceStateInactive(const std::any &messageObj);
    int HandleErrorClear(const std::any &messageObj);
    void InitStateEnter();
    void AliveStateEnter();
    void ErrorStateEnter();
    void InitStateExit();
    void AliveStateExit();
    void ErrorStateExit();
    void ChooseUpstreamType();

private:
    struct MainSmStateTable {
        int event_;
        int curState_;
        int nextState_;
        HandleFunc func_;
    };

    std::recursive_mutex mutex_;
    std::string netshare_requester_;
    int32_t errorType_;
    bool hasSetForward_ = false;
    std::vector<std::shared_ptr<NetworkShareSubStateMachine>> subMachineList_;
    std::shared_ptr<NetworkShareUpstreamMonitor> networkMonitor_ = nullptr;
    int curState_;
    std::vector<MainSmStateTable> stateTable_;
};
} // namespace NetManagerStandard
} // namespace OHOS
#endif // NETWORKSHARE_MAIN_STATEMACHINE_H
