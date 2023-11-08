local moo = import "moo.jsonnet";
local ns = "dunedaq.listrev.listreverser";
local s = moo.oschema.schema(ns);

local types = {
    count : s.number("Count", "i4",
                     doc="A count of not too many things"),

    conf: s.record("ConfParams", [
        s.field("send_timeout_ms", self.count, 100, doc="Milliseconds to wait while sending"),
        s.field("request_timeout_ms", self.count, 1000, doc="Milliseconds to wait before giving up on a request"),
        s.field("num_generators", self.count, 1, doc="Number of RandomDataListGenerator instances in the system"),
        s.field("reverser_id", self.count, 0, doc="Index of this ListReverser instance"),
    ], doc="ListReverser configuration"),

};

moo.oschema.sort_select(types, ns)
