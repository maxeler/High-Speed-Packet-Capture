all-simulation:
	make -C ./ORIG
	make -C ./APP/ all-simulation

test: all-simulation
	cd TESTS && sudo -E ./run_tests.sh

