#!/bin/sh
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2016 INRIA and Alexandre Gauthier
#
# Natron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Natron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****

# MKJOBS: Number of threads
# CONFIG=(debug,release,relwithdebinfo): the build type
# DISABLE_BREAKPAD=1: When set, automatic crash reporting (google-breakpad support) will be disabled
# PLUGINDIR: The path to the plug-ins in the final bundle, e.g: "$CWD/build/Natron/App/Natron.app/Contents/Plugins"
#Usage PLUGINDIR="..." MKJOBS=4 CONFIG=relwithdebinfo BRANCH=workshop ./build-plugins.sh

source `pwd`/common.sh || exit 1

cd $CWD/build || exit 1

#If "workshop" is passed, use master branch for all plug-ins otherwise use the git tags in common.sh
if [ "$BRANCH" = "workshop" ]; then
    IO_BRANCH=master
    MISC_BRANCH=master
    ARENA_BRANCH=master
    CV_BRANCH=master
else
    IO_BRANCH=$IOPLUG_GIT_TAG
    MISC_BRANCH=$MISCPLUG_GIT_TAG
    ARENA_BRANCH=$ARENAPLUG_GIT_TAG
    CV_BRANCH=$CVPLUG_GIT_TAG
fi

if [ ! -d "$PLUGINDIR" ]; then
    echo "Error: plugin directory '$PLUGINDIR' does not exist"
    exit 1
fi

#Build openfx-io
git clone $GIT_IO
cd openfx-io || exit 1
git checkout "$IO_BRANCH" || exit 1
git submodule update -i --recursive || exit 1
if [ "$IO_BRANCH" = "master" ]; then
    # the snapshots are always built with the latest version of submodules
    git submodule foreach git pull origin master
fi

#Always bump git commit, it is only used to version-stamp binaries
IO_GIT_VERSION=`git log|head -1|awk '{print $2}'`
sed -i "" -e "s/IOPLUG_DEVEL_GIT=.*/IOPLUG_DEVEL_GIT=${IO_GIT_VERSION}/" $CWD/commits-hash.sh || exit 1

make CXX="$CXX" BITS=$BITS CONFIG=$CONFIG OCIO_HOME=/opt/local OIIO_HOME=/opt/local SEEXPR_HOME=/opt/local -j${MKJOBS} || exit 1

cp -r IO/$OS-$BITS-$CONFIG/IO.ofx.bundle "$PLUGINDIR/OFX/Natron" || exit 1
cd ..

#Build openfx-misc
git clone $GIT_MISC
cd openfx-misc || exit 1
git checkout "$MISC_BRANCH" || exit 1
git submodule update -i --recursive || exit 1
if [ "$MISC_BRANCH" = "master" ]; then
    # the snapshots are always built with the latest version of submodules
    git submodule foreach git pull origin master
fi

#Always bump git commit, it is only used to version-stamp binaries
MISC_GIT_VERSION=`git log|head -1|awk '{print $2}'`
sed -i "" -e "s/MISCPLUG_DEVEL_GIT=.*/MISCPLUG_DEVEL_GIT=${MISC_GIT_VERSION}/" $CWD/commits-hash.sh || exit 1

make CXX="$CXX" BITS=$BITS CONFIG=$CONFIG -j${MKJOBS} HAVE_CIMG=0 || exit 1
cp -r Misc/$OS-$BITS-$CONFIG/Misc.ofx.bundle "$PLUGINDIR/OFX/Natron" || exit 1
make -C CImg CImg.h || exit 1
if [ "$COMPILER" = "gcc" ]; then
    # build CImg with OpenMP support
    make -C CImg CXX="$CXX" BITS=$BITS CONFIG=$CONFIG -j${MKJOBS} CXXFLAGS_ADD=-fopenmp LDFLAGS_ADD=-fopenmp || exit 1
elif [ -n "$GXX" ]; then
    # GCC is available too!
    # libSupport was compiled by clang, now clean it to build it again with gcc
    make CXX="$CXX" BITS=$BITS CONFIG=$CONFIG -j${MKJOBS} HAVE_CIMG=0 clean || exit 1
    # build CImg with OpenMP support, but statically link libgomp (see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=31400)
    make -C CImg CXX="$GXX" BITS=$BITS CONFIG=$CONFIG -j${MKJOBS} CXXFLAGS_ADD=-fopenmp LDFLAGS_ADD="-fopenmp -static-libgcc" || exit 1
fi
cp -r CImg/$OS-$BITS-$CONFIG/CImg.ofx.bundle "$PLUGINDIR/OFX/Natron" || exit 1
cd ..

#Build openfx-arena
git clone $GIT_ARENA
cd openfx-arena || exit 1
git checkout "$ARENA_BRANCH" || exit 1
git submodule update -i --recursive || exit 1
if [ "$ARENA_BRANCH" = "master" ]; then
    # the snapshots are always built with the latest version of submodules
    if true; then
        git submodule foreach git pull origin master
    else
       echo "Warning: openfx-arena submodules not updated..."
    fi
