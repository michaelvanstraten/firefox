/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "gtest/gtest.h"

#include "mozilla/_ipdltest/IPDLUnitTest.h"

#include "mozilla/ipc/ProtocolUtils.h"

#include "mozilla/tracing/tests/PTestMultiprocessParent.h"
#include "mozilla/tracing/tests/PTestMultiprocessChild.h"

#include "opentelemetry/context/runtime_context.h"

#include "Common.h"

using mozilla::ipc::IPCResult;

using opentelemetry::context::RuntimeContext;
using opentelemetry::trace::TraceId;

namespace mozilla::tracing::tests {

class TestMultiprocessParent final : public PTestMultiprocessParent {
  NS_INLINE_DECL_REFCOUNTING(TestMultiprocessParent, override);

 private:
  ~TestMultiprocessParent() = default;
};

class TestMultiprocessChild final : public PTestMultiprocessChild {
  NS_INLINE_DECL_REFCOUNTING(TestMultiprocessChild, override)
 private:
  IPCResult RecvWithTraceContext(const Context& context,
                                 const TraceId& traceId) override;

  ~TestMultiprocessChild() = default;
};

IPCResult TestMultiprocessChild::RecvWithTraceContext(const Context& context,
                                                      const TraceId& traceId) {
  // Attaches the retrieved runtime context to the current thread. The
  // token serves as a guard, ensuring that upon its destruction, the
  // context reverts to its state prior to attachment.
  auto token = RuntimeContext::Attach(context);

  // Starting a new span within a context that already has an
  // active span will inherit its trace ID and set the parent
  // span.
  auto span = GetTracer()->StartSpan("Inner operation");

  // Verifies that the trace ID of the new span matches that of the outer
  // operation.
  EXPECT_EQ(span->GetContext().trace_id(), traceId);

  Close();

  return IPC_OK();
}

IPDL_TEST(TestMultiprocess, Tracing) {
  auto tracer = GetTracer();

  auto span = tracer->StartSpan("Outer operation");

  // Establishes a scope where the provided span becomes the currently active
  // span in the context. Upon exiting the scope, the span is ended, and the
  // previously active span is restored.
  auto scope = tracer->WithActiveSpan(span);

  bool ok = mActor->SendWithTraceContext(
      // Retrieves the current runtime context, which includes
      // information about the currently active span.
      RuntimeContext::GetCurrent(),
      // Extracts the trace ID that uniquely identifies the current trace.
      span->GetContext().trace_id());

  EXPECT_TRUE(ok);
};
}  // namespace mozilla::tracing::tests
