tc qdisc del dev vlc0 root
tc qdisc add dev vlc0 root handle 1:0 netem delay 200ms
tc qdisc add dev vlc0 parent 1:1 handle 10: tbf rate 32bps limit 36  burst 100
