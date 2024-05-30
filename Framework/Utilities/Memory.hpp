#pragma once

#include "Utilities/Utility.hpp"
#include <memory>
#include <mutex>

namespace Utility
{
    // template <typename T>
    // class UniqueRef
    // {
    // public:
    //     NOCOPY(UniqueRef)
    //     UniqueRef(T* pointer);
    //     UniqueRef(std::unique_ptr<T>&& pointer);
    //     UniqueRef(UniqueRef&& other) noexcept;
    //     UniqueRef();
    //     ~UniqueRef();

    //     UniqueRef& operator=(T* pointer);
    //     UniqueRef& operator=(std::unique_ptr<T>&& pointer);
    //     UniqueRef& operator=(UniqueRef&& other) noexcept;

    //     T* operator->() const noexcept;
    //     T& operator*() const noexcept;
    //     bool operator==(const UniqueRef& other) const noexcept;
    //     bool operator!=(const UniqueRef& other) const noexcept;

    //     T* Get() const noexcept;
    //     void Reset(T* pointer = nullptr) noexcept;

    // private:
    //     std::unique_ptr<T> mPtr;
    // };
}