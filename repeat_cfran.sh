./ns3 run "scenario-vr.cc --customDdluPort=40000  --e2PortBase=52604 --nGnbNodes=5 --ues=4 --enableIdealProtocol=true --RngRun=0 --suffix=_0" > cfran_0.log &
./ns3 run "scenario-vr.cc --customDdluPort=41000  --e2PortBase=52124 --nGnbNodes=5 --ues=4 --enableIdealProtocol=true --RngRun=1 --suffix=_1" > cfran_1.log &
./ns3 run "scenario-vr.cc --customDdluPort=42000  --e2PortBase=52224 --nGnbNodes=5 --ues=4 --enableIdealProtocol=true --RngRun=2 --suffix=_2" > cfran_2.log &
./ns3 run "scenario-vr.cc --customDdluPort=43000  --e2PortBase=52324 --nGnbNodes=5 --ues=4 --enableIdealProtocol=true --RngRun=3 --suffix=_3" > cfran_3.log &
./ns3 run "scenario-vr.cc --customDdluPort=44000  --e2PortBase=52424 --nGnbNodes=5 --ues=4 --enableIdealProtocol=true --RngRun=4 --suffix=_4" > cfran_4.log &
# ./ns3 run "scenario-vr.cc --customDdluPort=45000  --e2PortBase=52524 --nGnbNodes=5 --ues=4 --enableIdealProtocol=true --RngRun=5 --suffix=_5" > cfran_5.log &
# ./ns3 run "scenario-vr.cc --customDdluPort=46000  --e2PortBase=52624 --nGnbNodes=5 --ues=4 --enableIdealProtocol=true --RngRun=6 --suffix=_6" > cfran_6.log &
# ./ns3 run "scenario-vr.cc --customDdluPort=47000  --e2PortBase=52674 --nGnbNodes=5 --ues=4 --enableIdealProtocol=true --RngRun=7 --suffix=_7" > cfran_7.log &
# ./ns3 run "scenario-vr.cc --customDdluPort=48000  --e2PortBase=52684 --nGnbNodes=5 --ues=4 --enableIdealProtocol=true --RngRun=8 --suffix=_8" > cfran_8.log &
# ./ns3 run "scenario-vr.cc --customDdluPort=49000  --e2PortBase=52694 --nGnbNodes=5 --ues=4 --enableIdealProtocol=true --RngRun=9 --suffix=_9" > cfran_9.log &