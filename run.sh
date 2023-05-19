make clean
make
#./create_hdd.sh
qemu-system-x86_64 -L . -m 64 -fda ./DiskWithPackage.img -hda ./HDD.img --boot a -M pc -serial tcp::4444,server,nowait -smp 4

stty sane