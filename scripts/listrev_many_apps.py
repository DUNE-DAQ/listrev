import json
import argparse
import shutil
import os


parser = argparse.ArgumentParser(description='Process some integers.')
parser.add_argument('-n', '--num-apps', type=int, default=10, help='the number of listrev apps to create')
parser.add_argument('-f', '--file', type=str, default='boot.json', help='the boot.json file to read from')
args = parser.parse_args()


with open(args.file) as file:
    data = json.load(file)

new_apps = {}
new_order = []
dir_path = os.path.dirname(os.path.realpath(args.file))


for i in range(args.num_apps):
    new_apps[f'listrev-app-s-{i}'] = data['apps']['listrev-app-s']
    new_order.append(f'listrev-app-s-{i}')
    shutil.copy(os.path.join(dir_path, 'data', 'listrev-app-s_conf.json'), os.path.join(dir_path, 'data', f'listrev-app-s-{i}_conf.json'))
    shutil.copy(os.path.join(dir_path, 'data', 'listrev-app-s_init.json'), os.path.join(dir_path, 'data', f'listrev-app-s-{i}_init.json'))

os.remove(os.path.join(dir_path, 'data', f'listrev-app-s_conf.json'))
os.remove(os.path.join(dir_path, 'data', f'listrev-app-s_init.json'))

data['apps'] = new_apps
data['order'] = new_order

os.remove(args.file)

with open(args.file, 'w') as file:
    json.dump(data, file, indent=4)