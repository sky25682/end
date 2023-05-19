kill -6 $(ps -a |grep qemu | awk '{print $1}')
