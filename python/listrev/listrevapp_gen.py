# This module facilitates the generation of WIB modules within WIB apps

# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

# Load configuration types
import moo.otypes

moo.otypes.load_types('listrev/randomdatalistgenerator.jsonnet')

# Import new types
import dunedaq.listrev.randomdatalistgenerator  as rlg


from appfwk.app import App, ModuleGraph
from appfwk.daqmodule import DAQModule
from appfwk.conf_utils import Direction, Connection

#===============================================================================
def get_listrev_app(nickname, 
                host="localhost",
                n_ints=4,
                n_wait_ms=1000
                ):
    '''
    Here an entire application is generated. 
    '''
    # Define modules

    modules = []
    connections = {}
    connections['q1'] = Connection(f'lr.input',
                                           queue_name='orig1',
                                           queue_kind='FollyMPMCQueue',
                                           queue_capacity=1000)

    connections['q2'] = Connection(f'lrv.original_data_input',
                                           queue_name='orig2',
                                           queue_kind='FollyMPMCQueue',
                                           queue_capacity=1000)
    
    modules += [DAQModule(name = 'rdlg', 
                          plugin = 'RandomDataListGenerator',
                          connections=connections,
                          conf = rlg.ConfParams(
                                     nIntsPerList = n_ints,
                                     waitBetweenSendsMsec = n_wait_ms)
                             )]
    connections = {}
    connections['output'] = Connection(f'lrv.reversed_data_input',
                                           queue_name='giro1',
                                           queue_kind='FollyMPMCQueue',
                                           queue_capacity=1000)

    modules += [DAQModule(name = 'lr', plugin = 'ListReverser', connections=connections)]

    modules += [DAQModule(name = 'lrv', plugin = 'ReversedListValidator')]

    mgraph = ModuleGraph(modules)
    lr_app = App(modulegraph=mgraph, host=host, name=nickname)

    return lr_app
