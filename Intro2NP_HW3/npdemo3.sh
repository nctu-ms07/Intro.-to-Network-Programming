#!/bin/bash

CLIENT=$1
SERVER_IP=$2
SERVER_PORT=$3
VENV=$4
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

if [ -n "${VENV}" ]; then
    echo "use venv"
    #echo  ${VENV}
    for i in $(seq 0 3)
    do
        tmux send-keys -t ${i} "source ${VENV}" Enter
        sleep 0.5
    done
fi
echo "Connection"
for i in $(seq 0 3)
do
    tmux send-keys -t ${i} "${CLIENT} ${SERVER_IP} ${SERVER_PORT}" Enter
    sleep 0.5
done

echo "Registeration and Login"
for i in $(seq 0 3)
do
    tmux send-keys -t ${i} "register user${i} user${i}@qwer.zxcv user${i}" Enter
    sleep $SLEEP_TIME
done

for i in $(seq 0 3)
do
	tmux send-keys -t ${i} "login user${i} user${i}" Enter 
    sleep $SLEEP_TIME
done


for i in $(seq 0 0)
do
    index=0
    tmux send-keys -t ${index} "create-chatroom 8888" Enter 
    sleep $SLEEP_TIME

    index=3
    tmux send-keys -t ${index} "create-chatroom 8889" Enter 
    sleep $SLEEP_TIME

    index=1
    tmux send-keys -t ${index} "join-chatroom user0" Enter
    sleep $SLEEP_TIME

    index=2
    tmux send-keys -t ${index} "join-chatroom user0" Enter
    sleep $SLEEP_TIME

    index=1
    tmux send-keys -t ${index} "I'm user ${index} (first)" Enter
    sleep $SLEEP_TIME

    index=2
    tmux send-keys -t ${index} "I'm user ${index} (first)" Enter
    sleep $SLEEP_TIME

    index=0
    tmux send-keys -t ${index} "I'm chatroom owner user ${index} (first)" Enter
    sleep $SLEEP_TIME


    index=3
    tmux send-keys -t ${index} "I'm chatroom owner user ${index} (first)" Enter
    sleep $SLEEP_TIME
    

    index=0
    tmux send-keys -t ${index} "detach" Enter
    sleep $SLEEP_TIME


    index=0
    tmux send-keys -t ${index} "join-chatroom user3" Enter
    sleep $SLEEP_TIME

    index=0
    tmux send-keys -t ${index} "hello, I'm user0 (first)" Enter
    sleep $SLEEP_TIME

    index=3
    tmux send-keys -t ${index} "hello, I'm user3 (first)" Enter
    sleep $SLEEP_TIME

    index=0
    tmux send-keys -t ${index} "leave-chatroom" Enter
    sleep $SLEEP_TIME

    index=3
    tmux send-keys -t ${index} "leave-chatroom" Enter
    sleep 3


    index=3
    tmux send-keys -t ${index} "restart-chatroom" Enter
    sleep $SLEEP_TIME

    index=3
    tmux send-keys -t ${index} "leave-chatroom" Enter
    sleep $SLEEP_TIME

    index=0
    tmux send-keys -t ${index} "logout" Enter
    sleep $SLEEP_TIME

    index=1
    tmux send-keys -t ${index} "pane0 can't show it(second)" Enter
    sleep $SLEEP_TIME
    
    index=1
    tmux send-keys -t ${index} "I'm user ${index} (second)" Enter
    sleep $SLEEP_TIME
    
    index=1
    tmux send-keys -t ${index} "I'm user ${index} (second)" Enter
    sleep $SLEEP_TIME

    index=2
    tmux send-keys -t ${index} "I'm user ${index} (second)" Enter
    sleep $SLEEP_TIME

    index=0
    tmux send-keys -t ${index} "attach" Enter
    sleep $SLEEP_TIME

    index=0
    tmux send-keys -t ${index} "I'm user ${index} (third)" Enter
    sleep $SLEEP_TIME

    index=1
    tmux send-keys -t ${index} "I'm user ${index} (third)" Enter
    sleep $SLEEP_TIME

    index=2
    tmux send-keys -t ${index} "I'm user ${index} (third)" Enter
    sleep $SLEEP_TIME


    index=2
    tmux send-keys -t ${index} "leave-chatroom" Enter
    sleep $SLEEP_TIME

    index=0
    tmux send-keys -t ${index} "leave-chatroom" Enter
    sleep 3

    index=1
    tmux send-keys -t ${index} "list-chatroom" Enter
    sleep $SLEEP_TIME

    index=0
    tmux send-keys -t ${index} "restart-chatroom" Enter
    sleep $SLEEP_TIME

    index=1
    tmux send-keys -t ${index} "list-chatroom" Enter
    sleep $SLEEP_TIME

    index=0
    tmux send-keys -t ${index} "leave-chatroom" Enter
    sleep $SLEEP_TIME

    index=1
    tmux send-keys -t ${index} "list-chatroom" Enter
    sleep $SLEEP_TIME

done

    
for i in $(seq 0 3)
do
	tmux send-keys -t ${i} "logout" Enter 
    sleep $SLEEP_TIME
    tmux send-keys -t ${i} "exit" Enter
done


echo "Show result."
sleep 1
tmux attach-session -t $SESSION