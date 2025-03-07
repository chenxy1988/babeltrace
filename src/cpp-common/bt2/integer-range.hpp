/*
 * Copyright (c) 2020 Philippe Proulx <pproulx@efficios.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2_INTEGER_RANGE_HPP
#define BABELTRACE_CPP_COMMON_BT2_INTEGER_RANGE_HPP

#include <type_traits>
#include <cstdint>
#include <babeltrace2/babeltrace.h>

#include "internal/borrowed-obj.hpp"

namespace bt2 {

namespace internal {

template <typename ValueT>
struct ConstIntegerRangeSpec;

// Functions specific to unsigned integer ranges
template <>
struct ConstIntegerRangeSpec<const bt_integer_range_unsigned> final
{
    static std::uint64_t lower(const bt_integer_range_unsigned * const libRangePtr) noexcept
    {
        return bt_integer_range_unsigned_get_lower(libRangePtr);
    }

    static std::uint64_t upper(const bt_integer_range_unsigned * const libRangePtr) noexcept
    {
        return bt_integer_range_unsigned_get_upper(libRangePtr);
    }

    static bool isEqual(const bt_integer_range_unsigned * const libRangePtrA,
                        const bt_integer_range_unsigned * const libRangePtrB) noexcept
    {
        return static_cast<bool>(bt_integer_range_unsigned_is_equal(libRangePtrA, libRangePtrB));
    }
};

// Functions specific to signed integer ranges
template <>
struct ConstIntegerRangeSpec<const bt_integer_range_signed> final
{
    static std::int64_t lower(const bt_integer_range_signed * const libRangePtr) noexcept
    {
        return bt_integer_range_signed_get_lower(libRangePtr);
    }

    static std::int64_t upper(const bt_integer_range_signed * const libRangePtr) noexcept
    {
        return bt_integer_range_signed_get_upper(libRangePtr);
    }

    static bool isEqual(const bt_integer_range_signed * const libRangePtrA,
                        const bt_integer_range_signed * const libRangePtrB) noexcept
    {
        return static_cast<bool>(bt_integer_range_signed_is_equal(libRangePtrA, libRangePtrB));
    }
};

} // namespace internal

template <typename LibObjT>
class ConstIntegerRange final : public internal::BorrowedObj<LibObjT>
{
private:
    using typename internal::BorrowedObj<LibObjT>::_ThisBorrowedObj;
    using typename internal::BorrowedObj<LibObjT>::_LibObjPtr;
    using _ThisConstIntegerRange = ConstIntegerRange<LibObjT>;

public:
    using Value =
        typename std::conditional<std::is_same<LibObjT, const bt_integer_range_unsigned>::value,
                                  std::uint64_t, std::int64_t>::type;

public:
    explicit ConstIntegerRange(const _LibObjPtr libObjPtr) noexcept : _ThisBorrowedObj {libObjPtr}
    {
    }

    ConstIntegerRange(const _ThisConstIntegerRange& range) noexcept : _ThisBorrowedObj {range}
    {
    }

    _ThisConstIntegerRange& operator=(const _ThisConstIntegerRange& range) noexcept
    {
        _ThisBorrowedObj::operator=(range);
        return *this;
    }

    bool operator==(const _ThisConstIntegerRange& other) const noexcept
    {
        return internal::ConstIntegerRangeSpec<LibObjT>::isEqual(this->_libObjPtr(),
                                                                 other._libObjPtr());
    }

    bool operator!=(const _ThisConstIntegerRange& other) const noexcept
    {
        return !(*this == other);
    }

    Value lower() const noexcept
    {
        return internal::ConstIntegerRangeSpec<LibObjT>::lower(this->_libObjPtr());
    }

    Value upper() const noexcept
    {
        return internal::ConstIntegerRangeSpec<LibObjT>::upper(this->_libObjPtr());
    }
};

using ConstUnsignedIntegerRange = ConstIntegerRange<const bt_integer_range_unsigned>;
using ConstSignedIntegerRange = ConstIntegerRange<const bt_integer_range_signed>;

} // namespace bt2

#endif // BABELTRACE_CPP_COMMON_BT2_INTEGER_RANGE_HPP
