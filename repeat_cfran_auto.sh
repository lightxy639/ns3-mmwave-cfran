#!/bin/bash

cd ~/ns3-mmwave-cfran

# 从文件中读取numBase
numBase=$(cat ~/last_numbase.txt)

# 读取repeatNum参数
repeatNum=$1

# 循环执行repeatNum次实验
for ((i = 0; i < repeatNum; i++))
do
    # 计算当前实验的numBase值
    currentNumBase=$((numBase + i))

    customDdluPortBase=$((40000 + i))
    # e2PortBase=$((56000 + 15 * i))
    e2PortBase=$((52000 + numBase % 3 * 100 + 15 * i))

    # 构造日志文件名
    logFile="cfran_${i}.log"
    
    
    # 运行实验，并将输出重定向到日志文件
    nohup ~/ns3-mmwave-cfran/ns3 run "scenario-vr.cc --customDdluPort=${customDdluPortBase} --e2PortBase=${e2PortBase} --nGnbNodes=5 --ues=6 --enableIdealProtocol=true --RngRun=${currentNumBase} --suffix=_${currentNumBase}" > "${logFile}" 2>&1 &
done

