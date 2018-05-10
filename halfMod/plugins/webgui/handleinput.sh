#!/bin/bash

if ! [ -f "./halfMod/plugins/webgui/hex2dec" ]; then
    cd halfMod/plugins/webgui/
    source "build_hex2dec.sh"
    cd ../../..
fi

if [[ "$1" == "--decode" ]]; then
    shift 1
    decode=true
fi

if [ -f "./halfMod/plugins/webgui/post.${6}" ]; then
    post=`cat "./halfMod/plugins/webgui/post.${6}"`
    if [[ $decode ]]; then
        post=`echo $post | sed 's/&/ /'`
        post=`./halfMod/plugins/webgui/hex2dec --code "$post"`
    fi
fi

ip="$1"
name="$2"
flags="$3"
host="$4"
mark=`./halfMod/plugins/webgui/hex2dec $5`
port="$6"
cookie=`./halfMod/plugins/webgui/hex2dec $7`
if [[ "${cookie}x" != "x" ]]; then
    user=$cookie
    user=${user%%=*}
fi

#while [[ $# -gt 4 ]]; do
#    post+=( `./halfMod/plugins/webgui/hex2dec $5` )
#    shift 1
#done

