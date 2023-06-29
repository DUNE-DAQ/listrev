import pytest
import os
import re
import urllib.request

import integrationtest.log_file_checks as log_file_checks

# Values that help determine the running conditions
run_duration=20  # seconds

# Default values for validation parameters
check_for_logfile_errors=True
expected_event_count=run_duration

# The next three variable declarations *must* be present as globals in the test
# file. They're read by the "fixtures" in conftest.py to determine how
# to run the config generation and nanorc

# The name of the python module for the config generation
confgen_name="listrev_gen"
# Don't require the --frame-file option, we don't need it
frame_file_required=False
# Don't create/require a detector readout map file
dro_map_required=False
# Determine if the Connectivity Service is available
use_connectivity_service = True
try:
  urllib.request.urlopen('http://localhost:5000').status
except:
  use_connectivity_service = False
# The arguments to pass to the config generator, excluding the json
# output directory (the test framework handles that)

single_app_conf={"detector": {"op_env": "integtest"},"boot": { "use_connectivity_service": use_connectivity_service}}
v_conf={"detector": {"op_env": "integtest"},"boot": { "use_connectivity_service": use_connectivity_service}, "listrev": {"apps": ["gr", "v"]}}
g_conf={"detector": {"op_env": "integtest"},"boot": { "use_connectivity_service": use_connectivity_service}, "listrev": {"apps": ["rv", "g"]}}
r_conf={"detector": {"op_env": "integtest"},"boot": { "use_connectivity_service": use_connectivity_service}, "listrev": {"apps": ["gv", "r"]}}
separate_conf={"detector": {"op_env": "integtest"},"boot": { "use_connectivity_service": use_connectivity_service}, "listrev": {"apps": ["g", "r", "v"]}}

confgen_arguments={"Single App": single_app_conf,
                   "Separate Verifier": v_conf,
                   "Separate Generator": g_conf,
                   "Separate Reverser": r_conf,
                   "Independent Apps": separate_conf}
# The commands to run in nanorc, as a list
nanorc_command_list="integtest-partition boot conf".split()
nanorc_command_list+="start_run --disable-data-storage 101 wait ".split() + [str(run_duration)] + "stop_run wait 2".split()
nanorc_command_list+="scrap terminate".split()

# The tests themselves

def test_nanorc_success(run_nanorc):
    current_test=os.environ.get('PYTEST_CURRENT_TEST')
    match_obj = re.search(r".*\[(.+)\].*", current_test)
    if match_obj:
        current_test = match_obj.group(1)
    current_test += ": NanoRC Success Check"
    banner_line = re.sub(".", "=", current_test)
    print()
    print(banner_line)
    print(current_test)
    print(banner_line)
    # Check that nanorc completed correctly
    assert run_nanorc.completed_process.returncode==0

def test_log_files(run_nanorc):
    current_test=os.environ.get('PYTEST_CURRENT_TEST')
    match_obj = re.search(r".*\[(.+)\].*", current_test)
    if match_obj:
        current_test = match_obj.group(1)
    current_test += ": Log File Check (event counts)"
    banner_line = re.sub(".", "=", current_test)
    print()
    print(banner_line)
    print(current_test)
    print(banner_line)
    if check_for_logfile_errors:
        # Check that there are no warnings or errors in the log files
        assert log_file_checks.logs_are_error_free(run_nanorc.log_files)

    # Exiting the do_work() method, generated 31 lists and successfully sent 62 copies.  DAQModule: rdlg
    generator_generated = 0
    generator_sent = 0
    # Exiting do_work() method, received 31 lists and successfully sent 31.  DAQModule: lr
    reverser_received = 0
    reverser_sent = 0
    # Exiting do_work() method, received 31 reversed lists, compared 31 of them to their original data, and found 0 mismatches.  DAQModule: lrv
    validator_received = 0
    validator_received_reversed = 0
    validator_errors = 999

    for idx in range(len(run_nanorc.log_files)):
        for line in open(run_nanorc.log_files[idx]).readlines():
            if "Exiting" in line:
                if "generated" in line:
                    m = re.search("generated ([0-9]+) lists and successfully sent ([0-9]+) copies",line)
                    generator_generated = int(m.group(1))
                    generator_sent = int(m.group(2))
                if "ListReverser" in line:
                    m = re.search("received ([0-9]+) lists and successfully sent ([0-9]+).",line)
                    reverser_received = int(m.group(1))
                    reverser_sent = int(m.group(2))
                if "mismatches" in line:
                    m = re.search("received ([0-9]+) reversed lists, compared ([0-9]+) of them to their original data, and found ([0-9]+) mismatches.",line)
                    validator_received = int(m.group(2))
                    validator_received_reversed = int(m.group(1))
                    validator_errors = int(m.group(3))
   
    assert generator_generated >= expected_event_count
    assert generator_sent >= expected_event_count * 2
    assert reverser_received >= expected_event_count
    assert reverser_sent >= expected_event_count
    assert validator_received >= expected_event_count
    assert validator_received_reversed >= expected_event_count
    assert validator_errors == 0
