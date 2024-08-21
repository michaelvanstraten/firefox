/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef MOZ_TRACING_IPC_TYPES_H_
#define MOZ_TRACING_IPC_TYPES_H_

#include <chrono>
#include "chrome/common/ipc_message_utils.h"
#include "ipc/EnumSerializer.h"

#include "opentelemetry/common/timestamp.h"
#include "opentelemetry/context/context.h"
#include "opentelemetry/context/runtime_context.h"
#include "opentelemetry/nostd/shared_ptr.h"
#include "opentelemetry/sdk/trace/span_data.h"
#include "opentelemetry/trace/context.h"
#include "opentelemetry/trace/span.h"
#include "opentelemetry/trace/span_context.h"

namespace IPC {

template <>
struct ParamTraits<opentelemetry::context::Context> {
  using Context = opentelemetry::context::Context;

  static void Write(MessageWriter* writer, const Context& ctx) {
    auto const span_context = opentelemetry::trace::GetSpan(ctx)->GetContext();
    WriteParam(writer, span_context);
  }
  static bool Read(MessageReader* reader, Context* ctx) {
    using opentelemetry::nostd::shared_ptr;

    auto span_context = opentelemetry::trace::SpanContext::GetInvalid();

    if (!ReadParam(reader, &span_context)) {
      return false;
    }

    if (!span_context.IsValid()) {
      // TO-DO!: figure out what to do here
      return true;
    }

    shared_ptr<opentelemetry::trace::Span> default_span{
        new opentelemetry::trace::DefaultSpan(span_context)};
    auto current_ctx = opentelemetry::context::RuntimeContext::GetCurrent();
    *ctx = opentelemetry::trace::SetSpan(current_ctx, default_span);

    return true;
  }
};

template <>
struct ParamTraits<opentelemetry::trace::SpanContext> {
  using SpanContext = opentelemetry::trace::SpanContext;

  static void Write(MessageWriter* writer, const SpanContext& ctx) {
    WriteParam(writer, ctx.trace_flags());
    WriteParam(writer, ctx.trace_id());
    WriteParam(writer, ctx.span_id());
    /* WriteParam(writer, ctx.trace_state()); */
  }
  static bool Read(MessageReader* reader, SpanContext* ctx) {
    opentelemetry::trace::TraceFlags trace_flags;
    opentelemetry::trace::TraceId trace_id;
    opentelemetry::trace::SpanId span_id;

    if (!(ReadParam(reader, &trace_flags) && ReadParam(reader, &trace_id) &&
          ReadParam(reader, &span_id))) {
      return false;
    }

    static const bool is_remote = true;
    *ctx = SpanContext(trace_id, span_id, trace_flags, is_remote);

    return true;
  }
};

template <>
struct ParamTraits<opentelemetry::sdk::trace::SpanData> {
  using SpanData = opentelemetry::sdk::trace::SpanData;
  static void Write(MessageWriter* writer, const SpanData& span_data) {
    WriteParam(writer, span_data.GetSpanContext());
    WriteParam(writer, span_data.GetParentSpanId());
    /* WriteParam(writer, in.GetName()); */
    WriteParam(writer, span_data.GetSpanKind());
    WriteParam(writer, span_data.GetStatus());
    /* WriteParam(writer, in.GetDescription()); */
    /* WriteParam(writer, in.GetResource()); */
    /* WriteParam(writer, in.GetInstrumentationScope()); */
    /* WriteParam(writer, in.GetInstrumentationLibrary()); */
    WriteParam(writer, span_data.GetStartTime());
    WriteParam(writer, span_data.GetDuration().count());
    /* WriteParam(writer, in.GetAttributes()); */
    /* WriteParam(writer, in.GetEvents()); */
    /* WriteParam(writer, in.GetLinks()); */
  }
  static bool Read(MessageReader* reader, SpanData* out) {
    using opentelemetry::common::SystemTimestamp;
    using opentelemetry::trace::SpanContext;
    using opentelemetry::trace::SpanId;
    using opentelemetry::trace::SpanKind;
    using opentelemetry::trace::StatusCode;

    auto span_context = SpanContext::GetInvalid();
    SpanId parent_span_id;
    SpanKind span_kind;
    SystemTimestamp start_time;
    int64_t duration;
    StatusCode status_code;

    if (!(ReadParam(reader, &span_context) &&
          ReadParam(reader, &parent_span_id) && ReadParam(reader, &span_kind) &&
          ReadParam(reader, &start_time) && ReadParam(reader, &duration) &&
          ReadParam(reader, &status_code))) {
      return false;
    }

    out->SetIdentity(span_context, parent_span_id);
    out->SetSpanKind(span_kind);
    out->SetStartTime(start_time);
    out->SetDuration(std::chrono::nanoseconds(duration));
    out->SetStatus(status_code, "todo");

    return true;
  }
};

template <>
struct ParamTraits<opentelemetry::trace::SpanId> {
  using SpanId = opentelemetry::trace::SpanId;

