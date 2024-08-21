/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "mozilla/Logging.h"
#include "opentelemetry/exporters/ostream/span_exporter_factory.h"
#include "opentelemetry/sdk/trace/simple_processor_factory.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/tracer_provider.h"

namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace trace_exporter = opentelemetry::exporter::trace;

namespace mozilla::tracing {

mozilla::LazyLogModule gLogModule("tracing");

void InitTracer() {
  // Create ostream span exporter instance
  auto exporter = trace_exporter::OStreamSpanExporterFactory::Create();
  auto processor =
      trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));

  std::shared_ptr<opentelemetry::sdk::trace::TracerProvider> provider =
      opentelemetry::sdk::trace::TracerProviderFactory::Create(
          std::move(processor));

  // Set the global trace provider
  const std::shared_ptr<trace_api::TracerProvider>& api_provider = provider;
  trace_api::Provider::SetTracerProvider(api_provider);
}

void Init() {
  MOZ_LOG(gLogModule, LogLevel::Info, ("Initializing tracing component"));

  InitTracer();
}

}  // namespace mozilla::tracing
