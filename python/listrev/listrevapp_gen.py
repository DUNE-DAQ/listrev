# This module facilitates the generation of WIB modules within WIB apps

# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io

moo.io.default_load_path = get_moo_model_path()

# Load configuration types
import moo.otypes

moo.otypes.load_types("listrev/listreverser.jsonnet")
moo.otypes.load_types("listrev/randomdatalistgenerator.jsonnet")
moo.otypes.load_types("listrev/reversedlistvalidator.jsonnet")

# Import new types
import dunedaq.listrev.listreverser as lr
import dunedaq.listrev.randomdatalistgenerator as rlg
import dunedaq.listrev.reversedlistvalidator as rlv


from daqconf.core.app import App, ModuleGraph
from daqconf.core.daqmodule import DAQModule
from daqconf.core.conf_utils import Endpoint, Direction

# ===============================================================================
def get_listrev_app(
    nickname,
    host="localhost",
    n_wait_ms=100,
    request_timeout_ms=1000,
    request_rate_hz=10,
    generator_indicies=[],
    reverser_indicies=[],
    has_validator=False,
    n_generators=1,
    n_reversers=1,
    n_ints_min=50,
    n_ints_max=200,
    n_reqs=100,
):
    """
    Here an entire application is generated.
    """
    # Define modules

    modules = []

    modules += [
            DAQModule(
                name=f"rdlg{gidx}",
                plugin="RandomDataListGenerator",
                conf=rlg.ConfParams(send_timeout_ms=n_wait_ms, request_timeout_ms=request_timeout_ms, generator_id=gidx),
            ) for gidx in generator_indicies
        ]

    modules += [
            DAQModule(
                name=f"lr{ridx}",
                plugin="ListReverser",
                conf=lr.ConfParams(
                    send_timeout_ms=n_wait_ms,
                    request_timeout_ms=request_timeout_ms,
                    num_generators=n_generators,
                    reverser_id=ridx
                ),
            ) for ridx in reverser_indicies
        ]

    if has_validator:
        modules += [
            DAQModule(
                name="lrv",
                plugin="ReversedListValidator",
                conf=rlv.ConfParams(
                    send_timeout_ms=n_wait_ms,
                    request_timeout_ms=request_timeout_ms,
                    request_rate_hz=request_rate_hz,
                    max_outstanding_requests=n_reqs,
                    num_reversers=n_reversers,
                    num_generators=n_generators,
                    min_list_size=n_ints_min,
                    max_list_size=n_ints_max
                ),
            )
        ]

    mgraph = ModuleGraph(modules)

    for gidx in generator_indicies:
        for ridx in range(n_reversers):
            mgraph.add_endpoint(f"lr{ridx}_list_connection", f"rdlg{gidx}.q{ridx}", "IntList", Direction.OUT)
        mgraph.add_endpoint(
            f"rdlg{gidx}_request_connection",
            f"rdlg{gidx}.request_input",
            "RequestList",
            Direction.IN
        )
        mgraph.add_endpoint(f"creates", f"rdlg{gidx}.create_input", "CreateList", Direction.IN, is_pubsub=True, toposort=False)

    for ridx in reverser_indicies:
        mgraph.add_endpoint(f"lr{ridx}_list_connection", f"lr{ridx}.list_input", "IntList", Direction.IN)
        mgraph.add_endpoint(f"validator_list_connection", f"lr{ridx}.output", "ReversedList", Direction.OUT)
        mgraph.add_endpoint(
            f"lr{ridx}_request_connection",
            f"lr{ridx}.request_input",
            "RequestList",
            Direction.IN
        )

        for gidx in range(n_generators):
            mgraph.add_endpoint(f"rdlg{gidx}_request_connection", f"lr{ridx}.request_output_{gidx}", "RequestList", Direction.OUT)

    if has_validator:
        mgraph.add_endpoint("validator_list_connection", "lrv.list_input", "ReversedList", Direction.IN)
        for ridx in range(n_reversers):
            mgraph.add_endpoint(
                f"lr{ridx}_request_connection",
                f"lrv.request_output_{ridx}",
                "RequestList",
                Direction.OUT
            )
        mgraph.add_endpoint(f"creates", "lrv.creates_out", "CreateList", Direction.OUT, is_pubsub=True, toposort=False)

    lr_app = App(modulegraph=mgraph, host=host, name=nickname)

    return lr_app
