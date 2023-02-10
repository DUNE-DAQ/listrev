# listrev

The listrev package allows to excercise the basic functioning of DAQ applications, through three simple DAQ Modules that operate on lists of numbers.

In order to run it, setup the runtime environment for the DAQ version you are using.

To generate a valid configuration file you can do the following:
```
curl -O https://raw.githubusercontent.com/DUNE-DAQ/listrev/develop/config/listrev_config.json
listrev_gen -c ./listrev_config.json listrev_conf
```
The `-h` option will show you the available configuration options; `listrev_config.json` contains some basic settings to control the number of integers generated in each event and the frequency of each event. 

A directory *listrev_conf* will be created in your working directory. You can then [pass it to nanorc](https://dune-daq-sw.readthedocs.io/en/latest/packages/nanorc/) as you would any other configuration.

It will be possible to monitor the output of the application in the log file (created in the working directory) and operational monitoring (either a file in your working directory or grafana, depending on how you configured the system).
