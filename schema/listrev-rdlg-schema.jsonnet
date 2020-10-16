local moo = import "moo.jsonnet";
local ns = "dunedaq.listrev.rdlg";
local s = moo.oschema.schema(ns);

local types = {
    size: s.number("Size", "u8",
                   doc="A count of very many things"),

    count : s.number("Count", "i4",
                     doc="A count of not too many things"),

    conf: s.record("Conf", [
        s.field("nIntsPerList", self.size, 4,
                doc="Number of numbers"),
        s.field("waitBetweenSendsMsec", self.count, 1000,
                doc="Millisecs to wait between sending"),
    ], doc="RandomDataListGenerator configuration"),

};

moo.oschema.sort_select(types, ns)
