#pragma once

#include <memory>
#include <typeindex>
#include <map>

namespace graph {

class CapabilityBus {
public:
    virtual ~CapabilityBus() = default;

    template<typename CapabilityT>
    void Register(std::shared_ptr<CapabilityT> capability) {
        capabilities_[std::type_index(typeid(CapabilityT))] = capability;
    }

    template<typename CapabilityT>
    std::shared_ptr<CapabilityT> Get() const {
        auto it = capabilities_.find(std::type_index(typeid(CapabilityT)));
        if (it != capabilities_.end()) {
            return std::static_pointer_cast<CapabilityT>(it->second);
        }
        return nullptr;
    }

    template<typename CapabilityT>
    bool Has() const {
        return capabilities_.find(std::type_index(typeid(CapabilityT))) != capabilities_.end();
    }

    void Clear() {
        capabilities_.clear();
    }

private:
    std::map<std::type_index, std::shared_ptr<void>> capabilities_;
};

}