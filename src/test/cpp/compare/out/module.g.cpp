﻿// WARNING: Please don't edit this file. It was generated by C++/WinRT v2.0.000000.0
#include "pch.h"
#include "Component.Async.Class.h"
#include "Component.Collections.Class.h"
#include "Component.Edge.OneClass.h"
#include "Component.Edge.StaticClass.h"
#include "Component.Edge.ThreeClass.h"
#include "Component.Edge.TwoClass.h"
#include "Component.Edge.ZeroClass.h"
#include "Component.Fast.FastClass.h"
#include "Component.Fast.SlowClass.h"
#include "Component.Result.Class.h"

// Note: use "-lib example" option to change winrt_xxx to example_xxx to allow multiple libs to be stitched together.

bool WINRT_CALL winrt_can_unload_now() noexcept
{
    if (winrt::get_module_lock())
    {
        return false;
    }

    winrt::clear_factory_cache();
    return true;
}

void* WINRT_CALL winrt_get_activation_factory(std::wstring_view const& name)
{
    auto requal = [](std::wstring_view const& left, std::wstring_view const& right) noexcept
    {
        return std::equal(left.rbegin(), left.rend(), right.rbegin(), right.rend());
    };

    if (requal(name, L"Component.Async.Class"))
    {
        return winrt::detach_abi(winrt::make<winrt::Component::Async::factory_implementation::Class>());
    }

    if (requal(name, L"Component.Collections.Class"))
    {
        return winrt::detach_abi(winrt::make<winrt::Component::Collections::factory_implementation::Class>());
    }

    if (requal(name, L"Component.Edge.OneClass"))
    {
        return winrt::detach_abi(winrt::make<winrt::Component::Edge::factory_implementation::OneClass>());
    }

    if (requal(name, L"Component.Edge.StaticClass"))
    {
        return winrt::detach_abi(winrt::make<winrt::Component::Edge::factory_implementation::StaticClass>());
    }

    if (requal(name, L"Component.Edge.ThreeClass"))
    {
        return winrt::detach_abi(winrt::make<winrt::Component::Edge::factory_implementation::ThreeClass>());
    }

    if (requal(name, L"Component.Edge.TwoClass"))
    {
        return winrt::detach_abi(winrt::make<winrt::Component::Edge::factory_implementation::TwoClass>());
    }

    if (requal(name, L"Component.Edge.ZeroClass"))
    {
        return winrt::detach_abi(winrt::make<winrt::Component::Edge::factory_implementation::ZeroClass>());
    }

    if (requal(name, L"Component.Fast.FastClass"))
    {
        return winrt::detach_abi(winrt::make<winrt::Component::Fast::factory_implementation::FastClass>());
    }

    if (requal(name, L"Component.Fast.SlowClass"))
    {
        return winrt::detach_abi(winrt::make<winrt::Component::Fast::factory_implementation::SlowClass>());
    }

    if (requal(name, L"Component.Result.Class"))
    {
        return winrt::detach_abi(winrt::make<winrt::Component::Result::factory_implementation::Class>());
    }

    return nullptr;
}

int32_t WINRT_CALL WINRT_CanUnloadNow() noexcept
{
#ifdef _WRL_MODULE_H_
    if (!::Microsoft::WRL::Module<::Microsoft::WRL::InProc>::GetModule().Terminate())
    {
        return 1;
    }
#endif

    return winrt_can_unload_now() ? 0 : 1;
}

int32_t WINRT_CALL WINRT_GetActivationFactory(void* classId, void** factory) noexcept
{
    try
    {
        uint32_t length{};
        wchar_t const* const buffer = WINRT_WindowsGetStringRawBuffer(classId, &length);
        std::wstring_view const name{ buffer, length };

        *factory = winrt_get_activation_factory(name);

        if (*factory)
        {
            return 0;
        }

#ifdef _WRL_MODULE_H_
        return ::Microsoft::WRL::Module<::Microsoft::WRL::InProc>::GetModule().GetActivationFactory(static_cast<HSTRING>(classId), reinterpret_cast<::IActivationFactory**>(factory));
#else
        return winrt::hresult_class_not_available(name).to_abi();
#endif
    }
    catch (...) { return winrt::to_hresult(); }
}
