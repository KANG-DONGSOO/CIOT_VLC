tc qdisc del dev eth0 root
tc qdisc add dev eth0 root handle 1:0 netem delay 1s
tc qdisc add dev eth0 parent 1:1 handle 10: tbf rate 32bps limit 36  burst 100
