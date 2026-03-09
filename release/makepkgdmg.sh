#!/bin/bash

if [ -z "$1" ] ; then
  echo "Usage: $0 <version>"
  exit 1
fi

VERSION=$1

rm -f StudioMasterPkg.dmg

cp StudioMaster/README_MAC.txt StudioMasterPkg/

if dropdmg --config-name=StudioMasterPkg --layout-folder StudioMasterPkgLayout --volume-name="StudioMaster v${VERSION}"  --APP_VERSION=v${VERSION}  --signing-identity=C7AF15C3BCF2AD2E5C102B9DB6502CFAE2C8CF3B StudioMasterPkg
then
  mkdir -p ${VERSION}
  mv -v StudioMasterPkg.dmg ${VERSION}/sonobus-${VERSION}-mac.dmg  	
else
  echo "Error making package DMG"
  exit 2
fi

