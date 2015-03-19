.PHONY: all client server common clean all-simulation clean-simulation all-isca all-clean

all: all-simulation all-isca

all-isca: RUNRULE=ISCA
all-isca: server client	
clean-isca: RUNRULE=ISCA
clean-isca: clean

all-simulation: RUNRULE=SIMULATION
all-simulation: server client	
clean-simulation: RUNRULE=SIMULATION
clean-simulation: clean

clean:
	make -C ./server clean
	make -C ./client/CPUCode RUNRULE=$(RUNRULE) distclean
	make -C ./common clean

server: common
	make -C ./server

client: common
	-find ../PLATFORMS/$(RUNRULE)/Maxfiles/ -iname *.max.gz -exec gunzip {} +
	make -C ./client/CPUCode RUNRULE=$(RUNRULE) build

common: 
	make -C ./common
