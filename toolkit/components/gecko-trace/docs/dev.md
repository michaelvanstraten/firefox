# Developer Guide

## Component Architecture

```{eval-rst}
.. mermaid:: component-architecture.mmd
```

## Updating Third-Party Dependencies

The gecko-trace component depends on two main third-party libraries:

- [`opentelemetry-cpp`][1] - The core OpenTelemetry C++ library
- [`opentelemetry-proto`][2] - Protocol buffer definitions for OpenTelemetry

These dependencies are vendored in the following locations:
- `third_party/opentelemetry-cpp`
- `third_party/opentelemetry-cpp/third_party/opentelemetry-proto`

### Automatic Updates

An update bot is configured to automatically keep `opentelemetry-cpp` up-to-date with the latest [release][3].

The `third_party/opentelemetry-cpp/moz.yaml` file specifies the maintainer who receives notifications when the update bot discovers a new version. This person is responsible for reviewing the associated patch and deciding whether to merge it.

To change the contact person, update the following section in `moz.yaml`:

```yaml
# third_party/opentelemetry-cpp/moz.yaml
updatebot:
  # Change these values as needed
  maintainer-phab: mvanstraten
  maintainer-bz: mvanstraten@mozilla.com
```

### Keeping opentelemetry-proto in Sync

The `opentelemetry-proto` version should be kept in sync with the submodule specified in the [opentelemetry-cpp repository][4].

After each update bot update, verify that the version specified in the opentelemetry-cpp submodule matches the version specified in `third_party/opentelemetry-cpp/third_party/opentelemetry-proto/moz.yaml`.

If the versions don't match, manually re-vendor the repository:

```bash
mach vendor third_party/opentelemetry-cpp/third_party/opentelemetry-proto/moz.yaml
```

This command will also update any Rust/C++ files specified in the `update-actions` configuration:

```yaml
# third_party/opentelemetry-cpp/third_party/opentelemetry-proto/moz.yaml
update-actions:
  - action: run-script
    script: "{topsrcdir}/toolkit/components/protobuf/scripts/protoc_wrapper.py"
    cwd: "{topsrcdir}"
    args:
      - opentelemetry/proto/collector/trace/v1/trace_service.proto
      - opentelemetry/proto/common/v1/common.proto
```

For more information about vendoring third-party components in Gecko, see the [Firefox Source Documentation][5].

[1]: https://github.com/open-telemetry/opentelemetry-cpp
[2]: https://github.com/open-telemetry/opentelemetry-proto
[3]: https://github.com/open-telemetry/opentelemetry-cpp/releases
[4]: https://github.com/open-telemetry/opentelemetry-cpp/tree/main/third_party
[5]: https://firefox-source-docs.mozilla.org/mozbuild/vendor/index.html
