#include "pch.h"

#include "abi_writer.h"
#include "code_writers.h"
#include "metadata_cache.h"

using namespace std::literals;
using namespace xlang::meta::reader;
using namespace xlang::text;

metadata_cache::metadata_cache(xlang::meta::reader::cache const& c)
{
    // We need to initialize in two phases. The first phase creates the collection of all type defs. The second phase
    // processes dependencies and initializes generic types
    // NOTE: We may only need to do this for a subset of types, but that would introduce a fair amount of complexity and
    //       the runtime cost of processing everything is relatively insignificant
    xlang::task_group group;
    for (auto const& [ns, members] : c.namespaces())
    {
        // We don't do any synchronization of access to this type's data structures, so reserve space on the "main"
        // thread. Note that set/map iterators are not invalidated on insert/emplace
        auto [nsItr, nsAdded] = namespaces.emplace(std::piecewise_construct,
            std::forward_as_tuple(ns),
            std::forward_as_tuple());
        XLANG_ASSERT(nsAdded);

        auto [tableItr, tableAdded] = m_typeTable.emplace(std::piecewise_construct,
            std::forward_as_tuple(ns),
            std::forward_as_tuple());
        XLANG_ASSERT(tableAdded);

        group.add([&, &nsTypes = nsItr->second, &table = tableItr->second]()
        {
            process_namespace_types(members, nsTypes, table);
        });
    }
    group.get();

    for (auto& [ns, nsCache] : namespaces)
    {
        group.add([&]()
        {
            process_namespace_dependencies(nsCache);
        });
    }
    group.get();
}

void metadata_cache::process_namespace_types(
    cache::namespace_members const& members,
    namespace_cache& target,
    std::map<std::string_view, metadata_type const&>& table)
{
    // Mapped types are only in the 'Windows.Foundation' namespace, so pre-compute
    bool isFoundationNamespace = members.types.begin()->second.TypeNamespace() == foundation_namespace;

    target.enums.reserve(members.enums.size());
    for (auto const& e : members.enums)
    {
        // 'AsyncStatus' is an enum
        if (isFoundationNamespace)
        {
            if (auto ptr = mapped_type::from_typedef(e))
            {
                auto [itr, added] = table.emplace(e.TypeName(), *ptr);
                XLANG_ASSERT(added);
                continue;
            }
        }

        target.enums.emplace_back(e);
        auto [itr, added] = table.emplace(e.TypeName(), target.enums.back());
        XLANG_ASSERT(added);
    }

    target.structs.reserve(members.structs.size());
    for (auto const& s : members.structs)
    {
        // 'EventRegistrationToken' and 'HResult' are structs
        if (isFoundationNamespace)
        {
            if (auto ptr = mapped_type::from_typedef(s))
            {
                auto [itr, added] = table.emplace(s.TypeName(), *ptr);
                XLANG_ASSERT(added);
                continue;
            }
        }

        target.structs.emplace_back(s);
        auto [itr, added] = table.emplace(s.TypeName(), target.structs.back());
        XLANG_ASSERT(added);
    }

    target.delegates.reserve(members.delegates.size());
    for (auto const& d : members.delegates)
    {
        target.delegates.emplace_back(d);
        auto [itr, added] = table.emplace(d.TypeName(), target.delegates.back());
        XLANG_ASSERT(added);
    }

    target.interfaces.reserve(members.interfaces.size());
    for (auto const& i : members.interfaces)
    {
        // 'IAsyncInfo' is an interface
        if (isFoundationNamespace)
        {
            if (auto ptr = mapped_type::from_typedef(i))
            {
                auto [itr, added] = table.emplace(i.TypeName(), *ptr);
                XLANG_ASSERT(added);
                continue;
            }
        }

        target.interfaces.emplace_back(i);
        auto [itr, added] = table.emplace(i.TypeName(), target.interfaces.back());
        XLANG_ASSERT(added);
    }

    target.classes.reserve(members.classes.size());
    for (auto const& c : members.classes)
    {
        target.classes.emplace_back(c);
        auto [itr, added] = table.emplace(c.TypeName(), target.classes.back());
        XLANG_ASSERT(added);
    }

    for (auto const& contract : members.contracts)
    {
        // Contract versions are attributes on the contract type itself
        auto attr = get_attribute(contract, metadata_namespace, "ContractVersionAttribute"sv);
        XLANG_ASSERT(attr);
        XLANG_ASSERT(attr.Value().FixedArgs().size() == 1);

        target.contracts.push_back(api_contract{
                contract,
                std::get<uint32_t>(std::get<ElemSig>(attr.Value().FixedArgs()[0].value).value)
            });
    }
}

