/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/hw/sai/api/RouterInterfaceApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/switch/SaiRouteManager.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include "folly/MacAddress.h"

#include "folly/Overload.h"
#include "folly/container/F14Map.h"

#include <memory>
#include <mutex>
#include <variant>
#include <vector>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class SaiStore;

using SaiVlanRouterInterface = SaiObject<SaiVlanRouterInterfaceTraits>;
using SaiPortRouterInterface = SaiObject<SaiPortRouterInterfaceTraits>;

struct SaiRouterInterfaceHandle {
  using SaiRouterInterface = std::variant<
      std::shared_ptr<SaiVlanRouterInterface>,
      std::shared_ptr<SaiPortRouterInterface>>;
  SaiRouterInterface routerInterface;
  RouterInterfaceSaiId adapterKey() const {
    return std::visit(
        [](auto& handle) { return handle->adapterKey(); }, routerInterface);
  }
  cfg::InterfaceType type() const {
    return std::visit(
        folly::overload(
            [](const std::shared_ptr<SaiVlanRouterInterface>& handle) {
              return cfg::InterfaceType::VLAN;
            },
            [](const std::shared_ptr<SaiPortRouterInterface>& handle) {
              // TODO(Chenab): Support port router interface
              return cfg::InterfaceType::SYSTEM_PORT;
            }),
        routerInterface);
    CHECK(false) << " Unhandled interface type: ";
  }
  void setLocal(bool isLocal) {
    isLocalRif = isLocal;
  }
  bool isLocal() const {
    return isLocalRif;
  }
  std::vector<std::shared_ptr<SaiRoute>> toMeRoutes;
  bool isLocalRif{true};
};

class SaiRouterInterfaceManager {
 public:
  SaiRouterInterfaceManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);
  RouterInterfaceSaiId addLocalRouterInterface(
      const std::shared_ptr<Interface>& swInterface) {
    return addRouterInterface(swInterface, true /*isLocal*/);
  }
  void removeLocalRouterInterface(
      const std::shared_ptr<Interface>& swInterface) {
    removeRouterInterface(swInterface);
  }
  void changeLocalRouterInterface(
      const std::shared_ptr<Interface>& oldInterface,
      const std::shared_ptr<Interface>& newInterface) {
    changeRouterInterface(oldInterface, newInterface, true /*isLocal*/);
  }

  RouterInterfaceSaiId addRemoteRouterInterface(
      const std::shared_ptr<Interface>& swInterface) {
    return addRouterInterface(swInterface, false /*remote*/);
  }
  void removeRemoteRouterInterface(
      const std::shared_ptr<Interface>& swInterface) {
    removeRouterInterface(swInterface);
  }
  void changeRemoteRouterInterface(
      const std::shared_ptr<Interface>& oldInterface,
      const std::shared_ptr<Interface>& newInterface) {
    changeRouterInterface(oldInterface, newInterface, false /*remote*/);
  }

  SaiRouterInterfaceHandle* getRouterInterfaceHandle(const InterfaceID& swId);
  const SaiRouterInterfaceHandle* getRouterInterfaceHandle(
      const InterfaceID& swId) const;

 private:
  RouterInterfaceSaiId addRouterInterface(
      const std::shared_ptr<Interface>& swInterface,
      bool isLocal);
  void removeRouterInterface(const std::shared_ptr<Interface>& swInterface);
  void changeRouterInterface(
      const std::shared_ptr<Interface>& oldInterface,
      const std::shared_ptr<Interface>& newInterface,
      bool isLocal);
  RouterInterfaceSaiId addOrUpdateRouterInterface(
      const std::shared_ptr<Interface>& swInterface,
      bool isLocal);
  RouterInterfaceSaiId addOrUpdateVlanRouterInterface(
      const std::shared_ptr<Interface>& swInterface,
      bool isLocal);
  RouterInterfaceSaiId addOrUpdatePortRouterInterface(
      const std::shared_ptr<Interface>& swInterface,
      bool isLocal);
  SaiRouterInterfaceHandle* getRouterInterfaceHandleImpl(
      const InterfaceID& swId) const;

  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  folly::F14FastMap<InterfaceID, std::unique_ptr<SaiRouterInterfaceHandle>>
      handles_;
};

} // namespace facebook::fboss
