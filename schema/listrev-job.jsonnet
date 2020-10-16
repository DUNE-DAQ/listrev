// Example commands for a listrev job.
//
// It is RandomDataListGenerator
//       |
//       +--> (orig1) -> ListReverser -> (giro1) ->\
//       |                                           ReversedListValidator
//       +--> (orig2) ---------------------------->/ 
//
local moo = import "moo.jsonnet";
local cmd = import "appfwk-cmd-make.jsonnet";
local rdlg = import "listrev-rdlg-make.jsonnet";


local q = {
    orig1: cmd.qspec("orig1", "StdDeQueue", 10),
    orig2: cmd.qspec("orig2", "StdDeQueue", 10),
    giro1: cmd.qspec("giro1", "StdDeQueue", 10),
};

[
    cmd.init([q.orig1, q.orig2, q.giro1],
             [cmd.mspec("rdlg", "RandomDataListGenerator", [
                 cmd.qinfo("q1", q.orig1.inst, "output"),
                 cmd.qinfo("q2", q.orig2.inst, "output")]),
              cmd.mspec("lr", "ListReverser", [
                  cmd.qinfo("input",q.orig1.inst,"input"),
                  cmd.qinfo("output",q.giro1.inst,"output")]),
              cmd.mspec("rlv", "ReversedListValidator", [
                  cmd.qinfo("reversed_data_input", q.giro1.inst, "input"),
                  cmd.qinfo("original_data_input", q.orig2.inst, "input")])]) { waitms: 1000 },

    cmd.conf([cmd.mcmd("rdlg", rdlg.conf(4,1000))]) { waitms: 1000 },

    cmd.start(42) { waitms: 1000 },

    cmd.stop() { waitms: 1000 },
]
