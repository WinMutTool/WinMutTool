#!/bin/bash

if [ ! -d original ]; then
	mkdir original
fi

ALGOS=(WinMut WinMutNo AccMut AccMutNo validate timing)

setup() {
	URL=${1}
	TARNAME=${2}
	PROJECTNAME=${3}
	DIRNAME=${4}
	echo "Setting up ${PROJECTNAME}"
	if [ ! -f "original/${TARNAME}" ]; then
		echo "...downloading archive"
		curl -L ${URL} -o "original/${TARNAME}" || (echo "Failed to download ${PROJECTNAME}"; exit -1)
	fi
	for ALGO in ${ALGOS[@]}; do
		if [ ! -d ${PROJECTNAME}/${ALGO} ]; then
			if [ ! -d ${PROJECTNAME} ]; then
				mkdir ${PROJECTNAME}
			fi
			echo "...unpacking archive"
			tar xvf "original/${TARNAME}" -C "${PROJECTNAME}" > /dev/null || (echo "Failed to unpack the archive for ${PROJECTNAME}"; exit -1)
			if [ -f "patch/${PROJECTNAME}.patch" ]; then
				pushd "${PROJECTNAME}/${DIRNAME}" > /dev/null
				patch -p1 < "../../patch/${PROJECTNAME}.patch" #> /dev/null
				popd > /dev/null
			fi
			mv "${PROJECTNAME}/${DIRNAME}" "${PROJECTNAME}/${ALGO}"
		fi
	done
	echo "...copying config file"
	cp "config/${PROJECTNAME}.sh" "${PROJECTNAME}/config.sh"
}

setup https://ftp.gnu.org/gnu/grep/grep-3.4.tar.xz grep-3.4.tar.xz grep grep-3.4
setup https://sourceforge.net/projects/libpng/files/libpng16/1.6.37/libpng-1.6.37.tar.xz/download libpng-1.6.37.tar.xz libpng libpng-1.6.37
setup https://ftp.gnu.org/gnu/binutils/binutils-2.33.1.tar.xz binutils-2.33.1.tar.xz binutils binutils-2.33.1
setup https://ftp.gnu.org/gnu/coreutils/coreutils-8.9.tar.xz coreutils-8.9.tar.xz coreutils coreutils-8.9
setup https://ftp.gnu.org/gnu/gmp/gmp-6.1.2.tar.xz gmp-6.1.2.tar.xz gmp gmp-6.1.2
setup https://download.libsodium.org/libsodium/releases/libsodium-1.0.18.tar.gz libsodium-1.0.18.tar.gz libsodium libsodium-1.0.18
setup https://github.com/lz4/lz4/archive/v1.9.2.tar.gz lz4-1.9.2.tar.gz lz4 lz4-1.9.2
setup https://ftp.pcre.org/pub/pcre/pcre2-10.33.tar.gz pcre2-10.33.tar.gz pcre2 pcre2-10.33
setup http://www.lua.org/ftp/lua-5.3.4.tar.gz lua-5.3.4.tar.gz lua lua-5.3.4
setup http://ffmpeg.org/releases/ffmpeg-4.1.5.tar.xz ffmpeg-4.1.5.tar.xz ffmpeg ffmpeg-4.1.5

# additional settings for lua

if [ ! -f "original/lua-5.3.4-tests.tar.gz" ]; then
	echo "...downloading tests for lua"
	curl -L http://www.lua.org/tests/lua-5.3.4-tests.tar.gz -o original/lua-5.3.4-tests.tar.gz
fi

for ALGO in ${ALGOS[@]}; do
	pushd lua/${ALGO} > /dev/null
	if [ ! -d "tests" ]; then
		tar xvf "../../original/lua-5.3.4-tests.tar.gz" -C . > /dev/null
		mv lua-5.3.4-tests tests
	fi
	popd > /dev/null
done


