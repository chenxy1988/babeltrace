/*
 * Copyright (c) 2020-2021 Philippe Proulx <pproulx@efficios.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2_TRACE_IR_HPP
#define BABELTRACE_CPP_COMMON_BT2_TRACE_IR_HPP

#include <type_traits>
#include <cstdint>
#include <babeltrace2/babeltrace.h>

#include "internal/borrowed-obj.hpp"
#include "cpp-common/optional.hpp"
#include "cpp-common/string_view.hpp"
#include "clock-class.hpp"
#include "clock-snapshot.hpp"
#include "field-class.hpp"
#include "field.hpp"
#include "value.hpp"
#include "internal/utils.hpp"

namespace bt2 {

template <typename LibObjT>
class CommonEvent;

template <typename LibObjT>
class CommonPacket;

template <typename LibObjT>
class CommonStream;

template <typename LibObjT>
class CommonTrace;

template <typename LibObjT>
class CommonEventClass;

template <typename LibObjT>
class CommonStreamClass;

template <typename LibObjT>
class CommonTraceClass;

namespace internal {

template <typename LibObjT>
struct CommonEventSpec;

// Functions specific to mutable events
template <>
struct CommonEventSpec<bt_event> final
{
    static bt_event_class *cls(bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_class(libObjPtr);
    }

    static bt_stream *stream(bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_stream(libObjPtr);
    }

    static bt_packet *packet(bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_packet(libObjPtr);
    }

    static bt_field *payloadField(bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_payload_field(libObjPtr);
    }

    static bt_field *specificContextField(bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_specific_context_field(libObjPtr);
    }

    static bt_field *commonContextField(bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_common_context_field(libObjPtr);
    }
};

// Functions specific to constant events
template <>
struct CommonEventSpec<const bt_event> final
{
    static const bt_event_class *cls(const bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_class_const(libObjPtr);
    }

    static const bt_stream *stream(const bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_stream_const(libObjPtr);
    }

    static const bt_packet *packet(const bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_packet_const(libObjPtr);
    }

    static const bt_field *payloadField(const bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_payload_field_const(libObjPtr);
    }

    static const bt_field *specificContextField(const bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_specific_context_field_const(libObjPtr);
    }

    static const bt_field *commonContextField(const bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_common_context_field_const(libObjPtr);
    }
};

} // namespace internal

template <typename LibObjT>
class CommonEvent final : public internal::BorrowedObj<LibObjT>
{
private:
    using typename internal::BorrowedObj<LibObjT>::_ThisBorrowedObj;
    using typename internal::BorrowedObj<LibObjT>::_LibObjPtr;
    using _ConstSpec = internal::CommonEventSpec<const bt_event>;
    using _Spec = internal::CommonEventSpec<LibObjT>;

    using _Packet =
        typename std::conditional<std::is_const<LibObjT>::value, CommonPacket<const bt_packet>,
                                  CommonPacket<bt_packet>>::type;

    using _Stream =
        typename std::conditional<std::is_const<LibObjT>::value, CommonStream<const bt_stream>,
                                  CommonStream<bt_stream>>::type;

    using _StructureField = typename std::conditional<std::is_const<LibObjT>::value,
                                                      ConstStructureField, StructureField>::type;

public:
    using Class = typename std::conditional<std::is_const<LibObjT>::value,
                                            CommonEventClass<const bt_event_class>,
                                            CommonEventClass<bt_event_class>>::type;

    explicit CommonEvent(const _LibObjPtr libObjPtr) noexcept : _ThisBorrowedObj {libObjPtr}
    {
    }

    template <typename OtherLibObjT>
    CommonEvent(const CommonEvent<OtherLibObjT>& event) noexcept : _ThisBorrowedObj {event}
    {
    }

    template <typename OtherLibObjT>
    CommonEvent<LibObjT>& operator=(const CommonEvent<OtherLibObjT>& event) noexcept
    {
        _ThisBorrowedObj::operator=(event);
        return *this;
    }

    CommonEventClass<const bt_event_class> cls() const noexcept;
    Class cls() noexcept;
    CommonStream<const bt_stream> stream() const noexcept;
    _Stream stream() noexcept;
    nonstd::optional<CommonPacket<const bt_packet>> packet() const noexcept;
    nonstd::optional<_Packet> packet() noexcept;

    nonstd::optional<ConstStructureField> payloadField() const noexcept
    {
        const auto libObjPtr = _ConstSpec::payloadField(this->_libObjPtr());

        if (libObjPtr) {
            return ConstStructureField {libObjPtr};
        }

        return nonstd::nullopt;
    }

    nonstd::optional<_StructureField> payloadField() noexcept
    {
        const auto libObjPtr = _Spec::payloadField(this->_libObjPtr());

        if (libObjPtr) {
            return _StructureField {libObjPtr};
        }

        return nonstd::nullopt;
    }

    nonstd::optional<ConstStructureField> specificContextField() const noexcept
    {
        const auto libObjPtr = _ConstSpec::specificContextField(this->_libObjPtr());

        if (libObjPtr) {
            return ConstStructureField {libObjPtr};
        }

        return nonstd::nullopt;
    }

    nonstd::optional<_StructureField> specificContextField() noexcept
    {
        const auto libObjPtr = _Spec::specificContextField(this->_libObjPtr());

        if (libObjPtr) {
            return _StructureField {libObjPtr};
        }

        return nonstd::nullopt;
    }

    nonstd::optional<ConstStructureField> commonContextField() const noexcept
    {
        const auto libObjPtr = _ConstSpec::commonContextField(this->_libObjPtr());

        if (libObjPtr) {
            return ConstStructureField {libObjPtr};
        }

        return nonstd::nullopt;
    }

    nonstd::optional<_StructureField> commonContextField() noexcept
    {
        const auto libObjPtr = _Spec::commonContextField(this->_libObjPtr());

        if (libObjPtr) {
            return _StructureField {libObjPtr};
        }

        return nonstd::nullopt;
    }
};

using Event = CommonEvent<bt_event>;
using ConstEvent = CommonEvent<const bt_event>;

namespace internal {

struct PacketRefFuncs final
{
    static void get(const bt_packet * const libObjPtr)
    {
        bt_packet_get_ref(libObjPtr);
    }

    static void put(const bt_packet * const libObjPtr)
    {
        bt_packet_put_ref(libObjPtr);
    }
};

template <typename LibObjT>
struct CommonPacketSpec;

// Functions specific to mutable packets
template <>
struct CommonPacketSpec<bt_packet> final
{
    static bt_stream *stream(bt_packet * const libObjPtr) noexcept
    {
        return bt_packet_borrow_stream(libObjPtr);
    }

    static bt_field *contextField(bt_packet * const libObjPtr) noexcept
    {
        return bt_packet_borrow_context_field(libObjPtr);
    }
};

// Functions specific to constant packets
template <>
struct CommonPacketSpec<const bt_packet> final
{
    static const bt_stream *stream(const bt_packet * const libObjPtr) noexcept
    {
        return bt_packet_borrow_stream_const(libObjPtr);
    }

    static const bt_field *contextField(const bt_packet * const libObjPtr) noexcept
    {
        return bt_packet_borrow_context_field_const(libObjPtr);
    }
};

} // namespace internal

template <typename LibObjT>
class CommonPacket final : public internal::BorrowedObj<LibObjT>
{
private:
    using typename internal::BorrowedObj<LibObjT>::_ThisBorrowedObj;
    using typename internal::BorrowedObj<LibObjT>::_LibObjPtr;
    using _ConstSpec = internal::CommonPacketSpec<const bt_packet>;
    using _Spec = internal::CommonPacketSpec<LibObjT>;
    using _ThisCommonPacket = CommonPacket<LibObjT>;

    using _Stream =
        typename std::conditional<std::is_const<LibObjT>::value, CommonStream<const bt_stream>,
                                  CommonStream<bt_stream>>::type;

    using _StructureField = typename std::conditional<std::is_const<LibObjT>::value,
                                                      ConstStructureField, StructureField>::type;

public:
    using Shared = internal::SharedObj<_ThisCommonPacket, LibObjT, internal::PacketRefFuncs>;

    explicit CommonPacket(const _LibObjPtr libObjPtr) noexcept : _ThisBorrowedObj {libObjPtr}
    {
    }

    template <typename OtherLibObjT>
    CommonPacket(const CommonPacket<OtherLibObjT>& packet) noexcept : _ThisBorrowedObj {packet}
    {
    }

    template <typename OtherLibObjT>
    _ThisCommonPacket& operator=(const CommonPacket<OtherLibObjT>& packet) noexcept
    {
        _ThisBorrowedObj::operator=(packet);
        return *this;
    }

    CommonStream<const bt_stream> stream() const noexcept;
    _Stream stream() noexcept;

    nonstd::optional<ConstStructureField> contextField() const noexcept
    {
        const auto libObjPtr = _ConstSpec::contextField(this->_libObjPtr());

        if (libObjPtr) {
            return ConstStructureField {libObjPtr};
        }

        return nonstd::nullopt;
    }

    nonstd::optional<_StructureField> contextField() noexcept
    {
        const auto libObjPtr = _Spec::contextField(this->_libObjPtr());

        if (libObjPtr) {
            return _StructureField {libObjPtr};
        }

        return nonstd::nullopt;
    }

    Shared shared() const noexcept
    {
        return Shared {*this};
    }
};

using Packet = CommonPacket<bt_packet>;
using ConstPacket = CommonPacket<const bt_packet>;

template <typename LibObjT>
nonstd::optional<ConstPacket> CommonEvent<LibObjT>::packet() const noexcept
{
    const auto libObjPtr = _ConstSpec::packet(this->_libObjPtr());

    if (libObjPtr) {
        return ConstPacket {libObjPtr};
    }

    return nonstd::nullopt;
}

template <typename LibObjT>
nonstd::optional<typename CommonEvent<LibObjT>::_Packet> CommonEvent<LibObjT>::packet() noexcept
{
    const auto libObjPtr = _Spec::packet(this->_libObjPtr());

    if (libObjPtr) {
        return _Packet {libObjPtr};
    }

    return nonstd::nullopt;
}

namespace internal {

struct StreamRefFuncs final
{
    static void get(const bt_stream * const libObjPtr)
    {
        bt_stream_get_ref(libObjPtr);
    }

    static void put(const bt_stream * const libObjPtr)
    {
        bt_stream_put_ref(libObjPtr);
    }
};

template <typename LibObjT>
struct CommonStreamSpec;

// Functions specific to mutable streams
template <>
struct CommonStreamSpec<bt_stream> final
{
    static bt_stream_class *cls(bt_stream * const libObjPtr) noexcept
    {
        return bt_stream_borrow_class(libObjPtr);
    }

    static bt_trace *trace(bt_stream * const libObjPtr) noexcept
    {
        return bt_stream_borrow_trace(libObjPtr);
    }

    static bt_value *userAttributes(bt_stream * const libObjPtr) noexcept
    {
        return bt_stream_borrow_user_attributes(libObjPtr);
    }
};

// Functions specific to constant streams
template <>
struct CommonStreamSpec<const bt_stream> final
{
    static const bt_stream_class *cls(const bt_stream * const libObjPtr) noexcept
    {
        return bt_stream_borrow_class_const(libObjPtr);
    }

    static const bt_trace *trace(const bt_stream * const libObjPtr) noexcept
    {
        return bt_stream_borrow_trace_const(libObjPtr);
    }

    static const bt_value *userAttributes(const bt_stream * const libObjPtr) noexcept
    {
        return bt_stream_borrow_user_attributes_const(libObjPtr);
    }
};

} // namespace internal

template <typename LibObjT>
class CommonStream final : public internal::BorrowedObj<LibObjT>
{
private:
    using typename internal::BorrowedObj<LibObjT>::_ThisBorrowedObj;
    using typename internal::BorrowedObj<LibObjT>::_LibObjPtr;
    using _ConstSpec = internal::CommonStreamSpec<const bt_stream>;
    using _Spec = internal::CommonStreamSpec<LibObjT>;
    using _ThisCommonStream = CommonStream<LibObjT>;

    using _Trace =
        typename std::conditional<std::is_const<LibObjT>::value, CommonTrace<const bt_trace>,
                                  CommonTrace<bt_trace>>::type;

public:
    using Shared = internal::SharedObj<_ThisCommonStream, LibObjT, internal::StreamRefFuncs>;

    using Class = typename std::conditional<std::is_const<LibObjT>::value,
                                            CommonStreamClass<const bt_stream_class>,
                                            CommonStreamClass<bt_stream_class>>::type;

    using UserAttributes =
        typename std::conditional<std::is_const<LibObjT>::value, ConstMapValue, MapValue>::type;

    explicit CommonStream(const _LibObjPtr libObjPtr) noexcept : _ThisBorrowedObj {libObjPtr}
    {
    }

    template <typename OtherLibObjT>
    CommonStream(const CommonStream<OtherLibObjT>& stream) noexcept : _ThisBorrowedObj {stream}
    {
    }

    template <typename OtherLibObjT>
    _ThisCommonStream& operator=(const CommonStream<OtherLibObjT>& stream) noexcept
    {
        _ThisBorrowedObj::operator=(stream);
        return *this;
    }

    Packet::Shared createPacket()
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_packet_create(this->_libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return Packet::Shared {Packet {libObjPtr}};
    }

    CommonStreamClass<const bt_stream_class> cls() const noexcept;
    Class cls() noexcept;
    CommonTrace<const bt_trace> trace() const noexcept;
    _Trace trace() noexcept;

    std::uint64_t id() const noexcept
    {
        return bt_stream_get_id(this->_libObjPtr());
    }

    void name(const char * const name)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto status = bt_stream_set_name(this->_libObjPtr(), name);

        if (status == BT_STREAM_SET_NAME_STATUS_MEMORY_ERROR) {
            throw LibMemoryError {};
        }
    }

    void name(const std::string& name)
    {
        this->name(name.data());
    }

    nonstd::optional<bpstd::string_view> name() const noexcept
    {
        const auto name = bt_stream_get_name(this->_libObjPtr());

        if (name) {
            return name;
        }

        return nonstd::nullopt;
    }

    template <typename LibValT>
    void userAttributes(const CommonMapValue<LibValT>& userAttrs)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        bt_stream_set_user_attributes(this->_libObjPtr(), userAttrs._libObjPtr());
    }

    ConstMapValue userAttributes() const noexcept
    {
        return ConstMapValue {_ConstSpec::userAttributes(this->_libObjPtr())};
    }

    UserAttributes userAttributes() noexcept
    {
        return UserAttributes {_Spec::userAttributes(this->_libObjPtr())};
    }

    Shared shared() const noexcept
    {
        return Shared {*this};
    }
};

using Stream = CommonStream<bt_stream>;
using ConstStream = CommonStream<const bt_stream>;

template <typename LibObjT>
ConstStream CommonEvent<LibObjT>::stream() const noexcept
{
    return ConstStream {_ConstSpec::stream(this->_libObjPtr())};
}

template <typename LibObjT>
typename CommonEvent<LibObjT>::_Stream CommonEvent<LibObjT>::stream() noexcept
{
    return _Stream {_Spec::stream(this->_libObjPtr())};
}

template <typename LibObjT>
ConstStream CommonPacket<LibObjT>::stream() const noexcept
{
    return ConstStream {_ConstSpec::stream(this->_libObjPtr())};
}

template <typename LibObjT>
typename CommonPacket<LibObjT>::_Stream CommonPacket<LibObjT>::stream() noexcept
{
    return _Stream {_Spec::stream(this->_libObjPtr())};
}

namespace internal {

struct TraceRefFuncs final
{
    static void get(const bt_trace * const libObjPtr)
    {
        bt_trace_get_ref(libObjPtr);
    }

    static void put(const bt_trace * const libObjPtr)
    {
        bt_trace_put_ref(libObjPtr);
    }
};

template <typename LibObjT>
struct CommonTraceSpec;

// Functions specific to mutable traces
template <>
struct CommonTraceSpec<bt_trace> final
{
    static bt_trace_class *cls(bt_trace * const libObjPtr) noexcept
    {
        return bt_trace_borrow_class(libObjPtr);
    }

    static bt_stream *streamByIndex(bt_trace * const libObjPtr, const std::uint64_t index) noexcept
    {
        return bt_trace_borrow_stream_by_index(libObjPtr, index);
    }

    static bt_stream *streamById(bt_trace * const libObjPtr, const std::uint64_t id) noexcept
    {
        return bt_trace_borrow_stream_by_id(libObjPtr, id);
    }

    static bt_value *userAttributes(bt_trace * const libObjPtr) noexcept
    {
        return bt_trace_borrow_user_attributes(libObjPtr);
    }
};

// Functions specific to constant traces
template <>
struct CommonTraceSpec<const bt_trace> final
{
    static const bt_trace_class *cls(const bt_trace * const libObjPtr) noexcept
    {
        return bt_trace_borrow_class_const(libObjPtr);
    }

    static const bt_stream *streamByIndex(const bt_trace * const libObjPtr,
                                          const std::uint64_t index) noexcept
    {
        return bt_trace_borrow_stream_by_index_const(libObjPtr, index);
    }

    static const bt_stream *streamById(const bt_trace * const libObjPtr,
                                       const std::uint64_t id) noexcept
    {
        return bt_trace_borrow_stream_by_id_const(libObjPtr, id);
    }

    static const bt_value *userAttributes(const bt_trace * const libObjPtr) noexcept
    {
        return bt_trace_borrow_user_attributes_const(libObjPtr);
    }
};

} // namespace internal

template <typename LibObjT>
class CommonTrace final : public internal::BorrowedObj<LibObjT>
{
    // Allow instantiate() to call `trace._libObjPtr()`
    friend class CommonStreamClass<bt_stream_class>;

private:
    using typename internal::BorrowedObj<LibObjT>::_ThisBorrowedObj;
    using typename internal::BorrowedObj<LibObjT>::_LibObjPtr;
    using _ConstSpec = internal::CommonTraceSpec<const bt_trace>;
    using _Spec = internal::CommonTraceSpec<LibObjT>;
    using _ThisCommonTrace = CommonTrace<LibObjT>;

    using _Stream =
        typename std::conditional<std::is_const<LibObjT>::value, CommonStream<const bt_stream>,
                                  CommonStream<bt_stream>>::type;

public:
    using Shared = internal::SharedObj<_ThisCommonTrace, LibObjT, internal::TraceRefFuncs>;

    using Class = typename std::conditional<std::is_const<LibObjT>::value,
                                            CommonTraceClass<const bt_trace_class>,
                                            CommonTraceClass<bt_trace_class>>::type;

    using UserAttributes =
        typename std::conditional<std::is_const<LibObjT>::value, ConstMapValue, MapValue>::type;

    struct ConstEnvironmentEntry
    {
        bpstd::string_view name;
        ConstValue value;
    };

    explicit CommonTrace(const _LibObjPtr libObjPtr) noexcept : _ThisBorrowedObj {libObjPtr}
    {
    }

    template <typename OtherLibObjT>
    CommonTrace(const CommonTrace<OtherLibObjT>& trace) noexcept : _ThisBorrowedObj {trace}
    {
    }

    template <typename OtherLibObjT>
    _ThisCommonTrace& operator=(const CommonTrace<OtherLibObjT>& trace) noexcept
    {
        _ThisBorrowedObj::operator=(trace);
        return *this;
    }

    CommonTraceClass<const bt_trace_class> cls() const noexcept;
    Class cls() noexcept;

    void name(const char * const name)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto status = bt_trace_set_name(this->_libObjPtr(), name);

        if (status == BT_TRACE_SET_NAME_STATUS_MEMORY_ERROR) {
            throw LibMemoryError {};
        }
    }

    void name(const std::string& name)
    {
        this->name(name.data());
    }

    nonstd::optional<bpstd::string_view> name() const noexcept
    {
        const auto name = bt_trace_get_name(this->_libObjPtr());

        if (name) {
            return name;
        }

        return nonstd::nullopt;
    }

    void uuid(const std::uint8_t * const uuid) noexcept
    {
        bt_trace_set_uuid(this->_libObjPtr(), uuid);
    }

    nonstd::optional<bt2_common::UuidView> uuid() const noexcept
    {
        const auto uuid = bt_trace_get_uuid(this->_libObjPtr());

        if (uuid) {
            return bt2_common::UuidView {uuid};
        }

        return nonstd::nullopt;
    }

    std::uint64_t size() const noexcept
    {
        return bt_trace_get_stream_count(this->_libObjPtr());
    }

    ConstStream operator[](const std::uint64_t index) const noexcept
    {
        return ConstStream {_ConstSpec::streamByIndex(this->_libObjPtr(), index)};
    }

    _Stream operator[](const std::uint64_t index) noexcept
    {
        return _Stream {_Spec::streamByIndex(this->_libObjPtr(), index)};
    }

    nonstd::optional<ConstStream> streamById(const std::uint64_t id) const noexcept
    {
        const auto libObjPtr = _ConstSpec::streamById(this->_libObjPtr(), id);

        if (libObjPtr) {
            return ConstStream {libObjPtr};
        }

        return nonstd::nullopt;
    }

    nonstd::optional<_Stream> streamById(const std::uint64_t id) noexcept
    {
        const auto libObjPtr = _Spec::streamById(this->_libObjPtr(), id);

        if (libObjPtr) {
            return _Stream {libObjPtr};
        }

        return nonstd::nullopt;
    }

    void environmentEntry(const char * const name, const std::int64_t val)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto status = bt_trace_set_environment_entry_integer(this->_libObjPtr(), name, val);

        if (status == BT_TRACE_SET_ENVIRONMENT_ENTRY_STATUS_MEMORY_ERROR) {
            throw LibMemoryError {};
        }
    }

    void environmentEntry(const std::string& name, const std::int64_t val)
    {
        this->environmentEntry(name.data(), val);
    }

    void environmentEntry(const char * const name, const char * const val)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto status = bt_trace_set_environment_entry_string(this->_libObjPtr(), name, val);

        if (status == BT_TRACE_SET_ENVIRONMENT_ENTRY_STATUS_MEMORY_ERROR) {
            throw LibMemoryError {};
        }
    }

    void environmentEntry(const std::string& name, const char * const val)
    {
        this->environmentEntry(name.data(), val);
    }

    void environmentEntry(const char * const name, const std::string& val)
    {
        this->environmentEntry(name, val.data());
    }

    void environmentEntry(const std::string& name, const std::string& val)
    {
        this->environmentEntry(name.data(), val.data());
    }

    std::uint64_t environmentSize() const noexcept
    {
        return bt_trace_get_environment_entry_count(this->_libObjPtr());
    }

    ConstEnvironmentEntry environmentEntry(const std::uint64_t index) const noexcept
    {
        const char *name;
        const bt_value *libObjPtr;

        bt_trace_borrow_environment_entry_by_index_const(this->_libObjPtr(), index, &name,
                                                         &libObjPtr);
        return ConstEnvironmentEntry {name, ConstValue {libObjPtr}};
    }

    nonstd::optional<ConstValue> environmentEntry(const char * const name) const noexcept
    {
        const auto libObjPtr =
            bt_trace_borrow_environment_entry_value_by_name_const(this->_libObjPtr(), name);

        if (libObjPtr) {
            return ConstValue {libObjPtr};
        }

        return nonstd::nullopt;
    }

    nonstd::optional<ConstValue> environmentEntry(const std::string& name) const noexcept
    {
        return this->environmentEntry(name.data());
    }

    template <typename LibValT>
    void userAttributes(const CommonMapValue<LibValT>& userAttrs)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        bt_trace_set_user_attributes(this->_libObjPtr(), userAttrs._libObjPtr());
    }

    ConstMapValue userAttributes() const noexcept
    {
        return ConstMapValue {_ConstSpec::userAttributes(this->_libObjPtr())};
    }

    UserAttributes userAttributes() noexcept
    {
        return UserAttributes {_Spec::userAttributes(this->_libObjPtr())};
    }

    Shared shared() const noexcept
    {
        return Shared {*this};
    }
};

using Trace = CommonTrace<bt_trace>;
using ConstTrace = CommonTrace<const bt_trace>;

template <typename LibObjT>
ConstTrace CommonStream<LibObjT>::trace() const noexcept
{
    return ConstTrace {_ConstSpec::trace(this->_libObjPtr())};
}

template <typename LibObjT>
typename CommonStream<LibObjT>::_Trace CommonStream<LibObjT>::trace() noexcept
{
    return _Trace {_Spec::trace(this->_libObjPtr())};
}

namespace internal {

struct EventClassRefFuncs final
{
    static void get(const bt_event_class * const libObjPtr)
    {
        bt_event_class_get_ref(libObjPtr);
    }

    static void put(const bt_event_class * const libObjPtr)
    {
        bt_event_class_put_ref(libObjPtr);
    }
};

template <typename LibObjT>
struct CommonEventClassSpec;

// Functions specific to mutable event classes
template <>
struct CommonEventClassSpec<bt_event_class> final
{
    static bt_stream_class *streamClass(bt_event_class * const libObjPtr) noexcept
    {
        return bt_event_class_borrow_stream_class(libObjPtr);
    }

    static bt_field_class *payloadFieldClass(bt_event_class * const libObjPtr) noexcept
    {
        return bt_event_class_borrow_payload_field_class(libObjPtr);
    }

    static bt_field_class *specificContextFieldClass(bt_event_class * const libObjPtr) noexcept
    {
        return bt_event_class_borrow_specific_context_field_class(libObjPtr);
    }

    static bt_value *userAttributes(bt_event_class * const libObjPtr) noexcept
    {
        return bt_event_class_borrow_user_attributes(libObjPtr);
    }
};

// Functions specific to constant event classes
template <>
struct CommonEventClassSpec<const bt_event_class> final
{
    static const bt_stream_class *streamClass(const bt_event_class * const libObjPtr) noexcept
    {
        return bt_event_class_borrow_stream_class_const(libObjPtr);
    }

    static const bt_field_class *payloadFieldClass(const bt_event_class * const libObjPtr) noexcept
    {
        return bt_event_class_borrow_payload_field_class_const(libObjPtr);
    }

    static const bt_field_class *
    specificContextFieldClass(const bt_event_class * const libObjPtr) noexcept
    {
        return bt_event_class_borrow_specific_context_field_class_const(libObjPtr);
    }

    static const bt_value *userAttributes(const bt_event_class * const libObjPtr) noexcept
    {
        return bt_event_class_borrow_user_attributes_const(libObjPtr);
    }
};

} // namespace internal

template <typename LibObjT>
class CommonEventClass final : public internal::BorrowedObj<LibObjT>
{
private:
    using typename internal::BorrowedObj<LibObjT>::_ThisBorrowedObj;
    using typename internal::BorrowedObj<LibObjT>::_LibObjPtr;
    using _ConstSpec = internal::CommonEventClassSpec<const bt_event_class>;
    using _Spec = internal::CommonEventClassSpec<LibObjT>;
    using _ThisCommonEventClass = CommonEventClass<LibObjT>;

    using _StreamClass = typename std::conditional<std::is_const<LibObjT>::value,
                                                   CommonStreamClass<const bt_stream_class>,
                                                   CommonStreamClass<bt_stream_class>>::type;

    using _StructureFieldClass =
        typename std::conditional<std::is_const<LibObjT>::value, ConstStructureFieldClass,
                                  StructureFieldClass>::type;

public:
    using Shared =
        internal::SharedObj<_ThisCommonEventClass, LibObjT, internal::EventClassRefFuncs>;

    using UserAttributes =
        typename std::conditional<std::is_const<LibObjT>::value, ConstMapValue, MapValue>::type;

    enum class LogLevel
    {
        EMERGENCY = BT_EVENT_CLASS_LOG_LEVEL_EMERGENCY,
        ALERT = BT_EVENT_CLASS_LOG_LEVEL_ALERT,
        CRITICAL = BT_EVENT_CLASS_LOG_LEVEL_CRITICAL,
        ERR = BT_EVENT_CLASS_LOG_LEVEL_ERROR,
        WARNING = BT_EVENT_CLASS_LOG_LEVEL_WARNING,
        NOTICE = BT_EVENT_CLASS_LOG_LEVEL_NOTICE,
        INFO = BT_EVENT_CLASS_LOG_LEVEL_INFO,
        DEBUG_SYSTEM = BT_EVENT_CLASS_LOG_LEVEL_DEBUG_SYSTEM,
        DEBUG_PROGRAM = BT_EVENT_CLASS_LOG_LEVEL_DEBUG_PROGRAM,
        DEBUG_PROC = BT_EVENT_CLASS_LOG_LEVEL_DEBUG_PROCESS,
        DEBUG_MODULE = BT_EVENT_CLASS_LOG_LEVEL_DEBUG_MODULE,
        DEBUG_UNIT = BT_EVENT_CLASS_LOG_LEVEL_DEBUG_UNIT,
        DEBUG_FUNCTION = BT_EVENT_CLASS_LOG_LEVEL_DEBUG_FUNCTION,
        DEBUG_LINE = BT_EVENT_CLASS_LOG_LEVEL_DEBUG_LINE,
        DEBUG = BT_EVENT_CLASS_LOG_LEVEL_DEBUG,
    };

    explicit CommonEventClass(const _LibObjPtr libObjPtr) noexcept : _ThisBorrowedObj {libObjPtr}
    {
    }

    template <typename OtherLibObjT>
    CommonEventClass(const CommonEventClass<OtherLibObjT>& eventClass) noexcept :
        _ThisBorrowedObj {eventClass}
    {
    }

    template <typename OtherLibObjT>
    _ThisCommonEventClass& operator=(const CommonEventClass<OtherLibObjT>& eventClass) noexcept
    {
        _ThisBorrowedObj::operator=(eventClass);
        return *this;
    }

    CommonStreamClass<const bt_stream_class> streamClass() const noexcept;
    _StreamClass streamClass() noexcept;

    std::uint64_t id() const noexcept
    {
        return bt_event_class_get_id(this->_libObjPtr());
    }

    void name(const char * const name)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto status = bt_event_class_set_name(this->_libObjPtr(), name);

        if (status == BT_EVENT_CLASS_SET_NAME_STATUS_MEMORY_ERROR) {
            throw LibMemoryError {};
        }
    }

    void name(const std::string& name)
    {
        this->name(name.data());
    }

    nonstd::optional<bpstd::string_view> name() const noexcept
    {
        const auto name = bt_event_class_get_name(this->_libObjPtr());

        if (name) {
            return name;
        }

        return nonstd::nullopt;
    }

    void logLevel(const LogLevel logLevel) noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        bt_event_class_set_log_level(this->_libObjPtr(),
                                     static_cast<bt_event_class_log_level>(logLevel));
    }

    nonstd::optional<LogLevel> logLevel() const noexcept
    {
        bt_event_class_log_level libLogLevel;
        const auto avail = bt_event_class_get_log_level(this->_libObjPtr(), &libLogLevel);

        if (avail == BT_PROPERTY_AVAILABILITY_AVAILABLE) {
            return static_cast<LogLevel>(libLogLevel);
        }

        return nonstd::nullopt;
    }

    void emfUri(const char * const emfUri)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto status = bt_event_class_set_emf_uri(this->_libObjPtr(), emfUri);

        if (status == BT_EVENT_CLASS_SET_EMF_URI_STATUS_MEMORY_ERROR) {
            throw LibMemoryError {};
        }
    }

    void emfUri(const std::string& emfUri)
    {
        this->emfUri(emfUri.data());
    }

    nonstd::optional<bpstd::string_view> emfUri() const noexcept
    {
        const auto emfUri = bt_event_class_get_emf_uri(this->_libObjPtr());

        if (emfUri) {
            return emfUri;
        }

        return nonstd::nullopt;
    }

    void payloadFieldClass(const StructureFieldClass& fc)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto status =
            bt_event_class_set_payload_field_class(this->_libObjPtr(), fc._libObjPtr());

        if (status == BT_EVENT_CLASS_SET_FIELD_CLASS_STATUS_MEMORY_ERROR) {
            throw LibMemoryError {};
        }
    }

    nonstd::optional<ConstStructureFieldClass> payloadFieldClass() const noexcept
    {
        const auto libObjPtr = _ConstSpec::payloadFieldClass(this->_libObjPtr());

        if (libObjPtr) {
            return ConstStructureFieldClass {libObjPtr};
        }

        return nonstd::nullopt;
    }

    nonstd::optional<_StructureFieldClass> payloadFieldClass() noexcept
    {
        const auto libObjPtr = _Spec::payloadFieldClass(this->_libObjPtr());

        if (libObjPtr) {
            return _StructureFieldClass {libObjPtr};
        }

        return nonstd::nullopt;
    }

    void specificContextFieldClass(const StructureFieldClass& fc)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto status =
            bt_event_class_set_specific_context_field_class(this->_libObjPtr(), fc._libObjPtr());

        if (status == BT_EVENT_CLASS_SET_FIELD_CLASS_STATUS_MEMORY_ERROR) {
            throw LibMemoryError {};
        }
    }

    nonstd::optional<ConstStructureFieldClass> specificContextFieldClass() const noexcept
    {
        const auto libObjPtr = _ConstSpec::specificContextFieldClass(this->_libObjPtr());

        if (libObjPtr) {
            return ConstStructureFieldClass {libObjPtr};
        }

        return nonstd::nullopt;
    }

    nonstd::optional<_StructureFieldClass> specificContextFieldClass() noexcept
    {
        const auto libObjPtr = _Spec::specificContextFieldClass(this->_libObjPtr());

        if (libObjPtr) {
            return _StructureFieldClass {libObjPtr};
        }

        return nonstd::nullopt;
    }

    template <typename LibValT>
    void userAttributes(const CommonMapValue<LibValT>& userAttrs)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        bt_event_class_set_user_attributes(this->_libObjPtr(), userAttrs._libObjPtr());
    }

    ConstMapValue userAttributes() const noexcept
    {
        return ConstMapValue {_ConstSpec::userAttributes(this->_libObjPtr())};
    }

    UserAttributes userAttributes() noexcept
    {
        return UserAttributes {_Spec::userAttributes(this->_libObjPtr())};
    }

    Shared shared() const noexcept
    {
        return Shared {*this};
    }
};

using EventClass = CommonEventClass<bt_event_class>;
using ConstEventClass = CommonEventClass<const bt_event_class>;

template <typename LibObjT>
ConstEventClass CommonEvent<LibObjT>::cls() const noexcept
{
    return ConstEventClass {_ConstSpec::cls(this->_libObjPtr())};
}

template <typename LibObjT>
typename CommonEvent<LibObjT>::Class CommonEvent<LibObjT>::cls() noexcept
{
    return Class {_Spec::cls(this->_libObjPtr())};
}

namespace internal {

struct StreamClassRefFuncs final
{
    static void get(const bt_stream_class * const libObjPtr)
    {
        bt_stream_class_get_ref(libObjPtr);
    }

    static void put(const bt_stream_class * const libObjPtr)
    {
        bt_stream_class_put_ref(libObjPtr);
    }
};

template <typename LibObjT>
struct CommonStreamClassSpec;

// Functions specific to mutable stream classes
template <>
struct CommonStreamClassSpec<bt_stream_class> final
{
    static bt_trace_class *traceClass(bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_trace_class(libObjPtr);
    }

    static bt_event_class *eventClassByIndex(bt_stream_class * const libObjPtr,
                                             const std::uint64_t index) noexcept
    {
        return bt_stream_class_borrow_event_class_by_index(libObjPtr, index);
    }

    static bt_event_class *eventClassById(bt_stream_class * const libObjPtr,
                                          const std::uint64_t id) noexcept
    {
        return bt_stream_class_borrow_event_class_by_id(libObjPtr, id);
    }

    static bt_clock_class *defaultClockClass(bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_default_clock_class(libObjPtr);
    }

    static bt_field_class *packetContextFieldClass(bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_packet_context_field_class(libObjPtr);
    }

    static bt_field_class *eventCommonContextFieldClass(bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_event_common_context_field_class(libObjPtr);
    }

    static bt_value *userAttributes(bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_user_attributes(libObjPtr);
    }
};

// Functions specific to constant stream classes
template <>
struct CommonStreamClassSpec<const bt_stream_class> final
{
    static const bt_trace_class *traceClass(const bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_trace_class_const(libObjPtr);
    }

    static const bt_event_class *eventClassByIndex(const bt_stream_class * const libObjPtr,
                                                   const std::uint64_t index) noexcept
    {
        return bt_stream_class_borrow_event_class_by_index_const(libObjPtr, index);
    }

    static const bt_event_class *eventClassById(const bt_stream_class * const libObjPtr,
                                                const std::uint64_t id) noexcept
    {
        return bt_stream_class_borrow_event_class_by_id_const(libObjPtr, id);
    }

    static const bt_clock_class *defaultClockClass(const bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_default_clock_class_const(libObjPtr);
    }

    static const bt_field_class *
    packetContextFieldClass(const bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_packet_context_field_class_const(libObjPtr);
    }

    static const bt_field_class *
    eventCommonContextFieldClass(const bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_event_common_context_field_class_const(libObjPtr);
    }

    static const bt_value *userAttributes(const bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_user_attributes_const(libObjPtr);
    }
};

} // namespace internal

template <typename LibObjT>
class CommonStreamClass final : public internal::BorrowedObj<LibObjT>
{
private:
    using typename internal::BorrowedObj<LibObjT>::_ThisBorrowedObj;
    using typename internal::BorrowedObj<LibObjT>::_LibObjPtr;
    using _ConstSpec = internal::CommonStreamClassSpec<const bt_stream_class>;
    using _Spec = internal::CommonStreamClassSpec<LibObjT>;
    using _ThisCommonStreamClass = CommonStreamClass<LibObjT>;

    using _TraceClass = typename std::conditional<std::is_const<LibObjT>::value,
                                                  CommonTraceClass<const bt_trace_class>,
                                                  CommonTraceClass<bt_trace_class>>::type;

    using _EventClass = typename std::conditional<std::is_const<LibObjT>::value,
                                                  CommonEventClass<const bt_event_class>,
                                                  CommonEventClass<bt_event_class>>::type;

    using _StructureFieldClass =
        typename std::conditional<std::is_const<LibObjT>::value, ConstStructureFieldClass,
                                  StructureFieldClass>::type;

    using _ClockClass =
        typename std::conditional<std::is_const<LibObjT>::value, ConstClockClass, ClockClass>::type;

public:
    using Shared =
        internal::SharedObj<_ThisCommonStreamClass, LibObjT, internal::StreamClassRefFuncs>;

    using UserAttributes =
        typename std::conditional<std::is_const<LibObjT>::value, ConstMapValue, MapValue>::type;

    explicit CommonStreamClass(const _LibObjPtr libObjPtr) noexcept : _ThisBorrowedObj {libObjPtr}
    {
    }

    template <typename OtherLibObjT>
    CommonStreamClass(const CommonStreamClass<OtherLibObjT>& streamClass) noexcept :
        _ThisBorrowedObj {streamClass}
    {
    }

    template <typename OtherLibObjT>
    _ThisCommonStreamClass& operator=(const CommonStreamClass<OtherLibObjT>& streamClass) noexcept
    {
        _ThisBorrowedObj::operator=(streamClass);
        return *this;
    }

    Stream::Shared instantiate(const Trace& trace)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_stream_create(this->_libObjPtr(), trace._libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return Stream::Shared {Stream {libObjPtr}};
    }

    Stream::Shared instantiate(const Trace& trace, const std::uint64_t id)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_stream_create_with_id(this->_libObjPtr(), trace._libObjPtr(), id);

        internal::validateCreatedObjPtr(libObjPtr);
        return Stream::Shared {Stream {libObjPtr}};
    }

    EventClass::Shared createEventClass()
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_event_class_create(this->_libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return EventClass::Shared {EventClass {libObjPtr}};
    }

    EventClass::Shared createEventClass(const std::uint64_t id)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_event_class_create_with_id(this->_libObjPtr(), id);

        internal::validateCreatedObjPtr(libObjPtr);
        return EventClass::Shared {EventClass {libObjPtr}};
    }

    CommonTraceClass<const bt_trace_class> traceClass() const noexcept;
    _TraceClass traceClass() noexcept;

    std::uint64_t id() const noexcept
    {
        return bt_stream_class_get_id(this->_libObjPtr());
    }

    void name(const char * const name)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto status = bt_stream_class_set_name(this->_libObjPtr(), name);

        if (status == BT_STREAM_CLASS_SET_NAME_STATUS_MEMORY_ERROR) {
            throw LibMemoryError {};
        }
    }

    void name(const std::string& name)
    {
        this->name(name.data());
    }

    nonstd::optional<bpstd::string_view> name() const noexcept
    {
        const auto name = bt_stream_class_get_name(this->_libObjPtr());

        if (name) {
            return name;
        }

        return nonstd::nullopt;
    }

    void assignsAutomaticEventClassId(const bool val) noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        bt_stream_class_set_assigns_automatic_event_class_id(this->_libObjPtr(),
                                                             static_cast<bt_bool>(val));
    }

    bool assignsAutomaticEventClassId() const noexcept
    {
        return static_cast<bool>(
            bt_stream_class_assigns_automatic_event_class_id(this->_libObjPtr()));
    }

    void assignsAutomaticStreamId(const bool val) noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        bt_stream_class_set_assigns_automatic_stream_id(this->_libObjPtr(),
                                                        static_cast<bt_bool>(val));
    }

    bool assignsAutomaticStreamId() const noexcept
    {
        return static_cast<bool>(bt_stream_class_assigns_automatic_stream_id(this->_libObjPtr()));
    }

    void supportsPackets(const bool supportsPackets, const bool withBeginningDefaultClkSnapshot,
                         const bool withEndDefaultClkSnapshot) noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        bt_stream_class_set_supports_packets(this->_libObjPtr(),
                                             static_cast<bt_bool>(supportsPackets),
                                             static_cast<bt_bool>(withBeginningDefaultClkSnapshot),
                                             static_cast<bt_bool>(withEndDefaultClkSnapshot));
    }

    bool supportsPackets() const noexcept
    {
        return static_cast<bool>(bt_stream_class_supports_packets(this->_libObjPtr()));
    }

    bool packetsHaveBeginningClockSnapshot() const noexcept
    {
        return static_cast<bool>(
            bt_stream_class_packets_have_beginning_default_clock_snapshot(this->_libObjPtr()));
    }

    bool packetsHaveEndClockSnapshot() const noexcept
    {
        return static_cast<bool>(
            bt_stream_class_packets_have_end_default_clock_snapshot(this->_libObjPtr()));
    }

    void supportsDiscardedEvents(const bool supportsDiscardedEvents,
                                 const bool withDefaultClkSnapshots) noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        bt_stream_class_set_supports_discarded_events(
            this->_libObjPtr(), static_cast<bt_bool>(supportsPackets),
            static_cast<bt_bool>(withDefaultClkSnapshots));
    }

    bool supportsDiscardedEvents() const noexcept
    {
        return static_cast<bool>(bt_stream_class_supports_discarded_events(this->_libObjPtr()));
    }

    bool discardedEventsHaveDefaultClockSnapshots() const noexcept
    {
        return static_cast<bool>(
            bt_stream_class_discarded_events_have_default_clock_snapshots(this->_libObjPtr()));
    }

    void supportsDiscardedPackets(const bool supportsDiscardedPackets,
                                  const bool withDefaultClkSnapshots) noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        bt_stream_class_set_supports_discarded_packets(
            this->_libObjPtr(), static_cast<bt_bool>(supportsPackets),
            static_cast<bt_bool>(withDefaultClkSnapshots));
    }

    bool supportsDiscardedPackets() const noexcept
    {
        return static_cast<bool>(bt_stream_class_supports_discarded_packets(this->_libObjPtr()));
    }

    bool discardedPacketsHaveDefaultClockSnapshots() const noexcept
    {
        return static_cast<bool>(
            bt_stream_class_discarded_packets_have_default_clock_snapshots(this->_libObjPtr()));
    }

    void defaultClockClass(const ClockClass& clkCls)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto status =
            bt_stream_class_set_default_clock_class(this->_libObjPtr(), clkCls._libObjPtr());

        BT_ASSERT(status == BT_STREAM_CLASS_SET_DEFAULT_CLOCK_CLASS_STATUS_OK);
    }

    nonstd::optional<ConstClockClass> defaultClockClass() const noexcept
    {
        const auto libObjPtr = _ConstSpec::defaultClockClass(this->_libObjPtr());

        if (libObjPtr) {
            return ConstClockClass {libObjPtr};
        }

        return nonstd::nullopt;
    }

    nonstd::optional<_ClockClass> defaultClockClass() noexcept
    {
        const auto libObjPtr = _Spec::defaultClockClass(this->_libObjPtr());

        if (libObjPtr) {
            return _ClockClass {libObjPtr};
        }

        return nonstd::nullopt;
    }

    std::uint64_t size() const noexcept
    {
        return bt_stream_class_get_event_class_count(this->_libObjPtr());
    }

    ConstEventClass operator[](const std::uint64_t index) const noexcept
    {
        return ConstEventClass {_ConstSpec::eventClassByIndex(this->_libObjPtr(), index)};
    }

    _EventClass operator[](const std::uint64_t index) noexcept
    {
        return _EventClass {_Spec::eventClassByIndex(this->_libObjPtr(), index)};
    }

    nonstd::optional<ConstEventClass> eventClassById(const std::uint64_t id) const noexcept
    {
        const auto libObjPtr = _ConstSpec::eventClassById(this->_libObjPtr(), id);

        if (libObjPtr) {
            return ConstEventClass {libObjPtr};
        }

        return nonstd::nullopt;
    }

    nonstd::optional<_EventClass> eventClassById(const std::uint64_t id) noexcept
    {
        const auto libObjPtr = _Spec::eventClassById(this->_libObjPtr(), id);

        if (libObjPtr) {
            return _EventClass {libObjPtr};
        }

        return nonstd::nullopt;
    }

    void packetContextFieldClass(const StructureFieldClass& fc)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto status =
            bt_stream_class_set_packet_context_field_class(this->_libObjPtr(), fc._libObjPtr());

        if (status == BT_EVENT_CLASS_SET_FIELD_CLASS_STATUS_MEMORY_ERROR) {
            throw LibMemoryError {};
        }
    }

    nonstd::optional<ConstStructureFieldClass> packetContextFieldClass() const noexcept
    {
        const auto libObjPtr = _ConstSpec::packetContextFieldClass(this->_libObjPtr());

        if (libObjPtr) {
            return ConstStructureFieldClass {libObjPtr};
        }

        return nonstd::nullopt;
    }

    nonstd::optional<_StructureFieldClass> packetContextFieldClass() noexcept
    {
        const auto libObjPtr = _Spec::packetContextFieldClass(this->_libObjPtr());

        if (libObjPtr) {
            return _StructureFieldClass {libObjPtr};
        }

        return nonstd::nullopt;
    }

    void eventCommonContextFieldClass(const StructureFieldClass& fc)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto status = bt_stream_class_set_event_common_context_field_class(this->_libObjPtr(),
                                                                                 fc._libObjPtr());

        if (status == BT_EVENT_CLASS_SET_FIELD_CLASS_STATUS_MEMORY_ERROR) {
            throw LibMemoryError {};
        }
    }

    nonstd::optional<ConstStructureFieldClass> eventCommonContextFieldClass() const noexcept
    {
        const auto libObjPtr = _ConstSpec::eventCommonContextFieldClass(this->_libObjPtr());

        if (libObjPtr) {
            return ConstStructureFieldClass {libObjPtr};
        }

        return nonstd::nullopt;
    }

    nonstd::optional<_StructureFieldClass> eventCommonContextFieldClass() noexcept
    {
        const auto libObjPtr = _Spec::eventCommonContextFieldClass(this->_libObjPtr());

        if (libObjPtr) {
            return _StructureFieldClass {libObjPtr};
        }

        return nonstd::nullopt;
    }

    template <typename LibValT>
    void userAttributes(const CommonMapValue<LibValT>& userAttrs)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        bt_stream_class_set_user_attributes(this->_libObjPtr(), userAttrs._libObjPtr());
    }

    ConstMapValue userAttributes() const noexcept
    {
        return ConstMapValue {_ConstSpec::userAttributes(this->_libObjPtr())};
    }

    UserAttributes userAttributes() noexcept
    {
        return UserAttributes {_Spec::userAttributes(this->_libObjPtr())};
    }

    Shared shared() const noexcept
    {
        return Shared {*this};
    }
};

using StreamClass = CommonStreamClass<bt_stream_class>;
using ConstStreamClass = CommonStreamClass<const bt_stream_class>;

template <typename LibObjT>
ConstStreamClass CommonEventClass<LibObjT>::streamClass() const noexcept
{
    return ConstStreamClass {_ConstSpec::streamClass(this->_libObjPtr())};
}

template <typename LibObjT>
typename CommonEventClass<LibObjT>::_StreamClass CommonEventClass<LibObjT>::streamClass() noexcept
{
    return _StreamClass {_Spec::streamClass(this->_libObjPtr())};
}

template <typename LibObjT>
ConstStreamClass CommonStream<LibObjT>::cls() const noexcept
{
    return ConstStreamClass {_ConstSpec::cls(this->_libObjPtr())};
}

template <typename LibObjT>
typename CommonStream<LibObjT>::Class CommonStream<LibObjT>::cls() noexcept
{
    return Class {_Spec::cls(this->_libObjPtr())};
}

namespace internal {

struct TraceClassRefFuncs final
{
    static void get(const bt_trace_class * const libObjPtr)
    {
        bt_trace_class_get_ref(libObjPtr);
    }

    static void put(const bt_trace_class * const libObjPtr)
    {
        bt_trace_class_put_ref(libObjPtr);
    }
};

template <typename LibObjT>
struct CommonTraceClassSpec;

// Functions specific to mutable stream classes
template <>
struct CommonTraceClassSpec<bt_trace_class> final
{
    static bt_stream_class *streamClassByIndex(bt_trace_class * const libObjPtr,
                                               const std::uint64_t index) noexcept
    {
        return bt_trace_class_borrow_stream_class_by_index(libObjPtr, index);
    }

    static bt_stream_class *streamClassById(bt_trace_class * const libObjPtr,
                                            const std::uint64_t id) noexcept
    {
        return bt_trace_class_borrow_stream_class_by_id(libObjPtr, id);
    }

    static bt_value *userAttributes(bt_trace_class * const libObjPtr) noexcept
    {
        return bt_trace_class_borrow_user_attributes(libObjPtr);
    }
};

// Functions specific to constant stream classes
template <>
struct CommonTraceClassSpec<const bt_trace_class> final
{
    static const bt_stream_class *streamClassByIndex(const bt_trace_class * const libObjPtr,
                                                     const std::uint64_t index) noexcept
    {
        return bt_trace_class_borrow_stream_class_by_index_const(libObjPtr, index);
    }

    static const bt_stream_class *streamClassById(const bt_trace_class * const libObjPtr,
                                                  const std::uint64_t id) noexcept
    {
        return bt_trace_class_borrow_stream_class_by_id_const(libObjPtr, id);
    }

    static const bt_value *userAttributes(const bt_trace_class * const libObjPtr) noexcept
    {
        return bt_trace_class_borrow_user_attributes_const(libObjPtr);
    }
};

} // namespace internal

template <typename LibObjT>
class CommonTraceClass final : public internal::BorrowedObj<LibObjT>
{
private:
    using typename internal::BorrowedObj<LibObjT>::_ThisBorrowedObj;
    using typename internal::BorrowedObj<LibObjT>::_LibObjPtr;
    using _ConstSpec = internal::CommonTraceClassSpec<const bt_trace_class>;
    using _Spec = internal::CommonTraceClassSpec<LibObjT>;
    using _ThisCommonTraceClass = CommonTraceClass<LibObjT>;

    using _StreamClass = typename std::conditional<std::is_const<LibObjT>::value,
                                                   CommonStreamClass<const bt_stream_class>,
                                                   CommonStreamClass<bt_stream_class>>::type;

public:
    using Shared =
        internal::SharedObj<_ThisCommonTraceClass, LibObjT, internal::TraceClassRefFuncs>;

    using UserAttributes =
        typename std::conditional<std::is_const<LibObjT>::value, ConstMapValue, MapValue>::type;

    explicit CommonTraceClass(const _LibObjPtr libObjPtr) noexcept : _ThisBorrowedObj {libObjPtr}
    {
    }

    template <typename OtherLibObjT>
    CommonTraceClass(const CommonTraceClass<OtherLibObjT>& traceClass) noexcept :
        _ThisBorrowedObj {traceClass}
    {
    }

    template <typename OtherLibObjT>
    _ThisCommonTraceClass& operator=(const CommonTraceClass<OtherLibObjT>& traceClass) noexcept
    {
        _ThisBorrowedObj::operator=(traceClass);
        return *this;
    }

    Trace::Shared instantiate(const Trace& trace)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_trace_create(this->_libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return Trace::Shared {Trace {libObjPtr}};
    }

    StreamClass::Shared createStreamClass()
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_stream_class_create(this->_libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return StreamClass::Shared {StreamClass {libObjPtr}};
    }

    StreamClass::Shared createStreamClass(const std::uint64_t id)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_stream_class_create_with_id(this->_libObjPtr(), id);

        internal::validateCreatedObjPtr(libObjPtr);
        return StreamClass::Shared {StreamClass {libObjPtr}};
    }

    FieldClass::Shared createBoolFieldClass()
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_field_class_bool_create(this->_libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return FieldClass::Shared {FieldClass {libObjPtr}};
    }

    BitArrayFieldClass::Shared createBitArrayFieldClass(const std::uint64_t length)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_field_class_bit_array_create(this->_libObjPtr(), length);

        internal::validateCreatedObjPtr(libObjPtr);
        return BitArrayFieldClass::Shared {BitArrayFieldClass {libObjPtr}};
    }

    IntegerFieldClass::Shared createUnsignedIntegerFieldClass()
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_field_class_integer_unsigned_create(this->_libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return IntegerFieldClass::Shared {IntegerFieldClass {libObjPtr}};
    }

    IntegerFieldClass::Shared createSignedIntegerFieldClass()
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_field_class_integer_signed_create(this->_libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return IntegerFieldClass::Shared {IntegerFieldClass {libObjPtr}};
    }

    UnsignedEnumerationFieldClass::Shared createUnsignedEnumerationFieldClass()
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_field_class_enumeration_unsigned_create(this->_libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return UnsignedEnumerationFieldClass::Shared {UnsignedEnumerationFieldClass {libObjPtr}};
    }

    SignedEnumerationFieldClass::Shared createSignedEnumerationFieldClass()
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_field_class_enumeration_signed_create(this->_libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return SignedEnumerationFieldClass::Shared {SignedEnumerationFieldClass {libObjPtr}};
    }

    FieldClass::Shared createSinglePrecisionRealFieldClass()
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_field_class_real_single_precision_create(this->_libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return FieldClass::Shared {FieldClass {libObjPtr}};
    }

    FieldClass::Shared createDoublePrecisionRealFieldClass()
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_field_class_real_double_precision_create(this->_libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return FieldClass::Shared {FieldClass {libObjPtr}};
    }

    FieldClass::Shared createStringFieldClass()
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_field_class_string_create(this->_libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return FieldClass::Shared {FieldClass {libObjPtr}};
    }

    StaticArrayFieldClass::Shared createStaticArrayFieldClass(const FieldClass& elementFieldClass,
                                                              const std::uint64_t length)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_field_class_array_static_create(
            this->_libObjPtr(), elementFieldClass._libObjPtr(), length);

        internal::validateCreatedObjPtr(libObjPtr);
        return StaticArrayFieldClass::Shared {StaticArrayFieldClass {libObjPtr}};
    }

    ArrayFieldClass::Shared createDynamicArrayFieldClass(const FieldClass& elementFieldClass)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_field_class_array_dynamic_create(
            this->_libObjPtr(), elementFieldClass._libObjPtr(), nullptr);

        internal::validateCreatedObjPtr(libObjPtr);
        return ArrayFieldClass::Shared {ArrayFieldClass {libObjPtr}};
    }

    DynamicArrayWithLengthFieldClass::Shared
    createDynamicArrayFieldClass(const FieldClass& elementFieldClass,
                                 const IntegerFieldClass& lengthFieldClass)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_field_class_array_dynamic_create(
            this->_libObjPtr(), elementFieldClass._libObjPtr(), lengthFieldClass._libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return DynamicArrayWithLengthFieldClass::Shared {
            DynamicArrayWithLengthFieldClass {libObjPtr}};
    }

    StructureFieldClass::Shared createStructureFieldClass(const std::uint64_t length)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_field_class_structure_create(this->_libObjPtr(), length);

        internal::validateCreatedObjPtr(libObjPtr);
        return StructureFieldClass::Shared {StructureFieldClass {libObjPtr}};
    }

    OptionFieldClass::Shared createOptionFieldClass(const FieldClass& optionalFieldClass)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_field_class_option_without_selector_create(
            this->_libObjPtr(), optionalFieldClass._libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return OptionFieldClass::Shared {OptionFieldClass {libObjPtr}};
    }

    OptionWithBoolSelectorFieldClass::Shared
    createOptionWithBoolSelectorFieldClass(const FieldClass& optionalFieldClass,
                                           const FieldClass& selectorFieldClass)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_field_class_option_with_selector_field_bool_create(
            this->_libObjPtr(), optionalFieldClass._libObjPtr(), selectorFieldClass._libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return OptionWithBoolSelectorFieldClass::Shared {
            OptionWithBoolSelectorFieldClass {libObjPtr}};
    }

    OptionWithUnsignedIntegerSelectorFieldClass::Shared
    createOptionWithUnsignedIntegerSelectorFieldClass(const FieldClass& optionalFieldClass,
                                                      const IntegerFieldClass& selectorFieldClass,
                                                      const ConstUnsignedIntegerRangeSet& ranges)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_field_class_option_with_selector_field_integer_unsigned_create(
            this->_libObjPtr(), optionalFieldClass._libObjPtr(), selectorFieldClass._libObjPtr(),
            ranges._libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return OptionWithUnsignedIntegerSelectorFieldClass::Shared {
            OptionWithUnsignedIntegerSelectorFieldClass {libObjPtr}};
    }

    OptionWithSignedIntegerSelectorFieldClass::Shared
    createOptionWithSignedIntegerSelectorFieldClass(const FieldClass& optionalFieldClass,
                                                    const IntegerFieldClass& selectorFieldClass,
                                                    const ConstSignedIntegerRangeSet& ranges)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_field_class_option_with_selector_field_integer_signed_create(
            this->_libObjPtr(), optionalFieldClass._libObjPtr(), selectorFieldClass._libObjPtr(),
            ranges._libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return OptionWithSignedIntegerSelectorFieldClass::Shared {
            OptionWithSignedIntegerSelectorFieldClass {libObjPtr}};
    }

    VariantWithoutSelectorFieldClass::Shared createVariantFieldClass()
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr = bt_field_class_variant_create(this->_libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return VariantWithoutSelectorFieldClass::Shared {
            VariantWithoutSelectorFieldClass {libObjPtr}};
    }

    VariantWithUnsignedIntegerSelectorFieldClass::Shared
    createVariantWithUnsignedIntegerSelectorFieldClass(const IntegerFieldClass& selectorFieldClass)
    {
        return VariantWithUnsignedIntegerSelectorFieldClass::Shared {
            VariantWithUnsignedIntegerSelectorFieldClass {
                this->_createVariantWithIntegerSelectorFieldClass(selectorFieldClass)}};
    }

    VariantWithSignedIntegerSelectorFieldClass::Shared
    createVariantWithSignedIntegerSelectorFieldClass(const IntegerFieldClass& selectorFieldClass)
    {
        return VariantWithSignedIntegerSelectorFieldClass::Shared {
            VariantWithSignedIntegerSelectorFieldClass {
                this->_createVariantWithIntegerSelectorFieldClass(selectorFieldClass)}};
    }

    void assignsAutomaticStreamClassId(const bool val) noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        bt_trace_class_set_assigns_automatic_stream_class_id(this->_libObjPtr(),
                                                             static_cast<bt_bool>(val));
    }

    bool assignsAutomaticStreamClassId() const noexcept
    {
        return static_cast<bool>(
            bt_trace_class_assigns_automatic_stream_class_id(this->_libObjPtr()));
    }

    std::uint64_t size() const noexcept
    {
        return bt_trace_class_get_stream_class_count(this->_libObjPtr());
    }

    ConstStreamClass operator[](const std::uint64_t index) const noexcept
    {
        return ConstStreamClass {_ConstSpec::streamClassByIndex(this->_libObjPtr(), index)};
    }

    _StreamClass operator[](const std::uint64_t index) noexcept
    {
        return _StreamClass {_Spec::streamClassByIndex(this->_libObjPtr(), index)};
    }

    nonstd::optional<ConstStreamClass> streamClassById(const std::uint64_t id) const noexcept
    {
        const auto libObjPtr = _ConstSpec::streamClassById(this->_libObjPtr(), id);

        if (libObjPtr) {
            return ConstStreamClass {libObjPtr};
        }

        return nonstd::nullopt;
    }

    nonstd::optional<_StreamClass> streamClassById(const std::uint64_t id) noexcept
    {
        const auto libObjPtr = _Spec::streamClassById(this->_libObjPtr(), id);

        if (libObjPtr) {
            return _StreamClass {libObjPtr};
        }

        return nonstd::nullopt;
    }

    template <typename LibValT>
    void userAttributes(const CommonMapValue<LibValT>& userAttrs)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        bt_trace_class_set_user_attributes(this->_libObjPtr(), userAttrs._libObjPtr());
    }

    ConstMapValue userAttributes() const noexcept
    {
        return ConstMapValue {_ConstSpec::userAttributes(this->_libObjPtr())};
    }

    UserAttributes userAttributes() noexcept
    {
        return UserAttributes {_Spec::userAttributes(this->_libObjPtr())};
    }

    Shared shared() const noexcept
    {
        return Shared {*this};
    }

private:
    bt_field_class *
    _createVariantWithIntegerSelectorFieldClass(const IntegerFieldClass& selectorFieldClass)
    {
        static_assert(!std::is_const<LibObjT>::value, "`LibObjT` must NOT be `const`.");

        const auto libObjPtr =
            bt_field_class_variant_create(this->_libObjPtr(), selectorFieldClass._libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return libObjPtr;
    }
};

using TraceClass = CommonTraceClass<bt_trace_class>;
using ConstTraceClass = CommonTraceClass<const bt_trace_class>;

template <typename LibObjT>
ConstTraceClass CommonStreamClass<LibObjT>::traceClass() const noexcept
{
    return ConstTraceClass {_ConstSpec::traceClass(this->_libObjPtr())};
}

template <typename LibObjT>
typename CommonStreamClass<LibObjT>::_TraceClass CommonStreamClass<LibObjT>::traceClass() noexcept
{
    return _TraceClass {_Spec::traceClass(this->_libObjPtr())};
}

template <typename LibObjT>
ConstTraceClass CommonTrace<LibObjT>::cls() const noexcept
{
    return ConstTraceClass {_ConstSpec::cls(this->_libObjPtr())};
}

template <typename LibObjT>
typename CommonTrace<LibObjT>::Class CommonTrace<LibObjT>::cls() noexcept
{
    return Class {_Spec::cls(this->_libObjPtr())};
}

} // namespace bt2

#endif // BABELTRACE_CPP_COMMON_BT2_TRACE_IR_HPP
