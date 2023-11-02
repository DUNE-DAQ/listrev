// This is an example of how to define a schema for operational monitoring

local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.listrev.listreverserinfo");

local info = {
    uint8  : s.number("uint8", "u8",
        doc="An unsigned of 8 bytes"),

   info: s.record("Info", [
       s.field("requests_received", self.uint8, 0, doc="Count of received requests"), 
       s.field("requests_sent", self.uint8, 0, doc="Count of sent requests"), 
       s.field("lists_received", self.uint8, 0, doc="Count of lists received"), 
       s.field("lists_sent", self.uint8, 0, doc="Count of sent lists"), 
   ], doc="List generator information information")
};

moo.oschema.sort_select(info) 
