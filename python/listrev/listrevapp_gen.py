# This module facilitates the generation of WIB modules within WIB apps

# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io

moo.io.default_load_path = get_moo_model_path()

# Load configuration types
import moo.otypes

moo.otypes.load_types("listrev/randomdatalistgenerator.jsonnet")

# Import new types
import dunedaq.listrev.randomdatalistgenerator as rlg


from daqconf.core.app import App, ModuleGraph
from daqconf.core.daqmodule import DAQModule
from daqconf.core.conf_utils import Endpoint, Direction

# ===============================================================================
def get_listrev_app(nickname, host="localhost", n_ints=4, n_wait_ms=1000, gen_mode="s"):
    """
    Here an entire application is generated.
    """
    # Define modules

    modules = []

    if gen_mode == "s" or "g" in gen_mode:
        modules += [
            DAQModule(
                name="rdlg",
                plugin="RandomDataListGenerator",
                conf=rlg.ConfParams(
                    nIntsPerList=n_ints, waitBetweenSendsMsec=n_wait_ms
                ),
            )
        ]

    if gen_mode == "s" or "r" in gen_mode:
        modules += [DAQModule(name="lr", plugin="ListReverser")]

    if gen_mode == "s" or "v" in gen_mode:
        modules += [DAQModule(name="lrv", plugin="ReversedListValidator")]

    mgraph = ModuleGraph(modules)

    if gen_mode == "s":
        mgraph.connect_modules("rdlg.q1", "lrv.original_data_input", "IntList", "original", size_hint=10)
        mgraph.connect_modules("rdlg.q2", "lr.input", "IntList", "to_reverse", size_hint=10)
        mgraph.connect_modules("lr.output", "lrv.reversed_data_input", "IntList", "reversed", size_hint=10)

    if gen_mode != "s" and "g" in gen_mode:
        mgraph.add_endpoint("original", "rdlg.q1", "IntList", Direction.OUT)
        mgraph.add_endpoint("to_reverse", "rdlg.q2", "IntList", Direction.OUT)

    if gen_mode != "s" and "r" in gen_mode:
        mgraph.add_endpoint("to_reverse", "lr.input", "IntList", Direction.IN)
        mgraph.add_endpoint("reversed", "lr.output", "IntList", Direction.OUT)

    if gen_mode != "s" and "v" in gen_mode:
        mgraph.add_endpoint("original", "lrv.original_data_input", "IntList", Direction.IN)
        mgraph.add_endpoint("reversed", "lrv.reversed_data_input", "IntList", Direction.IN)

    lr_app = App(modulegraph=mgraph, host=host, name=nickname)

    return lr_app
