/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

macro_rules! include_proto {
    ($path:literal) => {
        include!(mozbuild::srcdir_path!(concat!(
            "third_party/opentelemetry-cpp/third_party/opentelemetry-proto/",
            $path
        )));
    };
}

pub mod opentelemetry {
    pub mod proto {
        pub mod common {
            pub mod v1 {
                include_proto!("opentelemetry.proto.common.v1.rs");
            }
        }
        pub mod trace {
            pub mod v1 {
                include_proto!("opentelemetry.proto.trace.v1.rs");
            }
        }
        pub mod resource {
            pub mod v1 {
                include_proto!("opentelemetry.proto.resource.v1.rs");
            }
        }
        pub mod collector {
            pub mod trace {
                pub mod v1 {
                    include_proto!("opentelemetry.proto.collector.trace.v1.rs");
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    use std::time::{SystemTime, UNIX_EPOCH};

    use prost::Message;

    fn make_kv_string(
        key: &'static str,
        value: &'static str,
    ) -> opentelemetry::proto::common::v1::KeyValue {
        opentelemetry::proto::common::v1::KeyValue {
            key: key.to_string(),
            value: Some(opentelemetry::proto::common::v1::AnyValue {
                value: Some(
                    opentelemetry::proto::common::v1::any_value::Value::StringValue(
                        value.to_string(),
                    ),
                ),
            }),
        }
    }

    #[test]
    fn test_roundtrip_serialization() {
        let now = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_nanos() as u64;

        // Create the export request
        let test_export_request =
            opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest {
                resource_spans: vec![opentelemetry::proto::trace::v1::ResourceSpans {
                    resource: Some(opentelemetry::proto::resource::v1::Resource {
                        attributes: vec![
                            make_kv_string("service.name", "firefox"),
                            make_kv_string("service.name", "141.0.4"),
                        ],
                        dropped_attributes_count: 0,
                        entity_refs: vec![],
                    }),
                    scope_spans: vec![opentelemetry::proto::trace::v1::ScopeSpans {
                        scope: Some(opentelemetry::proto::common::v1::InstrumentationScope {
                            name: "gecko-trace".to_string(),
                            version: "0.1.0".to_string(),
                            attributes: vec![],
                            dropped_attributes_count: 0,
                        }),
                        spans: vec![opentelemetry::proto::trace::v1::Span {
                            trace_id: b"123456789abcdef01122334455667788".to_vec(),
                            span_id: b"aabbccddeeff0011".to_vec(),
                            trace_state: "vendor1=value1,vendor2=value2".to_string(),
                            parent_span_id: vec![], // Root span
                            flags: 1,               // Sampled flag
                            name: "HTTP GET /api/data".to_string(),
                            kind: opentelemetry::proto::trace::v1::span::SpanKind::Server as i32,
                            start_time_unix_nano: now - 1_000_000_000, // 1 second ago
                            end_time_unix_nano: now,
                            attributes: vec![make_kv_string("origin", "www.example.com")],
                            dropped_attributes_count: 0,
                            events: vec![opentelemetry::proto::trace::v1::span::Event {
                                time_unix_nano: now - 500_000_000, // 0.5 seconds ago
                                name: "Processing request".to_string(),
                                attributes: vec![],
                                dropped_attributes_count: 0,
                            }],
                            dropped_events_count: 0,
                            links: vec![],
                            dropped_links_count: 0,
                            status: Some(opentelemetry::proto::trace::v1::Status {
                                message: "".to_string(),
                                code: opentelemetry::proto::trace::v1::status::StatusCode::Ok
                                    as i32,
                            }),
                        }],
                        schema_url: "https://opentelemetry.io/schemas/1.21.0".to_string(),
                    }],
                    schema_url: "https://opentelemetry.io/schemas/1.21.0".to_string(),
                }],
            };

        assert_eq!(
            Ok(&test_export_request),
            opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest::decode(
                test_export_request.encode_to_vec().as_slice()
            )
            .as_ref()
        );
    }
}
