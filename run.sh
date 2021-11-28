#!/usr/bin/env sh
qemu-system-x86_64 -s -S -serial stdio -drive file=bin/img/chroma.img,index=0,media=disk,format=raw &
echo 'gdb -ex "file bin/kernel" -ex "target remote tcp:localhost:1234"'
wait
