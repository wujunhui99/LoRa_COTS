file l2b
target extended-remote localhost:3333
monitor reset halt
load
thbreak main
