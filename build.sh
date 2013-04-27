#!/bin/sh

grab_modules() {
	# echo Grabbing npm modules
    npm install keypress
}

if [ "$1" = "clean" ]; then
		echo 'Cleaning'
		rm -rf node_modules
		rm -rf build
		exit
fi


if [ ! -d "node_modules" ];
	then
		grab_modules
fi

node-gyp configure build
