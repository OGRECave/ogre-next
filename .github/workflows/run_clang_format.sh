#!/bin/bash

# THIS SCRIPT CAN BE USED TO COMPARE AGAINST PR BASE COMMIT
# OR PREVIOUS COMMIT (WHEN PUSH)
#
# NOT CURRENTLY IN USE BY OGRE-NEXT

eventName=$1
hashCommit=$2

echo "--- Fetching commits to base PR ---"
if [[ "$eventName" == "push" ]]; then
	git fetch --no-tags --prune --progress --no-recurse-submodules --deepen=1
	hashCommit=`git rev-parse HEAD~1`
else
	# Pull Request
	prCommits=`gh pr view $prId --json commits | jq '.commits | length'`
	fetchDepthToPrBase=`expr $prCommits + 2`
	echo "fetchDepthToPrBase: $fetchDepthToPrBase"
	git fetch --no-tags --prune --progress --no-recurse-submodules --deepen=$fetchDepthToPrBase
fi

echo "--- Running Clang Format Script ---" 
python3 run_clang_format.py $hashCommit || exit $?

echo "Done!"
