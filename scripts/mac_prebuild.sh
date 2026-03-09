#!/bin/bash

#cp -v ../../scripts/StudioMaster-mac-sandbox.entitlements StudioMaster.entitlements

if grep sandbox StudioMaster.entitlements &> /dev/null ; then
   cp -v ../../scripts/StudioMaster-mac.entitlements StudioMaster.entitlements
fi
