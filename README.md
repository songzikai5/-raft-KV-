编译
cd build
cmake ..
make


cd ../bin

#运行raft节点
./raftCoreRun -n 节点数 -f 配置文件名

#运行客户端

./callerMain  -f 配置文件名
