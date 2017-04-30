#!/bin/bash

top=-1

if [ $1 == "-l" ]
then
	path=$2
	command=$4
	if [ $# -eq 5 ]
	then
		top=$5
	fi
elif [ $1 == "-c" ]
then
	command=$2
	if [ $# -eq 5 ]
	then
		top=$3
		path=$5
	else
		path=$4
	fi
fi 

if [ $command == "list" ]
then
	echo "List of job directories"
	find $path -maxdepth 1 -type d -name '*sdi1000155*'

elif [ $command == "purge" ]
then
	echo "Deleting job directories in ${path}"
	for x in $(find $path -type d -name '*sdi1000155*')
	do
		rm -r $x
	done

	#svise ola ta named pipes sto path
	echo "Deleting fifos in ${path}"
	find $path -type p -delete

	#an to path den itan to current dir . tote svise kai ta named pipes sto durrent dir .
	if [ $path != "./" ]
	then
		echo "Deleting fifos in current directory as well"
		find . -type p -delete
	fi

elif [ $command == "size" ]
then
	if [ $top -eq -1 ]
	then
		du $path -h --exclude="./" --max-depth=1 ${path}sdi1000155_* | cut -f 1 | sort -h
	else
		top=$((top+1 ))
		du $path -h --exclude="./" --max-depth=1 ${path}sdi1000155_* | cut -f 1 | sort -hr | head -$top
	fi


else
	echo "wrong command given"
fi