  static void Write(MessageWriter* writer, const SpanId& span_id) {
    const auto& raw_id = span_id.Id();
    writer->WriteBytes(raw_id.data(), SpanId::kSize);
  }

  static bool Read(MessageReader* reader, SpanId* span_id) {
    if (!reader->HasBytesAvailable(SpanId::kSize)) {
      return false;
    }
    std::array<uint8_t, SpanId::kSize> raw_id{};
    if (!reader->ReadBytesInto(raw_id.data(), SpanId::kSize)) {
      return false;
    }
    *span_id = opentelemetry::trace::SpanId{
        opentelemetry::nostd::span<const uint8_t, SpanId::kSize>(
            raw_id.data(), raw_id.size())};

    return true;
  }
};

template <>
struct ParamTraits<opentelemetry::trace::SpanKind>
    : public ContiguousEnumSerializerInclusive<
          opentelemetry::trace::SpanKind,
          opentelemetry::trace::SpanKind::kInternal,
          opentelemetry::trace::SpanKind::kConsumer> {};

template <>
struct ParamTraits<opentelemetry::trace::StatusCode>
    : public ContiguousEnumSerializerInclusive<
          opentelemetry::trace::StatusCode,
          opentelemetry::trace::StatusCode::kUnset,
          opentelemetry::trace::StatusCode::kError> {};

template <>
struct ParamTraits<opentelemetry::trace::TraceFlags> {
  using TraceFlags = opentelemetry::trace::TraceFlags;

  static void Write(MessageWriter* writer, const TraceFlags& trace_flags) {
    WriteParam(writer, trace_flags.flags());
  }
  static bool Read(MessageReader* reader, TraceFlags* trace_flags) {
    uint8_t flags;

    if (!ReadParam(reader, &flags)) {
      return false;
    }

    *trace_flags = opentelemetry::trace::TraceFlags(flags);
    return true;
  }
};

template <>
struct ParamTraits<opentelemetry::trace::TraceId> {
  using TraceId = opentelemetry::trace::TraceId;

  static void Write(MessageWriter* writer, const TraceId& trace_id) {
    const auto& raw_id = trace_id.Id();
    writer->WriteBytes(raw_id.data(), TraceId::kSize);
  }

  static bool Read(MessageReader* reader, TraceId* trace_id) {
    if (!reader->HasBytesAvailable(TraceId::kSize)) {
      return false;
    }

    std::array<uint8_t, TraceId::kSize> raw_id{};
    if (!reader->ReadBytesInto(raw_id.data(), TraceId::kSize)) {
      return false;
    }

    *trace_id = opentelemetry::trace::TraceId{
        opentelemetry::nostd::span<const uint8_t, TraceId::kSize>(
            raw_id.data(), raw_id.size())};
    return true;
  }
};

template <>
struct ParamTraits<opentelemetry::common::SystemTimestamp> {
  using SystemTimestamp = opentelemetry::common::SystemTimestamp;

  static void Write(MessageWriter* writer,
                    const SystemTimestamp& system_timestamp) {
    writer->WriteInt64(system_timestamp.time_since_epoch().count());
  }

  static bool Read(MessageReader* reader, SystemTimestamp* system_timestamp) {
    int64_t time_since_epoch;

    if (!reader->ReadInt64(&time_since_epoch)) {
      return false;
    }

    *system_timestamp =
        SystemTimestamp(std::chrono::nanoseconds(time_since_epoch));

    return true;
  };
};

}  // namespace IPC

#endif  // !MOZ_TRACING_IPC_TYPES_H_
