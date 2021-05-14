# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

# Load configuration types
import moo.otypes
moo.otypes.load_types('cmdlib/cmd.jsonnet')
moo.otypes.load_types('rcif/cmd.jsonnet')
moo.otypes.load_types('appfwk/cmd.jsonnet')
moo.otypes.load_types('appfwk/app.jsonnet')
moo.otypes.load_types('listrev/randomdatalistgenerator.jsonnet')

# Import new types
import dunedaq.cmdlib.cmd as bcmd # base command, 
import dunedaq.appfwk.cmd as cmd # AddressedCmd, 
import dunedaq.appfwk.app as app # AddressedCmd, 
import dunedaq.rcif.cmd as rccmd # Addressed run control Cmd, 
import dunedaq.listrev.randomdatalistgenerator  as rlg

from appfwk.utils import acmd, mcmd, mrccmd, mspec


def generate(
    ):

    cmd_data = {}


    # Define modules and queues
    queue_bare_specs = [
            app.QueueSpec(inst="orig1", kind='FollySPSCQueue', capacity=100),
            app.QueueSpec(inst="orig2", kind='FollySPSCQueue', capacity=100),
            app.QueueSpec(inst="giro1", kind='FollySPSCQueue', capacity=100),
        ]
    
    # Only needed to reproduce the same order as when using jsonnet
    queue_specs = app.QueueSpecs(sorted(queue_bare_specs, key=lambda x: x.inst))


    mod_specs = [
        mspec("rdlg", "RandomDataListGenerator", [
                        app.QueueInfo(name="q1", inst="orig1", dir="output"),
                        app.QueueInfo(name="q2", inst="orig2", dir="output"),
                    ]),

        mspec("lr", "ListReverser", [
                        app.QueueInfo(name="input", inst="orig1", dir="input"),
                        app.QueueInfo(name="output", inst="giro1", dir="output"),
                    ]),

        mspec("lrv", "ReversedListValidator", [
                        app.QueueInfo(name="reversed_data_input", inst="giro1", dir="input"),
                        app.QueueInfo(name="original_data_input", inst="orig2", dir="input"),
                    ]),
        ]

    cmd_data['init'] = app.Init(queues=queue_specs, modules=mod_specs)

    cmd_data['conf'] = acmd([
                        ("rdlg", rlg.ConfParams(
                   nIntsPerList = 10,
                   waitBetweenSendsMsec = 100))
            ])

    startpars = rccmd.StartParams(run=1, disable_data_storage=False)
    cmd_data['start'] = acmd([
        (".*", startpars),
    ])

    cmd_data['stop'] = acmd([
        (".*", None),

    ])

    cmd_data['pause'] = acmd([
        ("", None)
    ])

    resumepars = rccmd.ResumeParams(trigger_interval_ticks=50000000)
    cmd_data['resume'] = acmd([
        (".*", resumepars)
    ])

    cmd_data['scrap'] = acmd([
        ("", None)
    ])

    cmd_data['record'] = acmd([
        ("", None)
    ])

    return cmd_data