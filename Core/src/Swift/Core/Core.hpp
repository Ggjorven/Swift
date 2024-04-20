#pragma once

#include <string>
#include <memory>

#include "Swift/Core/Logging.hpp"

namespace Swift
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
            APP_ASSERT(newPtr, "Assertion Failed: Failed to cast from Ref<T> to Ref<T2>");
            return newPtr;
        }
    };

    // Note(Jorben): I used to use this, but now I feel like it makes code less readable.
    /*
    // Typedefs
    typedef int8_t i8;
    typedef int16_t i16;
    typedef int32_t i32;
    typedef int64_t i64;

    typedef uint8_t u8;
    typedef uint16_t u16;
    typedef uint32_t u32;
    typedef uint64_t u64;

    typedef float f32;
    typedef double f64;

    typedef std::string str;
    */

}