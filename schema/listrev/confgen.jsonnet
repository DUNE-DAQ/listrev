// This is the configuration schema for listrev

local moo = import "moo.jsonnet";
local sdc = import "daqconf/types.jsonnet";
local daqconf = moo.oschema.hier(sdc).dunedaq.daqconf.types;

local sboot = import "daqconf/bootgen.jsonnet";
local bootgen = moo.oschema.hier(sboot).dunedaq.daqconf.bootgen;

local sdetector = import "daqconf/detectorgen.jsonnet";
local detectorgen = moo.oschema.hier(sdetector).dunedaq.daqconf.detectorgen;

local ns = "dunedaq.listrev.confgen";
local s = moo.oschema.schema(ns);

// A temporary schema construction context.
local cs = {
  number: s.number  ("number", "i8", doc="a number"), // !?!?!
  app:    s.string  ("app", doc="a string"), // !?!?!
  apps:   s.sequence("apps", self.app, "some strings"),

  listrev: s.record("listrev", [
    s.field('host_app',      daqconf.host, default='localhost', doc='Host to run the listrev sw app on'),
    s.field('ints_per_list', self.number,  default=4,           doc='Number of integers in the list'),
    s.field('wait_ms',       self.number,  default=1000,        doc='Number of ms to wait between list sends'),
    s.field('apps',          self.apps,    default=['s'],       doc="Apps to generate: \"s\" for single-app ListRev, otherwise specify \"g\", \"r\", and \"v\". E.g.: [\"gv\",\"r\"]")
  ]),

  commtest: s.record("commtest", [
    s.field('hosts',         daqconf.hosts, default=['localhost', 'localhost'], doc='Hosts to run test programs on. First host will receive \"rv\" app, while others will have \"g\" apps'),
    s.field('ints_per_list', self.number,   default=4,                          doc='Number of integers in the list'),
    s.field('wait_ms',       self.number,   default=1000,                       doc='Number of ms to wait between list sends'),
  ]),

  listrev_gen: s.record('listrev_gen', [
    s.field('detector',    detectorgen.detector,   default=detectorgen.detector,     doc='Boot parameters'),
    s.field('boot',        bootgen.boot,    default=bootgen.boot,      doc='Boot parameters'),
    s.field('listrev', self.listrev, default=self.listrev, doc='Listrev paramaters'),
  ]),

  commtest_gen: s.record('commtest_gen', [
    s.field('detector',    detectorgen.detector,   default=detectorgen.detector,     doc='Boot parameters'),
    s.field('boot',        bootgen.boot,    default=bootgen.boot,      doc='Boot parameters'),
    s.field('commtest', self.commtest, default=self.commtest, doc='Commtest paramaters'),
  ]),
};

// Output a topologically sorted array.
sdc + sboot + sdetector + moo.oschema.sort_select(cs, ns)
