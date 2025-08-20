# Gecko Trace

Firefox contains many operations that are highly asynchronous or that cross thread
and process boundaries. When failures occur in these complex systems, it can be
difficult to diagnose the root cause because the point of failure is often far
from the source of the problem. Standard logging can be insufficient for
tracking the entire lifecycle of an operation.

The Gecko Trace component tries to help solve this by providing a mechanism to trace
the flow of a single operation throughout the entire system. It is built on the
[OpenTelemetry](https://opentelemetry.io/docs/concepts/) framework and provides
an API for creating traces and attaching structured events to them. A trace gives
you a detailed, hierarchical view of a single operation, much like a profiler
capture, but focused on a specific logical task rather than a time-based sample.

This allows developers to:
- Better track execution flows to identify the root causes of elusive bugs.
- Gain a comprehensive view of activities that span multiple components,
  threads, or processes.
- Instrument code in a way that benefits both telemetry and potentially other
  tools like the Gecko Profiler in the future.

## Concepts

At a high level, Gecko Trace provides two main concepts from the OpenTelemetry
world: **Traces** and **Spans**. For more detailed definitions, you can refer to
the [OpenTelemetry documentation](https://opentelemetry.io/docs/concepts/signals/traces/).

### Traces

A **Trace** represents the entire lifecycle of a single logical operation. It
gives you a hierarchical view of all the work that happened during that operation,
which can even cross thread boundaries. A trace is made up of a tree of **Spans**.

For example, a trace might encapsulate a full end-to-end request to initialize an
IndexedDB origin.

```{mermaid}
graph TD
    subgraph "Trace: QuotaClient::InitOrigin"
        A["QuotaClient::InitOrigin"] --> B["LoadUsageFile"];
        A --> D["WithRecovery"];
        D --> E["CreateStorageConnection"];
        A --> F["UpdateUsageFile"];
    end
```
### Spans and Events

A **Span** represents a single, named unit of work within a trace. Each trace
has a "root span" that represents the top-level operation, and this root span
can have a tree of child spans for sub-operations. You can read more about spans
in the [OpenTelemetry documentation](https://opentelemetry.io/docs/concepts/signals/traces/#spans).

Spans can be created at runtime using the gecko-trace API. The name of the span
should represent the operation.

```cpp
#include "mozilla/GeckoTrace.h"

auto root_span = mozilla::gecko_trace::TraceProvider::GetTracer("dom.quota")
                ->StartSpan("QuotaClient::InitOrigin");
```

A Span has to be entered to be marked as the "active" span in the current
context. The returned `scope` object is an RAII wrapper that handles this for
you. When the `scope` object goes out of scope, the span is exited.

```cpp
auto scope = root_span->Enter();
```

Any subsequent span created while another span is active will become a child of
that active span, automatically forming the trace hierarchy.

```cpp
#include "mozilla/GeckoTrace.h"

// This span will be a child of the `root_span` created above,
// assuming it was "entered".
auto nested_span = mozilla::gecko_trace::TraceProvider::GetTracer("dom.quota")
                ->StartSpan("LoadUsageFile");
```

While you can create spans manually, the most convenient method is to use the
`GECKO_TRACE_SCOPE` macro. This macro creates a new span and enters it, ensuring
it is active for the entire scope of the current function.

```cpp
nsresult LoadUsageFile(nsIFile& aUsageFile) {
  GECKO_TRACE_SCOPE("dom::quota", "LoadUsageFile");

  AssertIsOnIOThread();
  // ... function body ...
}
```

#### Adding Events to a Span

An **Event** can be thought of as a structured log message that is attached to
the currently active span. If no span is active on the current thread, the event
is discarded. You can read more about span events in the [OpenTelemetry
documentation](https://opentelemetry.io/docs/concepts/signals/traces/#span-events).

Events are defined in `gecko-trace.yaml` files, which are similar to Glean's
`metrics.yaml`. To add a new event:

1.  **Find or create a `gecko-trace.yaml` file.** If your component does not
    already have one, create it and add its path to the
    `gecko_trace_yaml_files` list in
    `toolkit/components/gecko-trace/index.py`.

2.  **Define your event.** Add an entry to the `events` section of the YAML
    file. The event name can be dotted.

    ```yaml
    # In dom/quota/gecko-trace.yaml
    events:
      dom.quota.try:
        description: >
          An event recorded on an error of the quota manager or its clients.
        inherits_from: [source_located]
        attributes:
          result:
            description: >
              Optionally, the name of the error that occurred.
            type: string
          severity:
            description: >
              One of WARNING or ERROR.
            type: string
          # ... other attributes
    ```

    The `inherits_from: [source_located]` directive automatically includes a
    standard set of attributes defined elsewhere, such as `source_file` and
    `source_line`, into our event. This helps keep definitions consistent.


3.  **(Required) Regenerate the metrics files.** This step is required until bug
    1892687 is resolved. This command updates Glean with your new event
    definition so it can be processed by the telemetry pipeline.

    ```bash
    ./mach gecko-trace generated-metrics
    ```

4.  **Use your event in code.** The YAML definition generates a C++ class for
    your event under the `mozilla::gecko_trace::events` namespace. The event name
    (`dom.quota.try`) is converted to PascalCase (`DomQuotaTryEvent`).
    You can then use the generated builder methods to set attributes and emit
    the event.

    ```cpp
    // In dom/quota/QuotaCommon.cpp:LogError
    mozilla::gecko_trace::events::DomQuotaTryEvent()
        .WithSourceFile(sourceFileRelativePath)
        .WithSourceLine(aSourceFileLine)
        .WithResult(rvName)
        .WithSeverity(severity)
        .Emit();
    ```

```{toctree}
:titlesonly:
:maxdepth: 2
:glob:

dev
```
