import json
import os
import rich.traceback
from rich.console import Console
from os.path import exists, join


CLOCK_SPEED_HZ = 50000000;

# Add -h as default help option
CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

console = Console()

def generate_boot( lr1_spec: dict, lr2_spec: dict) -> dict:
    """
    Generates boot informations
    
    
    Args:
        lr1_spec (dict): Description
        lr2_spec (dict): Description
    
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
            lr1_spec['name']: {
                "exec": "daq_application",
                "host": "host_lr1",
                "port": lr1_spec["port"]
            },
            lr2_spec["name"]: {
                "exec": "daq_application",
                "host": "host_lr2",
                "port": lr2_spec["port"]
            }
        },
        "hosts": {
            "host_lr1": lr1_spec["host"],
            "host_lr2": lr2_spec["host"]
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
@click.argument('json_dir', type=click.Path())
def cli(host, json_dir):
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

    app_lr1="lr1"
    app_lr2="lr2"

    cmd_set = ["init", "conf", "start", "stop", "pause", "resume", "scrap"]
    for app,data in ((app_lr1, cmd_data_lr),(app_lr2, cmd_data_lr)):
        console.log(f"Generating {app} command data json files")
        for c in cmd_set:
            with open(f'{join(data_dir, app)}_{c}.json', 'w') as f:
                json.dump(data[c].pod(), f, indent=4, sort_keys=True)


    console.log(f"Generating top-level command json files")
    # start_order = [app_rudf, app_trgemu]
    for c in cmd_set:
        with open(join(json_dir,f'{c}.json'), 'w') as f:
            cfg = {
                "apps": { app: f'data/{app}_{c}' for app in (app_lr1, app_lr2) }
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
            lr1_spec = {
                "name": app_lr1,
                "host": host,
                "port": 3333
            },
            lr2_spec = {
                "name": app_lr2,
                "host": host,
                "port": 3334
            },
        )
        json.dump(cfg, f, indent=4, sort_keys=True)
    console.log(f"MDAapp config generated in {json_dir}")


if __name__ == '__main__':

    try:
        cli(show_default=True, standalone_mode=True)
    except Exception as e:
        console.print_exception()
