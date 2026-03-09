#!/bin/bash

PREFIX=/usr/local

if [ -n "$1" ] ; then
  PREFIX="$1"
fi

echo "Installing StudioMaster to ${PREFIX} ... (specify destination as command line argument if you want it elsewhere)"

BUILDDIR=../build/StudioMaster_artefacts/Release
INSTBUILDDIR=../build/StudioMasterInst_artefacts/Release

mkdir -p ${PREFIX}/bin
if ! cp ${BUILDDIR}/Standalone/sonobus  ${PREFIX}/bin/sonobus ; then
  echo
  echo "Looks like you need to run this as 'sudo $0'"
  exit 2
fi

mkdir -p ${PREFIX}/share/applications
cp sonobus.desktop ${PREFIX}/share/applications/sonobus.desktop
chmod +x ${PREFIX}/share/applications/sonobus.desktop

mkdir -p ${PREFIX}/share/pixmaps
cp ../images/sonobus_logo@2x.png ${PREFIX}/share/pixmaps/sonobus.png

if [ -d ${BUILDDIR}/VST3/StudioMaster.vst3 ] ; then
  mkdir -p ${PREFIX}/lib/vst3
  cp -a ${BUILDDIR}/VST3/StudioMaster.vst3 ${PREFIX}/lib/vst3/

  echo "StudioMaster VST3 plugin installed"
fi

if [ -d ${INSTBUILDDIR}/VST3/StudioMasterInstrument.vst3 ] ; then
  mkdir -p ${PREFIX}/lib/vst3
  cp -a ${INSTBUILDDIR}/VST3/StudioMasterInstrument.vst3 ${PREFIX}/lib/vst3/

  echo "StudioMaster VST3i plugin installed"
fi

if [ -d ${BUILDDIR}/LV2/StudioMaster.lv2 ] ; then
  mkdir -p ${PREFIX}/lib/lv2
  cp -a ${BUILDDIR}/LV2/StudioMaster.lv2 ${PREFIX}/lib/lv2/

  echo "StudioMaster LV2 plugin installed"
fi


echo "StudioMaster application installed"

