import pytest
import os
import re
import urllib.request
import copy

import integrationtest.log_file_checks as log_file_checks
import integrationtest.basic_checks as basic_checks
import integrationtest.data_classes as data_classes
from integrationtest.verbosity_helper import IntegtestVerbosityLevels

import functools
print = functools.partial(print, flush=True)  # always flush print() output

pytest_plugins = "integrationtest.integrationtest_drunc"

# Values that help determine the running conditions
run_duration = 20  # seconds

# Default values for validation parameters
check_for_logfile_errors = True
expected_event_count = run_duration * 10
expected_event_count_tolerance = expected_event_count / 10

excluded_substring_map = {
    "-controller": [
        'ERROR.*Broadcast:.*Propagating take_control to children',
        'ERROR.*Broadcast:.*Propagating describe to children',
        'WARNING.*Broadcast:.*There is no broadcasting service!',
        "Worker with pid \\d+ was terminated due to signal",
    ],
    "local-connection-server": [
        "errorlog: -",
        "Worker with pid \\d+ was terminated due to signal",
    ],
    "listrev": ["connect: Connection refused"]
}

# The arguments to pass to the config generator, excluding the json
# output directory (the test framework handles that)

common_config_obj = data_classes.drunc_config()
common_config_obj.session = "lr-session"

single_app_conf = copy.deepcopy(common_config_obj)
single_app_conf.config_db = os.path.dirname(__file__) + "/../config/lrSession-singleapp.data.xml"
v_conf = copy.deepcopy(common_config_obj)
v_conf.config_db = os.path.dirname(__file__) + "/../config/lrSession-v.data.xml"
g_conf = copy.deepcopy(common_config_obj)
g_conf.config_db = os.path.dirname(__file__) + "/../config/lrSession-g.data.xml"
r_conf = copy.deepcopy(common_config_obj)
r_conf.config_db = os.path.dirname(__file__) + "/../config/lrSession-r.data.xml"
separate_conf = copy.deepcopy(common_config_obj)
separate_conf.config_db = (
    os.path.dirname(__file__) + "/../config/lrSession-separate.data.xml"
)
multigen_conf = copy.deepcopy(common_config_obj)
multigen_conf.config_db = os.path.dirname(__file__) + "/../config/lrSession-gg.data.xml"

large_conf = copy.deepcopy(common_config_obj)
large_conf.config_db = os.path.dirname(__file__) + "/../config/lrSession.data.xml"

confgen_arguments = {
    "Single App": single_app_conf,
    "Separate Verifier": v_conf,
    "Separate Generator": g_conf,
    "Separate Reverser": r_conf,
    "Independent Apps": separate_conf,
    "Multiple Generators": multigen_conf,
    "Large System": large_conf,
}
# The commands to run in dunerc, as a list
dunerc_command_list = (
    "boot wait 5 conf start wait 1 enable-triggers wait ".split()
    + [str(run_duration)]
    + "disable-triggers wait 2 drain-dataflow wait 2 stop-trigger-sources stop scrap terminate".split()
)

# The tests themselves


def test_dunerc_success(run_dunerc, caplog):
    # check for run control success, problems during pytest setup, etc.
    basic_checks.basic_checks(run_dunerc, caplog, print_test_name=True)


def test_log_files(run_dunerc):
    if check_for_logfile_errors:
        # Check that there are no warnings or errors in the log files
        assert log_file_checks.logs_are_error_free(
            run_dunerc.log_files, excluded_substring_map=excluded_substring_map
        )

    # Exiting do_stop() method, generated 2081 lists, and sent 2081 list messages DAQModule: rdlg0
    generator_generated = 0
    generator_sent = 0
    # Exiting do_stop() method, received 2081 request messages, sent 2081, received 2081 lists, and sent 2081 reversed list messages DAQModule: lr0
    reverser_received = 0
    reverser_sent = 0
    # Exiting do_stop() method, received 2081 reversed list messages, compared 2081 reversed lists to their original data, and found 0 mismatches.  DAQModule: lrv
    validator_received = 0
    validator_received_reversed = 0
    validator_errors = 999

    for idx in range(len(run_dunerc.log_files)):
        for line in open(run_dunerc.log_files[idx], errors='ignore').readlines():
            if "Exiting do_stop" in line:
                if "RandomDataListGenerator" in line:
                    m = re.search(
                        "generated ([0-9]+) lists, and sent ([0-9]+) list messages",
                        line,
                    )
                    generator_generated = int(m.group(1))
                    generator_sent = int(m.group(2))
                if "ListReverser" in line:
                    m = re.search(
                        "received ([0-9]+) request messages, sent ([0-9]+), received ([0-9]+) lists, and sent ([0-9]+) reversed list messages",
                        line,
                    )
                    reverser_received = int(m.group(3))
                    reverser_sent = int(m.group(4))
                if "ReversedListValidator" in line:
                    m = re.search(
                        "received ([0-9]+) reversed list messages, compared ([0-9]+) reversed lists to their original data, and found ([0-9]+) mismatches.",
                        line,
                    )
                    validator_received = int(m.group(2))
                    validator_received_reversed = int(m.group(1))
                    validator_errors = int(m.group(3))

    if run_dunerc.verbosity_helper.compare_level(IntegtestVerbosityLevels.drunc_transitions):
        print()
        print(f"Checking number of generated lists: {generator_generated} >= {expected_event_count - expected_event_count_tolerance}")
    assert generator_generated >= expected_event_count - expected_event_count_tolerance
    if run_dunerc.verbosity_helper.compare_level(IntegtestVerbosityLevels.drunc_transitions):
        print(f"Checking number of sent lists by generator: {generator_sent} >= {expected_event_count - expected_event_count_tolerance}")
    assert generator_sent >= expected_event_count - expected_event_count_tolerance
    if run_dunerc.verbosity_helper.compare_level(IntegtestVerbosityLevels.drunc_transitions):
        print(f"Checking number of lists recived by reverser: {reverser_received}")
    assert (
        reverser_received >= expected_event_count - expected_event_count_tolerance
    )  # times num generators?
    if run_dunerc.verbosity_helper.compare_level(IntegtestVerbosityLevels.drunc_transitions):
        print(f"Checking number of reversed lists sent by reverser: {reverser_sent}")
    assert reverser_sent >= expected_event_count - expected_event_count_tolerance
    if run_dunerc.verbosity_helper.compare_level(IntegtestVerbosityLevels.drunc_transitions):
        print(f"Checking number of lists received by validator: {validator_received}")
    assert (
        validator_received >= expected_event_count - expected_event_count_tolerance
    )  # times num generators?
    if run_dunerc.verbosity_helper.compare_level(IntegtestVerbosityLevels.drunc_transitions):
        print(f"Checking number of reversed lists received by validator: {validator_received_reversed}")
    assert (
        validator_received_reversed
        >= expected_event_count - expected_event_count_tolerance
    )
    if run_dunerc.verbosity_helper.compare_level(IntegtestVerbosityLevels.drunc_transitions):
        print(f"Checking number of validator errors is 0: {validator_errors}")
    assert validator_errors == 0
    if run_dunerc.verbosity_helper.compare_level(IntegtestVerbosityLevels.drunc_transitions):
        print()