void metadata_cache::process_namespace_dependencies(namespace_cache& target)
{
    init_state state{ &target };

    for (auto& enumType : target.enums)
    {
        process_enum_dependencies(state, enumType);
        XLANG_ASSERT(!state.parent_generic_inst);
    }

    for (auto& structType : target.structs)
    {
        process_struct_dependencies(state, structType);
        XLANG_ASSERT(!state.parent_generic_inst);
    }

    for (auto& delegateType : target.delegates)
    {
        process_delegate_dependencies(state, delegateType);
        XLANG_ASSERT(!state.parent_generic_inst);
    }

    for (auto& interfaceType : target.interfaces)
    {
        process_interface_dependencies(state, interfaceType);
        XLANG_ASSERT(!state.parent_generic_inst);
    }

    for (auto& classType : target.classes)
    {
        process_class_dependencies(state, classType);
        XLANG_ASSERT(!state.parent_generic_inst);
    }
}

template <typename T>
static void process_contract_dependencies(namespace_cache& target, T const& type)
{
    if (auto attr = contract_attributes(type))
    {
        target.dependent_namespaces.emplace(decompose_type(attr->type_name).first);
        for (auto const& prevContract : attr->previous_contracts)
        {
            target.dependent_namespaces.emplace(decompose_type(prevContract.type_name).first);
        }
    }

    if (auto info = is_deprecated(type))
    {
        target.dependent_namespaces.emplace(decompose_type(info->contract_type).first);
    }
}

void metadata_cache::process_enum_dependencies(init_state& state, enum_type& type)
{
    // There's no pre-processing that we need to do for enums. Just take note of the namespace dependencies that come
    // from contract version(s)/deprecations
    process_contract_dependencies(*state.target, type.type());

    for (auto const& field : type.type().FieldList())
    {
        process_contract_dependencies(*state.target, field);
    }
}

void metadata_cache::process_struct_dependencies(init_state& state, struct_type& type)
{
    process_contract_dependencies(*state.target, type.type());

    for (auto const& field : type.type().FieldList())
    {
        process_contract_dependencies(*state.target, field);
        type.members.push_back(struct_member{ field, &find_dependent_type(state, field.Signature().Type()) });
    }
}

void metadata_cache::process_delegate_dependencies(init_state& state, delegate_type& type)
{
    process_contract_dependencies(*state.target, type.type());

    // We only care about instantiations of generic types, so early exit as we won't be able to resolve references
    if (type.is_generic())
    {
        return;
    }

    // Delegates only have a single function - Invoke - that we care about
    for (auto const& method : type.type().MethodList())
    {
        if (method.Name() != ".ctor"sv)
        {
            XLANG_ASSERT(method.Name() == "Invoke"sv);
            process_contract_dependencies(*state.target, method);
            type.invoke = process_function(state, method);
            break;
        }
    }

    XLANG_ASSERT(type.invoke.def);
}

void metadata_cache::process_interface_dependencies(init_state& state, interface_type& type)
{
    process_contract_dependencies(*state.target, type.type());

    // We only care about instantiations of generic types, so early exit as we won't be able to resolve references
    if (type.is_generic())
    {
        return;
    }

    for (auto const& iface : type.type().InterfaceImpl())
    {
        process_contract_dependencies(*state.target, iface);
        type.required_interfaces.push_back(&find_dependent_type(state, iface.Interface()));
    }

    for (auto const& method : type.type().MethodList())
    {
        process_contract_dependencies(*state.target, method);
        type.functions.push_back(process_function(state, method));
    }
}

void metadata_cache::process_class_dependencies(init_state& state, class_type& type)
{
    process_contract_dependencies(*state.target, type.type());

    // We only care about instantiations of generic types, so early exit as we won't be able to resolve references
    if (type.is_generic())
    {
        return;
    }

    // For classes, all we care about is the interfaces and taking note of the default interface, if there is one
    for (auto const& iface : type.type().InterfaceImpl())
    {
        process_contract_dependencies(*state.target, iface);
        auto ifaceType = &find_dependent_type(state, iface.Interface());
        type.required_interfaces.push_back(ifaceType);

        if (auto attr = get_attribute(iface, metadata_namespace, "DefaultAttribute"sv))
        {
            XLANG_ASSERT(!type.default_interface);
            type.default_interface = ifaceType;
        }
    }
}

function_def metadata_cache::process_function(init_state& state, MethodDef const& def)
{
    auto paramNames = def.ParamList();
    auto sig = def.Signature();
    XLANG_ASSERT(sig.GenericParamCount() == 0);

    std::optional<function_return_type> return_type;
    if (sig.ReturnType())
    {
        std::string_view name = "value"sv;
        if ((paramNames.first != paramNames.second) && (paramNames.first.Sequence() == 0))
        {
            name = paramNames.first.Name();
            ++paramNames.first;
        }

        return_type = function_return_type{ sig.ReturnType(), name, &find_dependent_type(state, sig.ReturnType().Type()) };
    }

    std::vector<function_param> params;
    for (auto const& param : sig.Params())
    {
        XLANG_ASSERT(paramNames.first != paramNames.second);
        params.push_back(function_param{ param, paramNames.first.Name(), &find_dependent_type(state, param.Type()) });
        ++paramNames.first;
    }

    return function_def{ def, std::move(return_type), std::move(params) };
}

