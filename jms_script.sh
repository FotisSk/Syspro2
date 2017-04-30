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

echo "path:" ${path}
echo "command:" ${command}


if [ $command == "list" ]
then
	echo "list command"
	find $path -maxdepth 1 -type d -name '*sdi1000155*'
elif [ $command == "purge" ]
then
	echo "purge command"
	for x in $(find $path -type d -name '*sdi1000155*')
	do
		rm -r $x
	done
	
	#svise ola ta named pipes sto path
	find $path -type p -delete

	#an to path den itan to current dir . tote svise kai ta named pipes sto durrent dir .
	if [ $path != "." ]
	then
		echo "path was not current dir"
		find $path -type p -delete
	fi
elif [ $command == "size" ]
then
	echo "size command"
	echo $top
else
	echo "wrong command given"
fi
