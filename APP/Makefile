.PHONY: all client server common clean all-simulation all-clean

all-simulation: RUNRULE=Simulation 
all-simulation: server client	
clean-simulation: RUNRULE=Simulation
clean-simulation: clean

clean:
	make -C ./server clean
	make -C ./client/CPUCode RUNRULE=$(RUNRULE) distclean
	make -C ./common clean

server: common
	make -C ./server

client: common
	make -C ./client/CPUCode RUNRULE=$(RUNRULE) build

common: 
	make -C ./common
