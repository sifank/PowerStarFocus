all:
	g++ -Wall -std=c++11 -I/usr/include -I/usr/include/libindi -c powerhub.cpp
	#g++ -Wall -std=c++11 -rdynamic powerhub.o -o indi_powerhub -lindidriver -lrt
	g++ -Wall -std=c++11 -rdynamic powerhub.o -o indi_powerhub -lindidriver

clean:
	rm -rf powerhub.o indi_powerhub

install:
	\cp -f indi_powerhub /usr/bin/
	\cp -f indi_powerhub.xml /usr/share/indi/

