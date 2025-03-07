/*
 * Copyright (c) 2020 Philippe Proulx <pproulx@efficios.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2_CLOCK_CLASS_HPP
#define BABELTRACE_CPP_COMMON_BT2_CLOCK_CLASS_HPP

#include <type_traits>
#include <cstdint>
#include <string>
#include <babeltrace2/babeltrace.h>

#include "internal/borrowed-obj.hpp"
#include "internal/shared-obj.hpp"
#include "cpp-common/optional.hpp"
#include "cpp-common/string_view.hpp"
#include "cpp-common/uuid-view.hpp"
#include "lib-error.hpp"
#include "value.hpp"

namespace bt2 {

namespace internal {

struct ClockClassRefFuncs final
{
    static void get(const bt_clock_class * const libObjPtr)
    {
        bt_clock_class_get_ref(libObjPtr);
    }

    static void put(const bt_clock_class * const libObjPtr)
    {
        bt_clock_class_put_ref(libObjPtr);
    }
};

template <typename LibObjT>
struct CommonClockClassSpec;

// Functions specific to mutable clock classes
template <>
struct CommonClockClassSpec<bt_clock_class> final
{
    static bt_value *userAttributes(bt_clock_class * const libObjPtr) noexcept
    {
        return bt_clock_class_borrow_user_attributes(libObjPtr);
    }
};

// Functions specific to constant clock classes
template <>
struct CommonClockClassSpec<const bt_clock_class> final
{
    static const bt_value *userAttributes(const bt_clock_class * const libObjPtr) noexcept
    {
        return bt_clock_class_borrow_user_attributes_const(libObjPtr);
    }
};

} // namespace internal

class ClockClassOffset final
{
public:
    explicit ClockClassOffset(const std::int64_t seconds, const std::uint64_t cycles) :
        _mSeconds {seconds}, _mCycles {cycles}
    {
    }

    ClockClassOffset(const ClockClassOffset&) noexcept = default;
    ClockClassOffset& operator=(const ClockClassOffset&) noexcept = default;

    std::int64_t seconds() const noexcept
    {
        return _mSeconds;
    }

    std::uint64_t cycles() const noexcept
    {
        return _mCycles;
    }

private:
    std::int64_t _mSeconds;
    std::uint64_t _mCycles;
};

template <typename LibObjT>
class CommonClockClass final : public internal::BorrowedObj<LibObjT>
{
private:
    using typename internal::BorrowedObj<LibObjT>::_ThisBorrowedObj;
    using typename internal::BorrowedObj<LibObjT>::_LibObjPtr;
    using _ThisCommonClockClass = CommonClockClass<LibObjT>;

public:
    using Shared =
        internal::SharedObj<_ThisCommonClockClass, LibObjT, internal::ClockClassRefFuncs>;

    using UserAttributes =
        typename std::conditional<std::is_const<LibObjT>::value, ConstMapValue, MapValue>::type;

    explicit CommonClockClass(const _LibObjPtr libObjPtr) noexcept : _ThisBorrowedObj {libObjPtr}
    {
    }

    template <typename OtherLibObjT>
    CommonClockClass(const CommonClockClass<OtherLibObjT>& clkClass) noexcept :
        _ThisBorrowedObj {clkClass}
    {
    }

    template <typename OtherLibObjT>
    _ThisCommonClockClass& operator=(const CommonClockClass<OtherLibObjT>& clkClass) noexcept
    {
        _ThisBorrowedObj::operator=(clkClass);
        return *this;
    }

    void frequency(const std::uint64_t frequency) noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        bt_clock_class_set_frequency(this->_libObjPtr(), frequency);
    }

    std::uint64_t frequency() const noexcept
    {
        return bt_clock_class_get_frequency(this->_libObjPtr());
    }

    void offset(const ClockClassOffset& offset) noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        bt_clock_class_set_offset(this->_libObjPtr(), offset.seconds(), offset.cycles());
    }

    ClockClassOffset offset() const noexcept
    {
        std::int64_t seconds;
        std::uint64_t cycles;

        bt_clock_class_get_offset(this->_libObjPtr(), &seconds, &cycles);
        return ClockClassOffset {seconds, cycles};
    }

    void precision(const std::uint64_t precision) noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        bt_clock_class_set_precision(this->_libObjPtr(), precision);
    }

    std::uint64_t precision() const noexcept
    {
        return bt_clock_class_get_precision(this->_libObjPtr());
    }

    void originIsUnixEpoch(const bool originIsUnixEpoch) noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        bt_clock_class_set_origin_is_unix_epoch(this->_libObjPtr(),
                                                static_cast<bt_bool>(originIsUnixEpoch));
    }

    bool originIsUnixEpoch() const noexcept
    {
        return static_cast<bool>(bt_clock_class_origin_is_unix_epoch(this->_libObjPtr()));
    }

    void name(const char * const name)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto status = bt_clock_class_set_name(this->_libObjPtr(), name);

        if (status == BT_CLOCK_CLASS_SET_NAME_STATUS_MEMORY_ERROR) {
            throw LibMemoryError {};
        }
    }

    void name(const std::string& name)
    {
        this->name(name.data());
    }

    nonstd::optional<bpstd::string_view> name() const noexcept
    {
        const auto name = bt_clock_class_get_name(this->_libObjPtr());

        if (name) {
            return name;
        }

        return nonstd::nullopt;
    }

    void description(const char * const description)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto status = bt_clock_class_set_description(this->_libObjPtr(), description);

        if (status == BT_CLOCK_CLASS_SET_DESCRIPTION_STATUS_MEMORY_ERROR) {
            throw LibMemoryError {};
        }
    }

    void description(const std::string& description)
    {
        this->description(description.data());
    }

    nonstd::optional<bpstd::string_view> description() const noexcept
    {
        const auto description = bt_clock_class_get_description(this->_libObjPtr());

        if (description) {
            return description;
        }

        return nonstd::nullopt;
    }

    void uuid(const std::uint8_t * const uuid) noexcept
    {
        bt_clock_class_set_uuid(this->_libObjPtr(), uuid);
    }

    nonstd::optional<bt2_common::UuidView> uuid() const noexcept
    {
        const auto uuid = bt_clock_class_get_uuid(this->_libObjPtr());

        if (uuid) {
            return bt2_common::UuidView {uuid};
        }

        return nonstd::nullopt;
    }

    template <typename LibValT>
    void userAttributes(const CommonMapValue<LibValT>& userAttrs)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        bt_clock_class_set_user_attributes(this->_libObjPtr(), userAttrs._libObjPtr());
    }

    ConstMapValue userAttributes() const noexcept
    {
        return ConstMapValue {internal::CommonClockClassSpec<const bt_clock_class>::userAttributes(
            this->_libObjPtr())};
    }

    UserAttributes userAttributes() noexcept
    {
        return UserAttributes {
            internal::CommonClockClassSpec<LibObjT>::userAttributes(this->_libObjPtr())};
    }

    std::int64_t cyclesToNsFromOrigin(const std::uint64_t value) const
    {
        std::int64_t nsFromOrigin;
        const auto status =
            bt_clock_class_cycles_to_ns_from_origin(this->_libObjPtr(), value, &nsFromOrigin);

        if (status == BT_CLOCK_CLASS_CYCLES_TO_NS_FROM_ORIGIN_STATUS_OVERFLOW_ERROR) {
            throw LibOverflowError {};
        }

        return nsFromOrigin;
    }

    Shared shared() const noexcept
    {
        return Shared {*this};
    }
};

using ClockClass = CommonClockClass<bt_clock_class>;
using ConstClockClass = CommonClockClass<const bt_clock_class>;

} // namespace bt2

#endif // BABELTRACE_CPP_COMMON_BT2_CLOCK_CLASS_HPP
