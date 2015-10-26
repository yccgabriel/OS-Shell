all: clean phase2

phase2:
	@ g++ -Wall -std=c++0x *.cpp -o phase2
	@ echo "Make completed, run \"./phase2\"\n"
#	@ ./phase2

clean:
	@ rm -f phase2