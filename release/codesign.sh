#!/bin/bash



# codesign them with developer ID cert

POPTS="--strict  --force --options=runtime --sign C7AF15C3BCF2AD2E5C102B9DB6502CFAE2C8CF3B --timestamp"
AOPTS="--strict  --force --options=runtime --sign C7AF15C3BCF2AD2E5C102B9DB6502CFAE2C8CF3B --timestamp"

codesign ${AOPTS} --entitlements StudioMaster.entitlements StudioMaster/StudioMaster.app
codesign ${POPTS} --entitlements StudioMaster.entitlements  StudioMaster/StudioMaster.component
codesign ${POPTS} --entitlements StudioMaster.entitlements StudioMaster/StudioMaster.vst3
codesign ${POPTS} --entitlements StudioMaster.entitlements StudioMaster/StudioMasterInstrument.vst3
codesign ${POPTS} --entitlements StudioMaster.entitlements  StudioMaster/StudioMaster.vst

# AAX is special
if [ -n "${AAXSIGNCMD}" ]; then
 echo "Signing AAX plugin"
 ${AAXSIGNCMD}  --in StudioMaster/StudioMaster.aaxplugin --out StudioMaster/StudioMaster.aaxplugin
fi


if [ "x$1" = "xonly" ] ; then
  echo Code-signing only
  exit 0
fi


mkdir -p tmp

# notarize them in parallel
./notarize-app.sh --submit=tmp/sbapp.uuid  StudioMaster/StudioMaster.app
./notarize-app.sh --submit=tmp/sbau.uuid StudioMaster/StudioMaster.component
./notarize-app.sh --submit=tmp/sbvst3.uuid StudioMaster/StudioMaster.vst3
./notarize-app.sh --submit=tmp/sbinstvst3.uuid StudioMaster/StudioMasterInstrument.vst3
./notarize-app.sh --submit=tmp/sbvst2.uuid StudioMaster/StudioMaster.vst 

if ! ./notarize-app.sh --resume=tmp/sbapp.uuid StudioMaster/StudioMaster.app ; then
  echo Notarization App failed
  exit 2
fi

if ! ./notarize-app.sh --resume=tmp/sbau.uuid StudioMaster/StudioMaster.component ; then
  echo Notarization AU failed
  exit 2
fi

if ! ./notarize-app.sh --resume=tmp/sbvst3.uuid StudioMaster/StudioMaster.vst3 ; then
  echo Notarization VST3 failed
  exit 2
fi

if ! ./notarize-app.sh --resume=tmp/sbinstvst3.uuid StudioMaster/StudioMasterInstrument.vst3 ; then
  echo Notarization Inst VST3 failed
  exit 2
fi
  
if ! ./notarize-app.sh --resume=tmp/sbvst2.uuid StudioMaster/StudioMaster.vst ; then
  echo Notarization VST2 failed
  exit 2
fi

#if ! ./notarize-app.sh StudioMaster/StudioMaster.aaxplugin ; then
#  echo Notarization AAX failed
#  exit 2
#fi





