#!/bin/bash

if [ -z "$1" ] ; then
   echo "Usage: $0 <version> <certpassword>"
   exit 1
fi

VERSION=$1

CERTPASS=$2

if [ -z "$CERTFILE" ] ; then
  echo You need to define CERTFILE env variable to sign anything
  exit 2
fi

#BUILDDIR='../Builds/VisualStudio2017/x64/Release'
#BUILDDIR32='../Builds/VisualStudio2017/Win32/Release32'
BUILDDIR='../build/StudioMaster_artefacts/Release'
BUILDDIR32='../build32/StudioMaster_artefacts/Release'
INSTBUILDDIR='../build/StudioMasterInst_artefacts/Release'
INSTBUILDDIR32='../build32/StudioMasterInst_artefacts/Release'

rm -rf StudioMaster

mkdir -p StudioMaster/Plugins/VST StudioMaster/Plugins/VST3 StudioMaster/Plugins/AAX

cp -v ../doc/README_WINDOWS.txt StudioMaster/README.txt
cp -v ${BUILDDIR}/Standalone/StudioMaster.exe StudioMaster/
cp -pHLRv ${BUILDDIR}/VST3/StudioMaster.vst3 StudioMaster/Plugins/VST3/
cp -pHLRv ${INSTBUILDDIR}/VST3/StudioMasterInstrument.vst3 StudioMaster/Plugins/VST3/
cp -v ${BUILDDIR}/VST/StudioMaster.dll StudioMaster/Plugins/VST/
cp -pHLRv ${BUILDDIR}/AAX/StudioMaster.aaxplugin StudioMaster/Plugins/AAX/


mkdir -p StudioMaster/Plugins32/VST StudioMaster/Plugins32/VST3 StudioMaster/Plugins32/AAX

cp -v ${BUILDDIR32}/Standalone/StudioMaster.exe StudioMaster/StudioMaster32.exe
cp -pHLRv ${BUILDDIR32}/VST3/StudioMaster.vst3 StudioMaster/Plugins32/VST3/
cp -pHLRv ${INSTBUILDDIR32}/VST3/StudioMasterInstrument.vst3 StudioMaster/Plugins32/VST3/
cp -v ${BUILDDIR32}/VST/StudioMaster.dll StudioMaster/Plugins32/VST/



# sign AAX
if [ -n "${AAXSIGNCMD}" ]; then
  echo "Signing AAX plugin"
  ${AAXSIGNCMD} --keypassword "${CERTPASS}"  --in 'StudioMaster\Plugins\AAX\Sonobus.aaxplugin' --out 'StudioMaster\Plugins\AAX\Sonobus.aaxplugin'
fi


# sign executable
#signtool.exe sign /v /t "http://timestamp.digicert.com" /f "$CERTFILE" /p "$CERTPASS" StudioMaster/StudioMaster.exe

mkdir -p instoutput
rm -f instoutput/*


iscc /O"instoutput" "/Ssigntool=signtool.exe sign /t http://timestamp.digicert.com /f ${CERTFILE} /p ${CERTPASS} \$f"  /DSBVERSION="${VERSION}" wininstaller.iss

#signtool.exe sign /v /t "http://timestamp.digicert.com" /f SonosaurusCodeSigningSectigoCert.p12 /p "$CERTPASS" instoutput/

#ZIPFILE=sonobus-${VERSION}-win.zip
#cp -v ../doc/README_WINDOWS.txt instoutput/README.txt
#rm -f ${ZIPFILE}
#(cd instoutput; zip  ../${ZIPFILE} StudioMaster\ Installer.exe README.txt )

EXEFILE=sonobus-${VERSION}-win.exe
rm -f ${EXEFILE}
cp instoutput/StudioMaster-${VERSION}-Installer.exe ${EXEFILE}
