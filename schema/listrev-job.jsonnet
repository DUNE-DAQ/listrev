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


[
    cmd.init([cmd.qspec("orig1", "StdDeQueue", 10),
              cmd.qspec("orig2", "StdDeQueue", 10),
              cmd.qspec("giro1", "StdDeQueue", 10)],
             [cmd.mspec("rdlg", "RandomDataListGenerator", [
                 cmd.qinfo("q1", "orig1", "output"),
                 cmd.qinfo("q2", "orig2", "output")]),
              cmd.mspec("lr", "ListReverser", [
                  cmd.qinfo("input","orig1","input"),
                  cmd.qinfo("output","giro1","output")]),
              cmd.mspec("rlv", "ReversedListValidator", [
                  cmd.qinfo("reversed_data_input", "giro1", "input"),
                  cmd.qinfo("original_data_input", "orig2", "input")])]),

    cmd.conf([cmd.mcmd("rdlg", rdlg.conf(4,1000))]),

    cmd.start(42),

    cmd.stop(),
]
