# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

# Load configuration types
import moo.otypes
moo.otypes.load_types('rcif/cmd.jsonnet')
moo.otypes.load_types('appfwk/cmd.jsonnet')
moo.otypes.load_types('listrev/randomdatalistgenerator.jsonnet')

# Import new types
import dunedaq.appfwk.cmd as cmd # AddressedCmd, 
import dunedaq.rcif.cmd as rc # AddressedCmd, 
import dunedaq.listrev.randomdatalistgenerator  as rlg

from appfwk.utils import mcmd, mspec

import json
import math
# Time to waait on pop()
QUEUE_POP_WAIT_MS=100;
# local clock speed Hz
CLOCK_SPEED_HZ = 50000000;

def generate(
        NUMBER_OF_DATA_PRODUCERS=2,          
        DATA_RATE_SLOWDOWN_FACTOR = 10,
        RUN_NUMBER = 333, 
        TRIGGER_RATE_HZ = 1.0,
        DATA_FILE="./frames.bin",
        OUTPUT_PATH=".",
        DISABLE_OUTPUT=False
    ):

    # Define modules and queues
    queue_bare_specs = [
            cmd.QueueSpec(inst="orig1", kind='FollySPSCQueue', capacity=100),
            cmd.QueueSpec(inst="orig2", kind='FollySPSCQueue', capacity=100),
            cmd.QueueSpec(inst="giro1", kind='FollySPSCQueue', capacity=100),
        ]
    

    # Only needed to reproduce the same order as when using jsonnet
    queue_specs = cmd.QueueSpecs(sorted(queue_bare_specs, key=lambda x: x.inst))


    mod_specs = [
        mspec("rdlg", "RandomDataListGenerator", [
                        cmd.QueueInfo(name="q1", inst="orig1", dir="output"),
                        cmd.QueueInfo(name="q2", inst="orig2", dir="output"),
                    ]),

        mspec("lr", "ListReverser", [
                        cmd.QueueInfo(name="input", inst="orig1", dir="input"),
                        cmd.QueueInfo(name="output", inst="giro1", dir="output"),
                    ]),

        mspec("lrv", "ReversedListValidator", [
                        cmd.QueueInfo(name="reversed_data_input", inst="giro1", dir="input"),
                        cmd.QueueInfo(name="original_data_input", inst="orig2", dir="input"),
                    ]),
        ]

    init_specs = cmd.Init(queues=queue_specs, modules=mod_specs)

    jstr = json.dumps(init_specs.pod(), indent=4, sort_keys=True)
    print(jstr)

    initcmd = cmd.Command(
        id=cmd.CmdId("init"),
        data=init_specs
    )

    confcmd = mcmd("conf", [
                ("rdlg", rlg.Conf(
                   nIntsPerList = 10,
                   waitBetweenSendsMsec = 100))
            ])
    
    jstr = json.dumps(confcmd.pod(), indent=4, sort_keys=True)
    print(jstr)

    startpars = cmd.StartParams(run=RUN_NUMBER)
    startcmd = mcmd("start", [
            (".*", rc.StartParams(
                run=RUN_NUMBER,
              )),
        ])

    jstr = json.dumps(startcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nStart\n\n", jstr)

    emptypars = cmd.EmptyParams()

    stopcmd = mcmd("stop", [
            (".*", emptypars),
        ])

    jstr = json.dumps(stopcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nStop\n\n", jstr)

    scrapcmd = mcmd("scrap", [
            (".*", emptypars)
        ])

    jstr = json.dumps(scrapcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nScrap\n\n", jstr)

    # Create a list of commands
    cmd_seq = [initcmd, confcmd, startcmd, stopcmd, scrapcmd]

    # Print them as json (to be improved/moved out)
    jstr = json.dumps([c.pod() for c in cmd_seq], indent=4, sort_keys=True)
    return jstr
        
if __name__ == '__main__':
    # Add -h as default help option
    CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

    import click

    @click.command(context_settings=CONTEXT_SETTINGS)
    @click.option('-r', '--run-number', default=333)
    @click.argument('json_file', type=click.Path(), default='listrev-app.json')
    def cli(run_number, json_file):
        """
          JSON_FILE: Input raw data file.
          JSON_FILE: Output json configuration file.
        """

        with open(json_file, 'w') as f:
            f.write(generate(
                    RUN_NUMBER = run_number, 
                ))

        print(f"'{json_file}' generation completed.")

    cli()
    
