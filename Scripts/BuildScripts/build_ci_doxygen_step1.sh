#!/bin/bash

if [ -z "$OGRE_VERSION" ]; then
	echo "OGRE_VERSION is not set. Aborting!"
	exit 1;
fi

# echo "--- Checking out gh-pages branch ---"
# git checkout gh-pages || exit $?
cd gh-pages || exit $?
cd api || exit $?
echo "--- Removing old ${OGRE_VERSION} ---"
git rm -rf ${OGRE_VERSION} || exit $?
rm -rf ${OGRE_VERSION} || exit $?
echo "--- Copying new ${OGRE_VERSION} ---"
mv ../../build/Doxygen/api/html ${OGRE_VERSION} || exit $?
git config user.email "github-actions"
git config user.name "github-actions@github.com"
echo "--- Adding to ${OGRE_VERSION} to git ---"
git add ${OGRE_VERSION} || exit $?
echo "--- Committing ---"
git commit -m "Deploy GH" || exit $?
echo "--- Pushing repo... ---"
git push || exit $?

echo "Done Step 1!"
