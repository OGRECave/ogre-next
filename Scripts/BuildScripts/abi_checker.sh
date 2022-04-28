#!/bin/bash

# Usage:
#
# Scripts/BuildScripts/abi_checker.sh 2 branch_name
#
# Where 2 is the lver
# branch_name is optional and is the name of the git branch

cd build/Debug/lib
mkdir -p AbiDump/$1

if [[ -z $2 || $2=="" ]]; then
	branch_name=`git branch --show-current`
else
	# Pull Requests can't use git branch, so Github provides it for us
	branch_name=$2
fi

#if [[ $branch_name == "master" && $1 != 1 ]]; then
#	echo "We're in master. Master does not do ABI checks. We're done."
#	exit
#fi

if [[ $1 != 1 ]]; then
	echo "--- Fetching base dumps to compare against ---"
	wget https://github.com/OGRECave/ogre-next/releases/download/bin-releases/AbiDumps_Ubuntu.18.04.LTS.$2.7z

	echo "--- Extracting base dumps ---"
	7z x AbiDumps_Ubuntu.18.04.LTS.$2.7z
fi

sudo apt-get install -y abi-compliance-checker

# Dump the libs
FILES="*.so*"
for file in $FILES
do
	# Ignore symlinks
	if ! [[ -L "$file" ]]; then
		echo "Dumping $file"
		abi-dumper "$file" -o "AbiDump/$1/$file.dump" -lver $1 &
	fi
done

wait

# Generate all reports. We need to gather their exit codes to see if they've failed
PIDs=()
for file in $FILES
do
	# Ignore symlinks
	if ! [[ -L "$file" ]]; then
		oldfile="${file%.*}.0"
		echo "Checking AbiDump/1/$oldfile.dump vs AbiDump/$1/$file.dump"
		abi-compliance-checker -l "${file%.*}" -old "AbiDump/1/$oldfile.dump" -new "AbiDump/$1/$file.dump" &
		PIDs+=($!)
	fi
done

EXIT_CODE=0
for pid in "${PIDs[@]}"
do
	echo Waiting for $pid
    wait "$pid"
	CODE="$?"
	if [[ "${CODE}" != "0" ]]; then
		echo "At least one report failed with exit code => ${CODE}";
		EXIT_CODE=1;
	fi
done

exit $EXIT_CODE