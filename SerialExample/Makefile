serialPort:
# arm-linux-g++ main.cpp -o tcpserver -I /usr/include/mymuduo -L/usr/lib/ -lmymuduo -lpthread -std=c++11 -Wl,-rpath=/usr/lib/ 
#	arm-linux-g++ main.cpp -o serialPort -I /home/brian/muduo_lib/ -L/usr/lib/mymuduo -lmymuduo -lpthread -std=c++11
	arm-linux-g++ main.cpp -o serialPort -I /usr/include/mymuduo -L/usr/lib/mymuduo -lmymuduo -L/usr/lib/json-arm -ljsoncpp -lpthread -std=c++11

clean:
	rm -f serialPort