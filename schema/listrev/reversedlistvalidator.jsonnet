local moo = import "moo.jsonnet";
local ns = "dunedaq.listrev.reversedlistvalidator";
local s = moo.oschema.schema(ns);

local types = {
    count : s.number("Count", "i4",
                     doc="A count of not too many things"),

    conf: s.record("ConfParams", [
        s.field("send_timeout_ms", self.count, 100, doc="Milliseconds to wait while sending"),
        s.field("request_timeout_ms", self.count, 1000, doc="Milliseconds to wait before giving up on a request"),
        s.field("request_rate_hz", self.count, 10, doc="Target request rate, in Hz"),
        s.field("max_outstanding_requests", self.count, 100, doc="Number of requests to handle at one time"),
        s.field("num_reversers", self.count, 1, doc="Number of ListReverser instances in the system"),
        s.field("num_generators", self.count, 1, doc="Number of RandomDataListGenerator instances in the system"),
        s.field("min_list_size", self.count, 50, doc="Minimum size of created lists"),
        s.field("max_list_size", self.count, 200, doc="Maximum size of created lists"),
    ], doc="ReversedListValidator configuration"),

};

moo.oschema.sort_select(types, ns)
