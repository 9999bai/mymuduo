#!/bin/bash

set -e

#cd /home/brian/muduo_lib/build
#cmake .. && make

#cd ..


#如果没有build目录则创建
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

# rm -rf `pwd`/build/*

# cd `pwd`/build && 
#     cmake .. &&
#     make
    
#回到项目根目录
# cd ..

#把头文件拷贝到 /usr/include/mymuduo  so库拷贝到 /usr/lib/mymuduo  PATH
if [ ! -d /usr/include/mymuduo ]; then
    mkdir /usr/include/mymuduo
fi

# if [ ! -d /usr/lib/mymuduo ]; then
#     mkdir /usr/lib/mymuduo
# fi

for header in `ls *.h`
do
    cp $header /usr/include/mymuduo
done

cp `pwd`/lib/libmymuduo.so /usr/lib/mymuduo


# cd UdpClientExample
# make clean && make


ldconfig
