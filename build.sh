#!/bin/bash
export NAME=BR4096

rm -rf release
mkdir -p release
make clean ; make intro.laturi SCR_W=1920 SCR_H=1080
cp intro.laturi release/${NAME}_1920_1080_compo.command
make clean ; make intro.laturi SCR_W=1280 SCR_H=720
cp intro.laturi release/${NAME}_1280_720_compo.command
make clean ; make intro.laturi SCR_W=1280 SCR_H=800
cp intro.laturi release/${NAME}_1280_800_compo.command

make clean ; make intro.compat SCR_W=1920 SCR_H=1080
cp intro.compat release/${NAME}_1920_1080_compatibility
make clean ; make intro.compat SCR_W=1280 SCR_H=720
cp intro.compat release/${NAME}_1280_720_compatibility
make clean ; make intro.compat SCR_W=1280 SCR_H=800
cp intro.compat release/${NAME}_1280_800_compatibility

make clean

cp screenshot.jpg release
cp screenshot_small.png release
cp infofile.txt release

rm -rf ${NAME} ${NAME}.zip
mv release ${NAME}

zip -r ${NAME}.zip ${NAME}

rm -rf ${NAME}
