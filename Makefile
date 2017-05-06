all:clean client sludge
	echo "Testing for the executables"
	echo "Preparing test archive file"
	./sludge -c test.dat
	echo "test1" > test1.txt
	echo "test2_asa" > test2.txt
	./sludge -a test.dat test1.txt
	./sludge -a test.dat test2.txt
	rm -f test1.txt test2.txt
	./sludge -a test.dat Makefile
	./sludge -l test.dat
	./sludge -r test.dat Makefile
	./sludge -d test.dat Makefile
	./sludge -e test.dat
	echo "Testing myFUSE with test archive file"
	mkdir -p testmnt
	./myfuse test.dat testmnt
	ls testmnt
	cat testmnt/test1.txt
	cat testmnt/test2.txt
#	echo "press any key and enter to end test"
#	@read line
#	fusermount -u testmnt
client:client.c
	gcc -Wall `pkg-config fuse --cflags --libs` client.c -o myfuse
sludge:sludge.c
	gcc -o sludge sludge.c
clean:
	rm -f *.txt *.dat sludge myfuse
#	rm -Rf testmnt
