#! /bin/bash

MODULE="phonebook"
MODE="666"


PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
cd ${PARENT_PATH}

set -e

if [ $# -ne 1 ]; then
	echo "wrong argument number, there should be only 1 argument"
	exit 1
fi



if [ $1 == "load" ]; then

	sudo insmod ${MODULE}.ko
	
	sudo rm -f /dev/${MODULE}

	MAJOR=$(awk -v module="$MODULE" '$2==module {print $1}' /proc/devices)
	sudo mknod /dev/${MODULE} c $MAJOR 0

	sudo chmod ${MODE} /dev/${MODULE}

elif [ $1 == "unload" ]; then

	sudo rmmod ${MODULE}.ko

	sudo rm -f /dev/${MODULE}

else

	echo "wrong command"
	echo "supported commands: load, unload"
	exit 1

fi
