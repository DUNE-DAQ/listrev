# listrev

The listrev package allows to excercise the basic functioning of DAQ applications, through three simple DAQ Modules that operate on lists of numbers.

In order to run it, setup the runtime environment for the DAQ version you are using.

Then in `drunc-unified-shell` you can boot the example system included in the release with the command `boot config/lrSystem.data.xml lr-system`

If you want to modify the example system, you can copy it to your work directory and edit with `dbe_main -f lrSystem.data.xml` or your favourite text editor.
   ```
   cp $DUNE_DAQ_RELEASE_SOURCE/listrev/config/lrSystem.data.xml .
   ```


## Evaluating the listrev Run

  * `grep Exiting log_*lr-system_listrev*` will show the reported statistics.
  * The example is targeted at 100 Hz, so the expected number of messages seen by ReversedListValidator should be at least 100 times the run duration.
  * There should be three lists in each message (from the three generators), so it should report 300 times the run duration for the number of lists.
  * Messages are round-robined to the two reversers, so each should see 50run_duration messages and 150run_duration lists. They should have approximately equal values for the reported counters.
  * Generators should generate 100*run_duration lists and send all (or almost all) of them.