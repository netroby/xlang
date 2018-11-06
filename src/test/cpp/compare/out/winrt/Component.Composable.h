﻿// WARNING: Please don't edit this file. It was generated by C++/WinRT v2.0.000000.0
#pragma once
#include "winrt/impl/Component.Composable.2.h"
#include "winrt/Component.h"
namespace winrt::impl
{
    template <typename D> hstring consume_Component_Composable_IBase<D>::BaseMethod() const
    {
        void* result;
        check_hresult(WINRT_SHIM(Component::Composable::IBase)->BaseMethod(&result));
        return { take_ownership_from_abi, result };
    }
    template <typename D> Component::Composable::Base consume_Component_Composable_IBaseFactory<D>::CreateInstance(Windows::Foundation::IInspectable const& baseInterface, Windows::Foundation::IInspectable& innerInterface) const
    {
        void* value;
        check_hresult(WINRT_SHIM(Component::Composable::IBaseFactory)->CreateInstance(get_abi(baseInterface), put_abi(innerInterface), &value));
        return { take_ownership_from_abi, value };
    }
    template <typename D> hstring consume_Component_Composable_IDerived<D>::DerivedMethod() const
    {
        void* result;
        check_hresult(WINRT_SHIM(Component::Composable::IDerived)->DerivedMethod(&result));
        return { take_ownership_from_abi, result };
    }
    template <typename D>
    struct produce<D, Component::Composable::IBase> : produce_base<D, Component::Composable::IBase>
    {
        int32_t WINRT_CALL BaseMethod(void** result) noexcept final
        {
            try
            {
                *result = nullptr;
                typename D::abi_guard guard(this->shim());
                *result = detach_from<hstring>(this->shim().BaseMethod());
                return 0;
            }
            catch (...) { return to_hresult(); }
        }
    };
    template <typename D>
    struct produce<D, Component::Composable::IBaseFactory> : produce_base<D, Component::Composable::IBaseFactory>
    {
        int32_t WINRT_CALL CreateInstance(void* baseInterface, void** innerInterface, void** value) noexcept final
        {
            try
            {
                if (innerInterface) *innerInterface = nullptr;
                Windows::Foundation::IInspectable winrt_impl_innerInterface;
                *value = nullptr;
                typename D::abi_guard guard(this->shim());
                *value = detach_from<Component::Composable::Base>(this->shim().CreateInstance(*reinterpret_cast<Windows::Foundation::IInspectable const*>(&baseInterface), winrt_impl_innerInterface));
            if (innerInterface) *innerInterface = detach_abi(winrt_impl_innerInterface);
                return 0;
            }
            catch (...) { return to_hresult(); }
        }
    };
    template <typename D>
    struct produce<D, Component::Composable::IDerived> : produce_base<D, Component::Composable::IDerived>
    {
        int32_t WINRT_CALL DerivedMethod(void** result) noexcept final
        {
            try
            {
                *result = nullptr;
                typename D::abi_guard guard(this->shim());
                *result = detach_from<hstring>(this->shim().DerivedMethod());
                return 0;
            }
            catch (...) { return to_hresult(); }
        }
    };
}
namespace winrt::Component::Composable
{
    inline Base::Base()
    {
        Windows::Foundation::IInspectable baseInterface, innerInterface;
        *this = impl::call_factory<Base, Component::Composable::IBaseFactory>([&](auto&& f) { return f.CreateInstance(baseInterface, innerInterface); });
    }
    inline Derived::Derived() :
        Derived(impl::call_factory<Derived>([](auto&& f) { return f.template ActivateInstance<Derived>(); }))
    {
    }
template <typename D, typename... Interfaces>
struct BaseT :
    implements<D, Windows::Foundation::IInspectable, composing, Interfaces...>,
    impl::require<D, Component::Composable::IBase>,
    impl::base<D, Base>
{
    using composable = Base;

protected:
    BaseT()
    {
        impl::call_factory<Base, Component::Composable::IBaseFactory>([&](auto&& f) { f.CreateInstance(*this, this->m_inner); });
    }
};
}
namespace std
{
    template<> struct hash<winrt::Component::Composable::IBase> : winrt::impl::hash_base<winrt::Component::Composable::IBase> {};
    template<> struct hash<winrt::Component::Composable::IBaseFactory> : winrt::impl::hash_base<winrt::Component::Composable::IBaseFactory> {};
    template<> struct hash<winrt::Component::Composable::IDerived> : winrt::impl::hash_base<winrt::Component::Composable::IDerived> {};
    template<> struct hash<winrt::Component::Composable::Base> : winrt::impl::hash_base<winrt::Component::Composable::Base> {};
    template<> struct hash<winrt::Component::Composable::Derived> : winrt::impl::hash_base<winrt::Component::Composable::Derived> {};
}
