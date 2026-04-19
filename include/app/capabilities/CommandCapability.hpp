#pragma once

#include <string>
#include <memory>
#include <functional>
#include "ui/CommandRegistry.hpp"


namespace app::capabilities {

class CommandCapability {
public:
    CommandCapability() = default;
    virtual ~CommandCapability() = default;
 
private:
    

    std::shared_ptr<CommandRegistry> command_registry_;
};

}  // namespace app::capabilities
