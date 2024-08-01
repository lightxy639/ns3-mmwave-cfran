./ns3 run "scenario-vr.cc --customDdluPort=40000  --e2PortBase=54604 --nGnbNodes=5 --ues=2 --enableIdealProtocol=true --RngRun=0 --suffix=_60" > cfran_0.log &
./ns3 run "scenario-vr.cc --customDdluPort=41000  --e2PortBase=53124 --nGnbNodes=5 --ues=2 --enableIdealProtocol=true --RngRun=1 --suffix=_61" > cfran_1.log &
./ns3 run "scenario-vr.cc --customDdluPort=42000  --e2PortBase=53224 --nGnbNodes=5 --ues=2 --enableIdealProtocol=true --RngRun=2 --suffix=_62" > cfran_2.log &
./ns3 run "scenario-vr.cc --customDdluPort=43000  --e2PortBase=53324 --nGnbNodes=5 --ues=2 --enableIdealProtocol=true --RngRun=3 --suffix=_63" > cfran_3.log &
./ns3 run "scenario-vr.cc --customDdluPort=44000  --e2PortBase=53424 --nGnbNodes=5 --ues=2 --enableIdealProtocol=true --RngRun=4 --suffix=_64" > cfran_4.log &
# ./ns3 run "scenario-vr.cc --customDdluPort=45000  --e2PortBase=53524 --nGnbNodes=5 --ues=5 --enableIdealProtocol=true --RngRun=5 --suffix=65" > cfran_5.log &
# ./ns3 run "scenario-vr.cc --customDdluPort=46000  --e2PortBase=53624 --nGnbNodes=5 --ues=5 --enableIdealProtocol=true --RngRun=6 --suffix=66" > cfran_6.log &
# ./ns3 run "scenario-vr.cc --customDdluPort=47000  --e2PortBase=53674 --nGnbNodes=5 --ues=5 --enableIdealProtocol=true --RngRun=7 --suffix=67" > cfran_7.log &
# ./ns3 run "scenario-vr.cc --customDdluPort=48000  --e2PortBase=53684 --nGnbNodes=5 --ues=5 --enableIdealProtocol=true --RngRun=8 --suffix=68" > cfran_8.log &   
# ./ns3 run "scenario-vr.cc --customDdluPort=49000  --e2PortBase=53694 --nGnbNodes=5 --ues=5 --enableIdealProtocol=true --RngRun=9 --suffix=69" > cfran_9.log &