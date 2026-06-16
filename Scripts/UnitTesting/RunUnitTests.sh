case "$OSTYPE" in
	darwin*)
		python3 RunUnitTests.py metal ../../build/bin/Debug/ ./JSON ../../build/UnitTestsOutput/
		;;
	*)
		# python3 RunUnitTests.py gl ../../build/Debug/bin/ ./JSON ../../build/UnitTestsOutput/ ../../build/UnitTestsOutput_old/
		python3 RunUnitTests.py vk ../../build/Debug/bin/ ./JSON ../../build/UnitTestsOutput/
		;;
esac
