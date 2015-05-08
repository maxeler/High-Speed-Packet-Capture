all-simulation:
	make -C ./ORIG
	make -C ./APP/ all-simulation

test: all-simulation
	cd TESTS && ./run_tests.sh
