if [ $# == '0' ]; then
	qemu-system-x86_64 -L . -m 256 -fda Disk.img -hda HDD.img -boot a -localtime -M pc -serial tcp::4444,server,nowait -smp 2
elif [ $1 == "hdd" ]; then
	qemu-system-x86_64 -L . -m 64 -fda Disk.img -hda HDD.img -boot a -localtime -M pc -serial tcp::4444,server,nowait -smp 16
elif [ $1 == "rdd" ]; then
	qemu-system-x86_64 -L . -m 64 -fda Disk.img -boot a -localtime -M pc -serial tcp::4444,server,nowait -smp 16
fi
