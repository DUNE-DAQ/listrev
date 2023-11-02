local moo = import "moo.jsonnet";
local ns = "dunedaq.listrev.reversedlistvalidator";
local s = moo.oschema.schema(ns);

local types = {
    count : s.number("Count", "i4",
                     doc="A count of not too many things"),

    conf: s.record("ConfParams", [
        s.field("send_timeout_ms", self.count, 100, doc="Milliseconds to wait while sending"),
        s.field("max_outstanding_requests", self.count, 100, doc="Number of requests to handle at one time"),
        s.field("request_send_interval", self.count, 1000, doc="Milliseconds to wait between request broadcasts"),
    ], doc="ReversedListValidator configuration"),

};

moo.oschema.sort_select(types, ns)
