// This is the configuration schema for listrev

local moo = import "moo.jsonnet";
local sdc = import "daqconf/confgen.jsonnet";
local daqconf = moo.oschema.hier(sdc).dunedaq.daqconf.confgen;

local ns = "dunedaq.listrev.confgen";
local s = moo.oschema.schema(ns);

// A temporary schema construction context.
local cs = {
  number: s.number  ("number", "i8", doc="a number"), // !?!?!
  app:    s.string  ("app", doc="a string"), // !?!?!
  apps:   s.sequence("apps", self.app, "some strings"),

  listrev: s.record("listrev", [
    s.field('host_app',      daqconf.Host, default='localhost', doc='Host to run the listrev sw app on'),
    s.field('ints_per_list', self.number,  default=4,           doc='Number of integers in the list'),
    s.field('wait_ms',       self.number,  default=1000,        doc='Number of ms to wait between list sends'),
    s.field('apps',          self.apps,    default=['s'],       doc="Apps to generate: \"s\" for single-app ListRev, otherwise specify \"g\", \"r\", and \"v\". E.g.: [\"gv\",\"r\"]")
  ]),

  listrev_gen: s.record('listrev_gen', [
    s.field('boot',    daqconf.boot, default=daqconf.boot, doc='Boot parameters'),
    s.field('listrev', self.listrev, default=self.listrev, doc='Listrev paramaters'),
  ]),
};

// Output a topologically sorted array.
sdc + moo.oschema.sort_select(cs, ns)
