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

    # Define connections (queues)

    connections['output'] = Connection(f'trb_dqm.data_fragment_input_queue',
                                           queue_name='data_fragments_q',
                                           queue_kind='FollyMPMCQueue',
                                           queue_capacity=1000)


    # Define modules

    modules = []
    connections = {}
    connections['output'] = Connection(f'trb_dqm.data_fragment_input_queue',
                                           queue_name='data_fragments_q',
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
    connections['output'] = Connection(f'trb_dqm.data_fragment_input_queue',
                                           queue_name='data_fragments_q',
                                           queue_kind='FollyMPMCQueue',
                                           queue_capacity=1000)

    modules += [DAQModule(name = 'lr', plugin = 'ListReverser', connections=connections)]

    connections = {}
    connections['output'] = Connection(f'trb_dqm.data_fragment_input_queue',
                                           queue_name='data_fragments_q',
                                           queue_kind='FollyMPMCQueue',
                                           queue_capacity=1000)

    modules += [DAQModule(name = 'lrv', plugin = 'ReversedListValidator', connections=connections)]

    mgraph = ModuleGraph(modules)
    mgraph.add_endpoint(
    lr_app = App(modulegraph=mgraph, host=host, name=nickname)

    return lr_app
