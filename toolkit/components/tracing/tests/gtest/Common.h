/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef MOZ_TRACING_TESTS_COMMOM_H_
#define MOZ_TRACING_TESTS_COMMOM_H_

#include "opentelemetry/trace/provider.h"

namespace mozilla::tracing::tests {
auto GetTracer() {
  auto provider = opentelemetry::trace::Provider::GetTracerProvider();
  return provider->GetTracer("test-tracer", "1.0.0");
}
}  // namespace mozilla::tracing::tests

#endif
