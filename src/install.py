#!/usr/bin/env python3

import os
import shutil
import sys
import argparse
os.chdir(os.path.dirname(sys.argv[0]))
parser = argparse.ArgumentParser(prog='AlfredInstall',description='Intall the alfred event based assistant. the isntallation process needs root',)
parser.add_argument('--remove',action='store_true',default=False)

if parser.parse_args().remove: # remove
	os.system("systemctl stop alfred-servant.service")
	os.system("systemctl disable alfred-servant.service")
	shutil.rmtree("/usr/local/lib/alfred")
	shutil.rmtree("/usr/local/share/alfred")
	os.remove("/usr/local/bin/alfred-cli")
	os.remove("/usr/local/bin/startalfred")
	os.remove("/etc/systemd/system/alfred-servant.service")
else : # install
	os.makedirs("/usr/local/lib/alfred",exist_ok=True)
	os.makedirs("/usr/local/share/alfred/addons",exist_ok=True)
	os.makedirs("/usr/local/share/alfred/classave",exist_ok=True)
	os.system("gcc -Wall -Wextra -I/usr/include/python3.8 --shared -fPIC -pthread libeventio/pyeventio.c -lrt -o /usr/local/lib/alfred/libeventio.so")
	shutil.copy("alfred.py","/usr/local/lib/alfred/alfred.py")
	shutil.copy("alfred-cli","/usr/local/bin/alfred-cli")
	os.chmod("/usr/local/bin/alfred-cli",int("101101101",base=2))
	with open("/usr/local/bin/startalfred","w") as fh:
		fh.write("""#!/bin/bash

python3 /usr/local/lib/alfred/alfred.py "$@" | systemd-cat -p info
""")
	os.chmod("/usr/local/bin/startalfred",int("111000000",base=2))
	with open("/etc/systemd/system/alfred-servant.service") as fh:
		fh.write("""[Unit]
Description=Alfred, the ultimate event based servant

[Service]
Type=simple
ExecStart=/usr/local/bin/startalfred
Restart=on-failure
RestartSec=10
KillMode=process

[Install]
WantedBy=multi-user.target
""")
	os.chmod("/etc/systemd/system/alfred-servant.service",int("110100000",base=2))
	os.system("systemctl daemon-reload")
	os.system("systemctl enable alfred-servant.service")
	os.system("systemctl start alfred-servant.service")
