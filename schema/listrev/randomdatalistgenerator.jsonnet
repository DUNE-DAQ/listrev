local moo = import "moo.jsonnet";
local ns = "dunedaq.listrev.randomdatalistgenerator";
local s = moo.oschema.schema(ns);

local types = {
    count : s.number("Count", "i4",
                     doc="A count of not too many things"),

    conf: s.record("ConfParams", [
        s.field("send_timeout_ms", self.count, 100, doc="Milliseconds to wait while sending"),
        s.field("request_timeout_ms", self.count, 1000, doc="Milliseconds to wait before giving up on a request"),
        s.field("generator_id", self.count, 0, doc="Index of this RandomDataListGenerator instance"),
    ], doc="RandomDataListGenerator configuration"),

};

moo.oschema.sort_select(types, ns)
