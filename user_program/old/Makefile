all:
	gcc master.c -o master
	gcc slave.c -o slave
clean:
	rm -f newSlave newMaster
new:
	gcc newMaster.c -o newMaster
	gcc newSlave.c -o newSlave
newDEBUG:
	gcc newMaster.c -D DEBUG -o newMaster
	gcc newSlave.c -D DEBUG -o newSlave