fi
# ImageMagick on OSX is usually compiled without openmp
echo "Warning: removing -lgomp from MAGICK_LINKFLAGS, since ImageMagick on OSX is compiled without OMP support"
sed -e s/-lgomp// -i.orig Makefile.master

#Always bump git commit, it is only used to version-stamp binaries
ARENA_GIT_VERSION=`git log|head -1|awk '{print $2}'`
sed -i "" -e "s/ARENAPLUG_DEVEL_GIT=.*/ARENAPLUG_DEVEL_GIT=${ARENA_GIT_VERSION}/" $CWD/commits-hash.sh || exit 1

make CXX="$CXX" USE_PANGO=1 USE_SVG=1 STATIC=1 BITS=$BITS CONFIG=$CONFIG -j${MKJOBS} || exit 1
cp -r Bundle/$OS-$BITS-$CONFIG/Arena.ofx.bundle "$PLUGINDIR/OFX/Natron" || exit 1
cd ..

#Dump symbols for breakpad before stripping
if [ "$DISABLE_BREAKPAD" != "1" ]; then
    for bin in IO Misc CImg Arena; do
        binary="$PLUGINDIR"/${bin}.ofx.bundle/*/*/$bin.ofx
	
		DSYM_64=${bin}x86_64.dSYM
		DSYM_32=${bin}i386.dSYM
        dsymutil -arch x86_64 -o $DSYM_64 "$binary"
        dsymutil -arch i386 -o $DSYM_32 "$binary"

		$DUMP_SYMS -a x86_64 -g $DSYM_64 "$PLUGINDIR"/${bin}.ofx.bundle/*/*/${bin}.ofx  > "$CWD/build/symbols/${bin}.ofx-${TAG}-Mac-x86_64.sym"
		$DUMP_SYMS -a i386 -g $DSYM_32 "$PLUGINDIR"/${bin}.ofx.bundle/*/*/${bin}.ofx  > "$CWD/build/symbols/${bin}.ofx-${TAG}-Mac-i386.sym"
		
		rm -rf $DSYM_64
		rm -rf $DSYM_32
		
        #Strip binary
        if [ -x "$binary" ]; then
            echo "* stripping $binary";
            # Retain the original binary for QA and use with the util 'atos'
            #mv -f "$binary" "${binary}_FULL";
            if lipo "$binary" -verify_arch i386 x86_64; then
                # Extract each arch into a "thin" binary for stripping
                lipo "$binary" -thin x86_64 -output "${binary}_x86_64";
                lipo "$binary" -thin i386   -output "${binary}_i386";


                # Perform desired stripping on each thin binary.
                strip -S -x -r "${binary}_i386";
                strip -S -x -r "${binary}_x86_64";

                # Make the new universal binary from our stripped thin pieces.
                lipo -arch i386 "${binary}_i386" -arch x86_64 "${binary}_x86_64" -create -output "${binary}";

                # We're now done with the temp thin binaries, so chuck them.
                rm -f "${binary}_i386";
                rm -f "${binary}_x86_64";
            fi
            #rm -f "${binary}_FULL";
        fi
    done
fi

# move all libraries to the same place, put symbolic links instead
for plugin in "$PLUGINDIR"/*.ofx.bundle; do
    cd "$plugin/Contents/Libraries"
    for lib in lib*.dylib; do
        if [ -f "../../../../Frameworks/$lib" ]; then
            rm "$lib"
        else
            mv "$lib" "../../../../Frameworks/$lib"
        fi
        ln -sf "../../../../Frameworks/$lib" "$lib"
    done
    if [ "$COMPILER" = "gcc" ]; then # use gcc's libraries everywhere
        for l in gcc_s.1 gomp.1 stdc++.6; do
            lib=lib${l}.dylib
            for deplib in "$plugin"/Contents/MacOS/*.ofx "$plugin"/Contents/Libraries/lib*dylib ; do
                install_name_tool -change /usr/lib/$lib @executable_path/../Frameworks/$lib $deplib
            done
        done
    fi
done

#Build openfx-opencv
#git clone $GIT_OPENCV
#cd openfx-opencv || exit 1
#git checkout "$CV_BRANCH" || exit 1
#git submodule update -i --recursive || exit 1
#if [ "$CV_BRANCH" = "master" ]; then
#    # the snapshots are always built with the latest version of submodules
#    git submodule foreach git pull origin master
#fi

#Always bump git commit, it is only used to version-stamp binaries
#CV_GIT_VERSION=`git log|head -1|awk '{print $2}'`
#sed -i -e "s/CVPLUG_DEVEL_GIT=.*/CVPLUG_DEVEL_GIT=${CV_GIT_VERSION}/" $CWD/commits-hash.sh || exit 1

#cd opencv2fx || exit 1
#make CXX="$CXX" BITS=$BITS CONFIG=$CONFIG -j${MKJOBS} || exit 1
#cp -r */$OS-$BITS-*/*.ofx.bundle "$PLUGINDIR" || exit 1
#cd ..


