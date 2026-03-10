// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell
// Modifications Copyright (C) 2026 Master Musica

#pragma once

#include "JuceHeader.h"

//==============================================================================
/**
    Gerencia as informações de versão e atualizações do StudioMaster.
    Conectado ao repositório: https://github.com/lelia-salles/studiomaster
*/
class VersionInfo
{
public:
    // Constantes de Branding da Master Musica
    static inline const String appName = "StudioMaster";
    static inline const String companyName = "Master Musica";
    static inline const String supportUrl = "https://www.mastermusica.com.br/studio-master";

    struct Asset
    {
        const String name;
        const String url;
    };

    static std::unique_ptr<VersionInfo> fetchFromUpdateServer (const String& versionString);
    static std::unique_ptr<VersionInfo> fetchLatestFromUpdateServer();
    static std::unique_ptr<InputStream> createInputStreamForAsset (const Asset& asset, int& statusCode);

    bool isNewerVersionThanCurrent();

    const String versionString;
    const String releaseNotes;
    const std::vector<Asset> assets;

private:
    VersionInfo(const String& v, const String& r, std::vector<Asset>&& a)
        : versionString(v), releaseNotes(r), assets(std::move(a)) {}

    static std::unique_ptr<VersionInfo> fetch (const String& endpoint);
};