/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "gtest/gtest.h"

#include "nsIThread.h"
#include "nsThreadUtils.h"

#include "opentelemetry/context/runtime_context.h"

#include "Common.h"

using namespace mozilla::tracing::tests;

TEST(Tracing, Multithreaded)
{
  using opentelemetry::context::RuntimeContext;

  auto tracer = GetTracer();

  auto span = tracer->StartSpan("Outer operation");

  // Establishes a scope where the provided span becomes the currently active
  // span in the context. Upon exiting the scope, the span is ended, and the
  // previously active span is restored.
  auto scope = tracer->WithActiveSpan(span);

  nsCOMPtr<nsIThread> thread;
  ASSERT_EQ(NS_NewNamedThread("Test Thread", getter_AddRefs(thread)), NS_OK);

  thread->Dispatch(NS_NewRunnableFunction(
      __func__,
      [
          // Retrieves the current runtime context, which includes
          // information about the currently active span.
          context = RuntimeContext::GetCurrent(),
          // Extracts the trace ID that uniquely identifies the current trace.
          trace_id = span->GetContext().trace_id()] {
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
        ASSERT_EQ(span->GetContext().trace_id(), trace_id);
      }));

  ASSERT_EQ(thread->Shutdown(), NS_OK);
}
