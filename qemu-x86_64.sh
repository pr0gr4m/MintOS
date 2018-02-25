if [ $# == '0' ]; then
	qemu-system-x86_64 -L . -m 64 -fda Disk.img -boot a -localtime -M pc -serial tcp::4444,server,nowait
elif [ $1 == "hdd" ]; then
	qemu-system-x86_64 -L . -m 64 -fda Disk.img -hda HDD.img -boot a -localtime -M pc
elif [ $1 == "rdd" ]; then
	qemu-system-x86_64 -L . -m 64 -fda Disk.img -boot a -localtime -M pc
fi
