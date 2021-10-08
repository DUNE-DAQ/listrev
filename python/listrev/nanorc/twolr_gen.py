import json
import os
import rich.traceback
from rich.console import Console
from os.path import exists, join


CLOCK_SPEED_HZ = 50000000;

# Add -h as default help option
CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

console = Console()

def generate_boot(lrs_spec: [dict]) -> dict:
    """
    Generates boot informations
    
    
    Args:
        lr_spec (dict): Description
    
    Returns:
        dict: Description
    
    
    """

    # TODO: think how to best handle this bit.
    # Who is responsible for this bit? Not minidaqapp?
    # Is this an appfwk configuration fragment?
    daq_app_specs = {
        "daq_application_ups" : {
            "comment": "Application profile based on a full dbt runtime environment",
            "env": {
               "DBT_AREA_ROOT": "getenv" 
            },
            "cmd": [
                "CMD_FAC=rest://localhost:${APP_PORT}",
                "INFO_SVC=file://info_${APP_ID}_${APP_PORT}.json",
                "cd ${DBT_AREA_ROOT}",
                "source dbt-setup-env.sh",
                "dbt-setup-runtime-environment",
                "cd ${APP_WD}",
                "daq_application --name ${APP_ID} -c ${CMD_FAC} -i ${INFO_SVC}"
            ]
        },
        "daq_application" : {
            "comment": "Application profile using  PATH variables (lower start time)",
            "env":{
                "CET_PLUGIN_PATH": "getenv",
                "DUNEDAQ_SHARE_PATH": "getenv",
                "LD_LIBRARY_PATH": "getenv",
                "PATH": "getenv"
            },
            "cmd": [
                "CMD_FAC=rest://localhost:${APP_PORT}",
                "INFO_SVC=file://info_${APP_NAME}_${APP_PORT}.json",
                "cd ${APP_WD}",
                "daq_application --name ${APP_NAME} -c ${CMD_FAC} -i ${INFO_SVC}"
            ]
        }
    }

    boot = {
        "env": {
            "DUNEDAQ_ERS_VERBOSITY_LEVEL": 1
        },
        "apps": {
            lr_spec['name']: {
                "exec": "daq_application",
                "host": "host_lr"+str(i),
                "port": lr_spec["port"]
            }
            for i, lr_spec in enumerate(lrs_spec)
            
        },
        "hosts": {
            "host_lr"+str(i): lr_spec["host"] for i, lr_spec in enumerate(lrs_spec)

        },
        "response_listener": {
            "port": 56789
        },
        "exec": daq_app_specs
    }

    console.log("Boot data")
    console.log(boot)
    return boot

import click

@click.command(context_settings=CONTEXT_SETTINGS)
@click.option('--host', default='localhost')
@click.option('--nlist', default=2)
@click.argument('json_dir', type=click.Path())
def cli(host, nlist, json_dir):
    """
      JSON_DIR: Json file output folder
    """
    console.log("Loading listrev config generator")
    from . import listrev_gen
    console.log(f"Generating configs for host listrev={host}")


    # network_endpoints={
    #     "trigdec" : "tcp://{host_trgemu}:12345",
    #     "triginh" : "tcp://{host_rudf}:12346",
    #     "timesync": "tcp://{host_rudf}:12347"
    # }


    cmd_data_lr = listrev_gen.generate()

    console.log("listrev cmd data:", cmd_data_lr)

    if exists(json_dir):
        raise RuntimeError(f"Directory {json_dir} already exists")

    data_dir = join(json_dir, 'data')
    os.makedirs(data_dir)

    apps_name=["lr"+str(i) for i in range(nlist)]

    cmd_set = ["init", "conf", "start", "stop", "pause", "resume", "scrap"]
    for app in apps_name:
        console.log(f"Generating {app} command data json files")
        for c in cmd_set:
            with open(f'{join(data_dir, app)}_{c}.json', 'w') as f:
                json.dump(cmd_data_lr[c].pod(), f, indent=4, sort_keys=True)


    console.log(f"Generating top-level command json files")
    # start_order = [app_rudf, app_trgemu]

    for c in cmd_set:
        with open(join(json_dir,f'{c}.json'), 'w') as f:
            cfg = {
                "apps": { app: f'data/{app}_{c}' for app in apps_name}
            }
            # if c == 'start':
            #     cfg['order'] = start_order
            # elif c == 'stop':
            #     cfg['order'] = start_order[::-1]
            # elif c in ('resume', 'pause'):
            #     del cfg['apps'][app_rudf]

            json.dump(cfg, f, indent=4, sort_keys=True)


    console.log(f"Generating boot json file")
    with open(join(json_dir,'boot.json'), 'w') as f:
        cfg = generate_boot(
            [{
                "name": apps_name[i],
                "host": host,
                "port": 3333+i
            } for i in range(nlist)]
        )
        json.dump(cfg, f, indent=4, sort_keys=True)
    console.log(f"MDAapp config generated in {json_dir}")


if __name__ == '__main__':

    try:
        cli(show_default=True, standalone_mode=True)
    except Exception as e:
        console.print_exception()
