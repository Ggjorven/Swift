#pragma once

#include <memory>

// TODO: Include logging

namespace VkOutline
{

    template<typename T>
    using Ref = std::shared_ptr<T>;

    class RefHelper
    {
    public:
        template<typename T, typename... Args>
        static Ref<T> Create(Args&&... args)
        {
            return std::make_shared<T>(std::forward<Args>(args)...);
        }

        template<typename T, typename T2>
        static Ref<T> RefAs(Ref<T2> ptr)
        {
            Ref<T> newPtr = std::dynamic_pointer_cast<T>(ptr);

            if (!newPtr)
            {
                // TODO: Make an assert?
            }

            return newPtr;
        }
    };

}