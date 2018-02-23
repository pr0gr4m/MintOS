if [ $# == '0' ]; then
	qemu-system-x86_64 -L . -m 64 -fda Disk.img -boot a -localtime -M pc
else
	qemu-system-x86_64 -L . -m 64 -fda Disk.img -hda HDD.img -boot a -localtime -M pc
fi
