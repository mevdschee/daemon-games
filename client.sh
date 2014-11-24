#!/bin/bash
user_interrupt(){
stty icanon echo
echo -e "\033[0m"
}
stty -icanon -echo
trap user_interrupt SIGINT
trap '' SIGTSTP
nc $1 $2
user_interrupt
