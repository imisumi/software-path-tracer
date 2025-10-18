#pragma once

#include <string_view>

namespace ui
{

class IPanel
{
public:
    virtual ~IPanel() = default;

    virtual void render() = 0;
    virtual const std::string_view getTitle() const = 0;
    virtual bool isOpen() const = 0;
    virtual void setOpen(bool open) = 0;
};

}  // namespace ui
