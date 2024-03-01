// This is an example of how to define a schema for operational monitoring

local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.listrev.reversedlistvalidatorinfo");

local info = {
    uint8  : s.number("uint8", "u8",
        doc="An unsigned of 8 bytes"),

   info: s.record("Info", [
       s.field("requests_total", self.uint8, 0, doc="Count of all requests"), 
       s.field("new_requests", self.uint8, 0, doc="Count of newly-generated requests"), 
       s.field("total_lists", self.uint8, 0, doc="Count of all lists received"), 
       s.field("new_lists", self.uint8, 0, doc="Count of new lists received"), 
       s.field("total_valid_pairs", self.uint8, 0, doc="Count of all valid list pairs"), 
       s.field("valid_list_pairs", self.uint8, 0, doc="Count of valid list pairs"), 
       s.field("total_invalid_pairs", self.uint8, 0, doc="Count of all invalid list pairs"),  
       s.field("invalid_list_pairs", self.uint8, 0, doc="Count of invalid list pairs"), 
   ], doc="List generator information information")
};

moo.oschema.sort_select(info) 
