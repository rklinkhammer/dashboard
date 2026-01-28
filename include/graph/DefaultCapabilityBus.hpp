// MIT License
//
// Copyright (c) 2025 Robert Klinkhammer
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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