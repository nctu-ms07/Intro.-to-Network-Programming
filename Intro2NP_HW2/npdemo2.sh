#!/bin/bash

CLIENT=$1
SERVER_IP=$2
SERVER_PORT=$3
SESSION="np_demo"
SLEEP_TIME=0.3
if [ -z ${SERVER_IP} ] || [ -z ${SERVER_PORT} ]; then
    echo "Usage: $0 <client> <server ip> <server port>"
    exit 1
fi

if [ -n "`tmux ls | grep ${SESSION}`" ]; then
  tmux kill-session -t $SESSION
fi

tmux new-session -d -s $SESSION
tmux set remain-on-exit on

tmux select-pane -t 0
tmux split-window -v
tmux split-window -h -p 50

tmux select-pane -t 0
tmux split-window -h -p 50

echo "Connection"
for i in $(seq 0 3)
do
    tmux send-keys -t ${i} "${CLIENT} ${SERVER_IP} ${SERVER_PORT}" Enter
    sleep 0.5
done

echo "Registeration and Login"
for i in $(seq 0 2)
do
    tmux send-keys -t ${i} "register user${i} user${i}@qwer.zxcv user${i}" Enter
    sleep $SLEEP_TIME
done

for i in $(seq 0 2)
do
	tmux send-keys -t ${i} "login user${i} user${i}" Enter 
    sleep $SLEEP_TIME
done

echo "BBS function"

for i in $(seq 0 0)
do

	index=0
    tmux send-keys -t ${index} "create-board NP_HW" Enter 
    sleep $SLEEP_TIME
	
	index=1
	tmux send-keys -t ${index} "create-board NP_test" Enter 
    sleep $SLEEP_TIME
	
	index=2
    tmux send-keys -t ${index} "create-board NP_HW" Enter #!版存在
    sleep $SLEEP_TIME
	
	index=2
    tmux send-keys -t ${index} "create-board OS_HW" Enter 
    sleep $SLEEP_TIME
	index=3
    tmux send-keys -t ${index} "create-board NCTU" Enter 
    sleep $SLEEP_TIME
	
	
	index=0
    tmux send-keys -t ${index} "create-board NP_HW2" Enter 
    sleep $SLEEP_TIME
	
	index=1
    tmux send-keys -t ${index} "list-board" Enter 
    sleep $SLEEP_TIME
	index=3
    tmux send-keys -t ${index} "list-board" Enter 
    sleep $SLEEP_TIME
	
	
	index=0
	tmux send-keys -t ${index} "create-post NP_HW --title TA About NP HW1 --content NP project1<br>Make a BBS<br>Deadline:4/27" Enter
	sleep $SLEEP_TIME
	index=1
	tmux send-keys -t ${index} "create-post NP_HW2 --title NP HW1 deadline --content Help!<br>Does anyone know when the deadline is?" Enter
	sleep $SLEEP_TIME
	index=3
	tmux send-keys -t ${index} "create-post NP_test --title NP --content NP" Enter
	sleep $SLEEP_TIME
	
	index=2
	tmux send-keys -t ${index} "create-post NP_HW --title Need Help! --content I can not understand HW1.<br>Please give me some suggestions." Enter
	sleep $SLEEP_TIME
	
	
	index=3
	tmux send-keys -t ${index} "list-post NP_HW" Enter
	sleep $SLEEP_TIME
	
	index=0
	tmux send-keys -t ${index} "create-post NP_HW --title TA HW1 TA_time --content NP project1<br>TA time:14:00to15:00" Enter
	sleep $SLEEP_TIME
	
	index=1
	tmux send-keys -t ${index} "create-post NCTU --title Ha Ha --content Ha<br>Ha" Enter
	sleep $SLEEP_TIME
	
	index=2
	tmux send-keys -t ${index} "list-post NCTU" Enter
	sleep $SLEEP_TIME
	index=3
	tmux send-keys -t ${index} "list-post NP_HW" Enter
	sleep $SLEEP_TIME
	
	
	index=0
	tmux send-keys -t ${index} "read 100" Enter
	sleep $SLEEP_TIME
	index=2
	tmux send-keys -t ${index} "read 2" Enter
	sleep $SLEEP_TIME
	index=1
	tmux send-keys -t ${index} "list-post NP_HW" Enter
	sleep $SLEEP_TIME
	
	index=3
	tmux send-keys -t ${index} "read 4" Enter
	sleep $SLEEP_TIME
	
	index=0
	tmux send-keys -t ${index} "comment 100 Ha Ha" Enter
	sleep $SLEEP_TIME
	
	
	index=2
	tmux send-keys -t ${index} "comment 2 You posted in the wrong Board!" Enter
	sleep $SLEEP_TIME
	index=3
	tmux send-keys -t ${index} "comment 4 Ha Ha" Enter
	sleep $SLEEP_TIME
	
	index=0
	tmux send-keys -t ${index} "delete-post 100" Enter
	sleep $SLEEP_TIME
	index=1
	tmux send-keys -t ${index} "delete-post 3" Enter
	sleep $SLEEP_TIME
	index=3
	tmux send-keys -t ${index} "delete-post 3" Enter
	sleep $SLEEP_TIME
	
	index=1
	tmux send-keys -t ${index} "delete-post 2" Enter
	sleep $SLEEP_TIME
	
	
	index=2
	tmux send-keys -t ${index} "read 2" Enter
	sleep $SLEEP_TIME
	index=3
	tmux send-keys -t ${index} "list-post NP_HW2" Enter
	sleep $SLEEP_TIME
	
	index=0
	tmux send-keys -t ${index} "update-post 100 --content Ha Ha" Enter
	sleep $SLEEP_TIME
	
	index=1
	tmux send-keys -t ${index} "update-post 4 --content Ha Ha" Enter
	sleep $SLEEP_TIME
	index=2
	tmux send-keys -t ${index} "read 4" Enter
	sleep $SLEEP_TIME
	
	index=3
	tmux send-keys -t ${index} "update-post 4 --title Ha Ha" Enter
	sleep $SLEEP_TIME
	
	index=2
	tmux send-keys -t ${index} "comment 4 Can extend the TA time?" Enter
	sleep $SLEEP_TIME
	
	index=0
	tmux send-keys -t ${index} "read 4" Enter
	sleep $SLEEP_TIME
	index=0
	tmux send-keys -t ${index} "update-post 4 --title TA HW1 TA_time::Update" Enter
	sleep $SLEEP_TIME
	index=0
	tmux send-keys -t ${index} "update-post 4 --content NP project1<br>TA time:14:00to17:00" Enter
	sleep $SLEEP_TIME
	index=0
	tmux send-keys -t ${index} "comment 4 OK^^" Enter
	sleep $SLEEP_TIME
	index=3
	tmux send-keys -t ${index} "read 4" Enter
	sleep $SLEEP_TIME
	
done

for i in $(seq 0 2)
do
	tmux send-keys -t ${i} "logout" Enter 
    sleep $SLEEP_TIME
	
done

for i in $(seq 0 3)
do
	tmux send-keys -t ${i} "exit" Enter 
    sleep $SLEEP_TIME
done

echo "Show result."
sleep 1
tmux attach-session -t $SESSION