metadata_type const& metadata_cache::find_dependent_type(init_state& state, TypeSig const& type)
{
    metadata_type const* result;
    xlang::visit(type.Type(),
        [&](ElementType t)
        {
            result = &element_type::from_type(t);
        },
        [&](coded_index<TypeDefOrRef> const& t)
        {
            result = &find_dependent_type(state, t);
        },
        [&](GenericTypeIndex t)
        {
            if (state.parent_generic_inst)
            {
                if (t.index < state.parent_generic_inst->generic_params().size())
                {
                    result = state.parent_generic_inst->generic_params()[t.index];
                }
                else
                {
                    XLANG_ASSERT(false);
                    xlang::throw_invalid("GenericTypeIndex out of range");
                }
            }
            else
            {
                XLANG_ASSERT(false);
                xlang::throw_invalid("GenericTypeIndex encountered with no generic instantiation to refer to");
            }
        },
        [&](GenericTypeInstSig const& t)
        {
            result = &find_dependent_type(state, t);
        });

    return *result;
}

metadata_type const& metadata_cache::find_dependent_type(init_state& state, coded_index<TypeDefOrRef> const& type)
{
    metadata_type const* result;
    visit(type, xlang::visit_overload{
        [&](GenericTypeInstSig const& t)
        {
            result = &find_dependent_type(state, t);
        },
        [&](auto const& defOrRef)
        {
            result = &find(defOrRef.TypeNamespace(), defOrRef.TypeName());
            if (auto typeDef = dynamic_cast<typedef_base const*>(result))
            {
                state.target->dependent_namespaces.insert(result->clr_abi_namespace());
                if (!typeDef->is_generic())
                {
                    state.target->type_dependencies.emplace(*typeDef);
                }
            }
        }});

    return *result;
}

metadata_type const& metadata_cache::find_dependent_type(init_state& state, GenericTypeInstSig const& type)
{
    auto genericType = dynamic_cast<typedef_base const*>(&find_dependent_type(state, type.GenericType()));
    if (!genericType)
    {
        XLANG_ASSERT(false);
        xlang::throw_invalid("Generic types must be TypeDefs");
    }

    std::vector<metadata_type const*> genericParams;
    for (auto const& param : type.GenericArgs())
    {
        genericParams.push_back(&find_dependent_type(state, param));
    }

    generic_inst inst{ genericType, std::move(genericParams) };
    auto [itr, added] = state.target->generic_instantiations.emplace(inst.idl_name(), std::move(inst));
    if (added)
    {
        auto restore = std::exchange(state.parent_generic_inst, &itr->second);
        auto check_dependency = [&](auto const& t)
        {
            auto mdType = &find_dependent_type(state, t);
            if (auto genericType = dynamic_cast<generic_inst const*>(mdType))
            {
                itr->second.dependencies.push_back(genericType);
            }
        };

        for (auto const& iface : genericType->type().InterfaceImpl())
        {
            check_dependency(iface.Interface());
        }

        for (auto const& fn : genericType->type().MethodList())
        {
            if (fn.Name() == ".ctor"sv)
            {
                continue;
            }

            auto sig = fn.Signature();
            if (sig.ReturnType())
            {
                check_dependency(sig.ReturnType().Type());
            }

            for (auto const& param : sig.Params())
            {
                check_dependency(param.Type());
            }
        }

        state.parent_generic_inst = restore;
    }

    return itr->second;
}

template <typename T>
static void merge_into(std::vector<T>& from, std::vector<std::reference_wrapper<T const>>& to)
{
    std::vector<std::reference_wrapper<T const>> result;
    result.reserve(from.size() + to.size());
    std::merge(from.begin(), from.end(), to.begin(), to.end(), std::back_inserter(result));
    to.swap(result);
}

type_cache metadata_cache::compile_namespaces(std::initializer_list<std::string_view> targetNamespaces)
{
    type_cache result{ this };

    auto includes_namespace = [&](std::string_view ns)
    {
        return std::find(targetNamespaces.begin(), targetNamespaces.end(), ns) != targetNamespaces.end();
    };

    for (auto ns : targetNamespaces)
    {
        auto itr = namespaces.find(ns);
        if (itr == namespaces.end())
        {
            XLANG_ASSERT(false);
            xlang::throw_invalid("Namespace '", ns, "' not found");
        }

        // Merge the type definitions together
        merge_into(itr->second.enums, result.enums);
        merge_into(itr->second.structs, result.structs);
        merge_into(itr->second.delegates, result.delegates);
        merge_into(itr->second.interfaces, result.interfaces);
        merge_into(itr->second.classes, result.classes);

        // Merge the dependencies together
        result.dependent_namespaces.insert(
            itr->second.dependent_namespaces.begin(),
            itr->second.dependent_namespaces.end());

        result.generic_instantiations.insert(itr->second.generic_instantiations.begin(), itr->second.generic_instantiations.end());

        std::partition_copy(
            itr->second.type_dependencies.begin(),
            itr->second.type_dependencies.end(),
            std::inserter(result.internal_dependencies, result.internal_dependencies.end()),
            std::inserter(result.external_dependencies, result.external_dependencies.end()),
            [&](auto const& type) { return includes_namespace(type.get().clr_logical_namespace()); });
    }

    return result;
}