// This is an example of how to define a schema for operational monitoring

local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.listrev.randomdatalistgeneratorinfo");

local info = {
    uint8  : s.number("uint8", "u8",
        doc="An unsigned of 8 bytes"),

   info: s.record("Info", [
       s.field("generated_numbers", self.uint8, 0, doc="Counting generated numbers"), 
       s.field("new_generated_numbers", self.uint8, 0, doc="Counting incrementally generated numbers"), 
   ], doc="List generator information information")
};

moo.oschema.sort_select(info) 
