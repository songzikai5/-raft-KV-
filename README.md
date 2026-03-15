# 说明

需要在`./bin`文件下创建`.conf`文件

示例

`test.conf`

```
node0ip=127.0.1.1

node0port=24994

node1ip=127.0.1.1

node1port=24995

node2ip=127.0.1.1

node2port=24996
```

# **编译**

```bash
cd build && cmake .. && make
```

# 启动

```bash
cd ../bin

#运行raft节点
./raftCoreRun -n 节点数 -f 配置文件名

#运行客户端

./callerMain -f 配置文件名
```

