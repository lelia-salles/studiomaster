#!/bin/bash

if [ -z "$1" ] ; then
   echo "Usage: $0 <version>"
   exit 1
fi

VERSION=$1


#BUILDDIR=../Builds/MacOSX/build/Release
BUILDDIR=../build/StudioMaster_artefacts/Release
INSTBUILDDIR=../build/StudioMasterInst_artefacts/Release

rm -rf StudioMaster

mkdir -p StudioMaster


cp ../doc/README_MAC.txt StudioMaster/

cp -pLRv ${BUILDDIR}/Standalone/StudioMaster.app  StudioMaster/
cp -pLRv ${BUILDDIR}/AU/StudioMaster.component  StudioMaster/
cp -pLRv ${BUILDDIR}/VST3/StudioMaster.vst3 StudioMaster/
cp -pLRv ${INSTBUILDDIR}/VST3/StudioMasterInstrument.vst3 StudioMaster/
cp -pLRv ${BUILDDIR}/VST/StudioMaster.vst  StudioMaster/
cp -pRHv ${BUILDDIR}/AAX/StudioMaster.aaxplugin  StudioMaster/


#cp -pLRv ${BUILDDIR}/StudioMaster.app  StudioMaster/
#cp -pLRv ${BUILDDIR}/StudioMaster.component  StudioMaster/
#cp -pLRv ${BUILDDIR}/StudioMaster.vst3 StudioMaster/
#cp -pLRv ${BUILDDIR}/StudioMaster.vst  StudioMaster/
#cp -pRHv ${BUILDDIR}/StudioMaster.aaxplugin  StudioMaster/

#ln -sf /Library/Audio/Plug-Ins/Components StudioMaster/
#ln -sf /Library/Audio/Plug-Ins/VST3 StudioMaster/
#ln -sf /Library/Audio/Plug-Ins/VST StudioMaster/
#ln -sf /Library/Application\ Support/Avid/Audio/Plug-Ins StudioMaster/


# this codesigns and notarizes everything
if ! ./codesign.sh ; then
  echo
  echo Error codesign/notarizing, stopping
  echo
  exit 1
fi

# make installer package (and sign it)

rm -f macpkg/StudioMasterTemp.pkgproj

if ! ./update_package_version.py ${VERSION} macpkg/StudioMaster.pkgproj macpkg/StudioMasterTemp.pkgproj ; then
  echo
  echo Error updating package project versions
  echo
  exit 1
fi

if ! packagesbuild  macpkg/StudioMasterTemp.pkgproj ; then
  echo 
  echo Error building package
  echo
  exit 1
fi

mkdir -p StudioMasterPkg
rm -f StudioMasterPkg/*

if ! productsign --sign ${INSTSIGNID} --timestamp  macpkg/build/StudioMaster\ Installer.pkg StudioMasterPkg/StudioMaster\ Installer.pkg ; then
  echo 
  echo Error signing package
  echo
  exit 1
fi

# make dmg with package inside it

if ./makepkgdmg.sh $VERSION ; then

   ./notarizedmg.sh ${VERSION}/sonobus-${VERSION}-mac.dmg

   echo
   echo COMPLETED DMG READY === ${VERSION}/sonobus-${VERSION}-mac.dmg
   echo
   
fi
