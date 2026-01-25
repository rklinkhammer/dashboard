#pragma once


#include <unordered_map>
#include <typeindex>
#include <memory>
#include "graph/CapabilityBus.hpp"

namespace graph {

class DefaultCapabilityBus : public CapabilityBus {
public:
    template<typename CapabilityT>
    void Register(std::shared_ptr<CapabilityT> capability) {
        static_assert(std::is_class_v<CapabilityT>);
        capabilities_[typeid(CapabilityT)] = std::move(capability);
    }

    template<typename CapabilityT>
    std::shared_ptr<CapabilityT> Get() const {
        auto it = capabilities_.find(typeid(CapabilityT));
        if (it == capabilities_.end()) {
            return nullptr;
        }
        return std::static_pointer_cast<CapabilityT>(it->second);
    }

    template<typename CapabilityT>
    bool Has() const {
        return capabilities_.count(typeid(CapabilityT)) != 0;
    }

    void Clear() {
        capabilities_.clear();
    }

private:
    std::unordered_map<std::type_index, std::shared_ptr<void>> capabilities_;
};

}