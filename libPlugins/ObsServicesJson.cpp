/*
 * Copyright (c) 2013-2020 MFCXY, Inc. <mfcxy@mfcxy.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "ObsServicesJson.h"

// obs includes
 #include <obs-module.h>
 #include <obs-frontend-api.h>
 #include <util/config-file.h>
 //#include <util/platform.h>

// mfc includes
#include <libfcs/fcslib_string.h>
#include <libfcs/Log.h>
#include <libfcs/MfcJson.h>

// project/solution includes
#include <libPlugins/MFCConfigConstants.h>
#include <libPlugins/ObsUtil.h>
//#include <libPlugins/PluginParameterBlock.h>
#include <libPlugins/Portable.h>

// System Includes
#include <string>
#include <iostream>
#include <fstream>

using njson = nlohmann::json;
using std::ifstream;
using std::string;

//---------------------------------------------------------------------------
// CObsServicesJson
//
// helper class to manage the services.json file.
CObsServicesJson::CObsServicesJson()
    : m_nVersion(0), m_bLoaded(false), m_bisDirty(false)
{}


//--------------------------------------------------------------------------
// load
//
// read the services file.
bool CObsServicesJson::load(const string& sFile)
{
    setLoaded(false);

    //_MESG("PATHDBG: Trying to load services from just file: %s", sFile.c_str());

    setServicesFilename(sFile);
    string sFilename = getNormalizedServiceFile(sFile);//obs_module_config_path("services.json");

    if (parseFile(sFilename))
        setLoaded(true);

    return isLoaded();
}


bool CObsServicesJson::load(const string& sFile, const string& sProgramFile)
{
    setLoaded(false);

    setServicesFilename(sFile);
    string sFilename = getNormalizedServiceFile(sFile);//obs_module_config_path("services.json");
    //_MESG("PATHDBG: Trying to load services from file: %s, program file: %s", sFilename.c_str(), sProgramFile.c_str());

    if (parseFile(sFilename))
    {
        setLoaded(true);
    }
    //else if (m_nVersion > RTMP_SERVICES_FORMAT_VERSION)
    else
    {
        // if we failed to load, we might have a bad file version.
        if (Update(sFile, sProgramFile))
        {
            if (parseFile(sFilename))
            {
                setLoaded(true);
            }
            else _TRACE("parseFile failed!");
        }
        else _TRACE("Bad services.json file versions!");
    }
    //else _TRACE("appears like services.json is corrupt!");

    return isLoaded();
}


//--------------------------------------------------------------------------
// save
//
// update the services file.
bool CObsServicesJson::save()
{
    string sData = m_njson.dump();
    string sFilename = getFilename();
    if (stdSetFileContents(sFilename, sData))
    {
        setDirty(false);
        return true;
    }
    return false;
}


const string CObsServicesJson::getNormalizedServiceFile(const string& sFile)
{
    string sFilename = sFile;//obs_module_config_path("services.json");
    assert(sFilename.size() > 0);
    string sFind = BROADCAST_FILENAME;
    string sReplace = "rtmp-services";
    size_t nFnd = sFilename.find(sFind);
    if (nFnd != string::npos)
        sFilename.replace(nFnd, sFind.length(), sReplace);
    return sFilename;
}


int CObsServicesJson::getJsonVersion(const string& sFile)
{
    string sFilename = getNormalizedServiceFile(sFile);
    //_TRACE("Attempting to load %s", sFilename.c_str());
    int nVer = INT_MAX;

    ifstream str(sFilename);
    if (str.is_open())
    {
        njson j;
        str >> j;

        if (j.find("format_version") != j.end())
        {
            nVer = j["format_version"].get<int>();
            //_TRACE("%s, file version: %d", sFilename.c_str(), nVer);
        }
        else _TRACE("Error Parsing file version: %s", sFilename.c_str());
    }
    else _TRACE("Error loading services.json %s", sFilename.c_str());

    return nVer;
}


bool CObsServicesJson::Update(const string& sFileProfile, const string& sFileProgram)
{
    _MESG("PATHDBG: Update profile settings? profile: %s, program: %s", sFileProfile.c_str(), sFileProgram.c_str());
    int nProfileVer = getJsonVersion(sFileProfile);
    if (nProfileVer > RTMP_SERVICES_FORMAT_VERSION)
    {
        int nProgramVer = getJsonVersion(sFileProgram);
        // profile version is wrong.
        if (nProgramVer <= RTMP_SERVICES_FORMAT_VERSION)
        {
            string sFilename = getNormalizedServiceFile(sFileProgram);
            string sData;
            if (stdGetFileContents(sFilename, sData))
            {
                _TRACE("updating service.jsons from profile %d to program files %d", nProfileVer, nProgramVer);
                sFilename = getNormalizedServiceFile(sFileProfile);
#ifdef _WIN32
                char szPath[_MAX_PATH + 1] = { '\0' };
                char* pFile = NULL;
                GetFullPathNameA(sFilename.c_str(), sizeof(szPath), szPath, &pFile);
                string filepath = szPath;
                string dirpath = filepath.substr(0, filepath.rfind('\\'));
                CreateDirectoryA(dirpath.c_str(), NULL);
                _TRACE("Creating %s just in case", dirpath.c_str());
#endif
                _TRACE("PATHDBG: Update profile settings? profile: %s, program: %s",
                       sFilename.c_str(), getNormalizedServiceFile(sFileProgram).c_str());
                if (stdSetFileContents(sFilename, sData))
                    _TRACE("updated service.jsons");
                else
                    _TRACE("failed to update %s", sFilename.c_str());
                return true;
            }
            else _TRACE("Error, failed to read source file %s", sFilename.c_str());
        }
        else
        {
            // both files are not the correct version.  obs-studio will throw an exception. let it handle it. bye
            _TRACE("Both service.jsons have bad version: profile %s program files %d expected %d", nProfileVer, nProgramVer, RTMP_SERVICES_FORMAT_VERSION);
        }
    }
    return false;
}


//--------------------------------------------------------------------------
// loadDefaultRTMPService
//
// Load the default webrtc MFC service values into obs-studio json
bool CObsServicesJson::loadDefaultRTMPService(njson& arr)
{
    njson jsRTMP =
    {
        { "common", true },
        { "name", MFC_SERVICES_JSON_NAME_RTMP_VALUE },
        { "recommended",
            {
                { "keyint", MFC_SERVICES_JSON_KEYINT_VALUE },
                { "max width", MFC_SERVICES_JSON_MAX_WIDTH_VALUE },
                { "max height", MFC_SERVICES_JSON_MAX_HEIGHT_VALUE },
                { "max fps", MFC_SERVICES_JSON_MAX_FPS_VALUE },
                { "max video bitrate", MFC_SERVICES_JSON_VIDEO_BITRATE_VALUE },
                { "max audio bitrate", MFC_SERVICES_JSON_AUDIO_BITRATE_VALUE },
                { "profile", MFC_SERVICES_JSON_X264_PROFILE_VALUE },
                { "bframes", MFC_SERVICES_JSON_BFRAMES_VALUE },
                { "x264opts", MFC_SERVICES_JSON_X264OPTS_VALUE }
            }
        },
        { "servers",
            {
                {
                    { "name", MFC_SERVICES_JSON_PRIMARY_SERVER_NAME },
                    { "url", MFC_DEFAULT_BROADCAST_URL }
                }/*,
                {
                    { "name", MFC_NORTH_AMERICA_EAST_SERVER_NAME },
                    { "url", MFC_NORTH_AMERICA_EAST_BROADCAST_URL }
                },
                {
                    { "name", MFC_NORTH_AMERICA_WEST_SERVER_NAME },
                    { "url", MFC_NORTH_AMERICA_WEST_BROADCAST_URL }
                },
                {
                    { "name", MFC_EUROPE_EAST_SERVER_NAME },
                    { "url", MFC_EUROPE_EAST_BROADCAST_URL }
                },
                {
                    { "name", MFC_EUROPE_WEST_SERVER_NAME },
                    { "url", MFC_EUROPE_WEST_BROADCAST_URL }
                },
                {
                    { "name", MFC_SOUTH_AMERICA_SERVER_NAME },
                    { "url", MFC_SOUTH_AMERICA_BROADCAST_URL }
                },
                {
                    { "name", MFC_AUSTRALIA_SERVER_NAME },
                    { "url", MFC_AUSTRALIA_BROADCAST_URL }
                },
                {
                    { "name", MFC_EAST_ASIA_SERVER_NAME },
                    { "url", MFC_EAST_ASIA_BROADCAST_URL }
                }*/
            }
        }
    };
    arr.push_back(jsRTMP);
    setDirty(true);
    return true;
}


//--------------------------------------------------------------------------
// loadDefaultWegbRTCService
//
// Load the default webrtc MFC service values into obs-studio json
bool CObsServicesJson::loadDefaultWebRTCService(njson& arr)
{
    njson jswebRTC =
    {
        { "common", true },
        { "name", MFC_SERVICES_JSON_NAME_WEBRTC_VALUE },
        { "recommended" ,
            {
                { "keyint", MFC_SERVICES_JSON_KEYINT_VALUE },
                { "max audio bitrate", MFC_SERVICES_JSON_AUDIO_BITRATE_VALUE },
                { "max video bitrate", MFC_SERVICES_JSON_VIDEO_BITRATE_VALUE },
                { "output", MFC_DEFAULT_WEBRTC_OUTPUT }
            },
        },
        { "servers",
            {
                {
                    { "name", MFC_SERVICES_JSON_PRIMARY_SERVER_NAME },
                    { "url", "Automatic" }
                }
            }
        }
    };
    arr.push_back(jswebRTC);
    setDirty(true);
    return true;
}


//--------------------------------------------------------------------------
// getURLList
//
// replace the URL server list.
void CObsServicesJson::getURLList(strVec* parrNames, strVec* parrURL)
{
    _ASSERT(isLoaded());
    if (isLoaded())
    {
        njson mfcServers;
        if (findMFCRTMPServerJson(&mfcServers))
        {
            for (njson::iterator itr = mfcServers.begin(); itr != mfcServers.end(); itr++)
            {
                njson& srv = *itr;
                string sServerName = srv["name"].get<string>();
                string sURL = srv["url"].get<string>();
                parrNames->push_back(sServerName);
                parrURL->push_back(sURL);
            }
        }
        else _TRACE("No MFC servers defined. This really shouldn't happen");
    }
}


//--------------------------------------------------------------------------
// setURLList
//
// replace the current MFC URL server list
void CObsServicesJson::setURLList(strVec& arrNames, strVec& arrURL)
{
    _ASSERT(isLoaded());
    string sData;
    if (isLoaded())
    {
        njson jsMFCServers;
        if (findMFCRTMPServerJson(&jsMFCServers))
        {
            jsMFCServers.clear();
            for (size_t c = 0; c < arrNames.size(); c++)
            {
                string sName = arrNames[c];
                string sURL = arrURL[c];
                njson jSvr = { { "name", sName.c_str() }, { "url", sURL.c_str() } };
                jsMFCServers.push_back(jSvr);
            }
            sData = jsMFCServers.dump();
            setDirty(false);
        }
    }
}


//--------------------------------------------------------------------------
// getVersion
//
// get the version of the file.
bool CObsServicesJson::getVersion(int& nVersion)
{
    bool foundVal = false;

    nVersion = INT_MAX;
    if (m_njson.find("format_version") != m_njson.end())
    {
        nVersion = m_njson["format_version"].get<int>();
        foundVal = true;
    }

    return foundVal;
}


//---------------------------------------------------------------------------
// getMFCServiceJson
//
// find the MFC service json
bool CObsServicesJson::findMFCRTMPServerJson(njson* pArr)
{
    if (m_njson.find("services") != m_njson.end() && m_njson["services"].is_array())
    {
        njson& arr = m_njson["services"];

        //setServiceNJson(arr);
        _TRACE("Got services array\n");
        // find myfreecams.
        njson mfc;
        if (findRTMPService(arr, &mfc))
        {
            njson::iterator itr = mfc.find("servers");
            if (itr != mfc.end())
            {
                *pArr = mfc["servers"];
                return true;
            }
            else
            {
                _ASSERT(!"No server object found");
                string sData = mfc.dump();
                _TRACE("No server object found %s", sData.c_str());
            }
        }
    } // end for each array elelment
    return false;;
}


bool CObsServicesJson::findRTMPService(njson& arr, njson* pSrv)
{
    return findMFCServiceJson(arr, pSrv, string(MFC_SERVICES_JSON_NAME_RTMP_VALUE));
}


bool CObsServicesJson::findWebRtcService(njson& arr, njson* pSrv)
{
    return findMFCServiceJson(arr, pSrv, string(MFC_SERVICES_JSON_NAME_WEBRTC_VALUE));
}


//---------------------------------------------------------------------------
// getMFCServiceJson
//
// find the MFC service json
bool CObsServicesJson::findMFCServiceJson(njson& arr, njson* pSrv, const string& sSvcName)
{
    assert(arr.is_array());

    for (njson::iterator itr = arr.begin(); itr != arr.end(); itr++)
    {
        njson j = *itr;
        string sName = j[MFC_SERVICES_JSON_NAME].get<string>();

        if (sName == sSvcName)
        {
            *pSrv = j;
            return true;
        } // endif this object is not MyFreeCams.
    }
    // _ASSERT(!"Name object not found in json");
    string sData = arr.dump();
    // _TRACE("Offending json: %s", sData.c_str());
    _TRACE("found service.json");

    return false;
}


//--------------------------------------------------------------------------
// parseFile
//
// parse the services json file.
bool CObsServicesJson::parseFile(const string& sFilename)
{
    m_sFilename = sFilename;
    bool bFnd = false;

    ifstream str(sFilename);
    if (str.is_open())
    {
        m_njson.clear();

        str >> m_njson;
        string js = m_njson.dump();

        string sVersion;
        int nVer = 0;

        if (m_njson.find("format_version") != m_njson.end())
        {
            nVer = m_njson["format_version"].get<int>();

            m_nVersion = nVer;
            if (nVer <= RTMP_SERVICES_FORMAT_VERSION)
            {
                //_TRACE("services.json version: %d", nVer);
                if (m_njson.find("services") != m_njson.end() && m_njson["services"].is_array())
                {
                    njson& arr = m_njson["services"];
                    njson mfc;
                    if (!findRTMPService(arr, &mfc))
                    {
                        _TRACE("%s service not found!", MFC_SERVICES_JSON_NAME_RTMP_VALUE);
                        if (loadDefaultRTMPService(arr))
                        {
#ifdef UNIT_TEST
                            if (!findRTMPService(arr, &mfc))
                                bFnd = false;
                            if (!findWebRtcService(arr, &mfc))
                                bFnd = false;
#endif
                            bFnd = true;
                        }
                    }
                    else
                    {
                        bFnd = true;
                    }

                    if (!findWebRtcService(arr, &mfc))
                    {
                        _TRACE("%s service not found!", MFC_SERVICES_JSON_NAME_WEBRTC_VALUE);
                        if (loadDefaultWebRTCService(arr))
                        {
#ifdef UNIT_TEST
                            if (!findRTMPService(arr, &mfc))
                                bFnd = false;
                            if (!findWebRtcService(arr, &mfc))
                                bFnd = false;
#endif
                            bFnd = true;
                        }
                    }
                    else
                    {
                        bFnd = true;
                    }
                }
                else
                {
                    _ASSERT(!"Services is not an array? this really shouldn't happen");
                    _TRACE("Json format error, services is not an array");
                }
            }
            else
            {
                _TRACE("Invalid version: %d. Version expected: %d", nVer, RTMP_SERVICES_FORMAT_VERSION);
                bFnd = false;
            }
        }
    }
    return bFnd;
}


bool CObsServicesJson::updateProfileSettings(const string& sKey, const string& sURL)
{
    string sFilename = CObsUtil::AppendPath(CObsUtil::getProfilePath(), SERVICE_JSON_FILE);
    bool retVal = false;
    MfcJsonObj js;

    //_MESG("PATHDBG: update profile settings in %s with key:%s", sFilename.c_str(), sKey.c_str());
    if (js.loadFromFile(sFilename))
    {
        MfcJsonObj* pSet = js.objectGet(SERVICE_JSON_SETTING);
        if (pSet)
        {
            bool mfcService = false;
            string sName;
            if (!pSet->objectGetString(SERVICE_JSON_SERVICE, sName))
                sName = "Custom";

            string sKeyCurrent, sURLCurrent;
            pSet->objectGetString(SERVICE_JSON_STREAM_KEY, sKeyCurrent);
            pSet->objectGetString(SERVICE_JSON_STREAM_URL, sURLCurrent);

            // only save if the current service starts with 'MyFreeCams',
            // such as services 'MyFreeCams RTMP' or 'MyFreeCams WebRTC',
            // or if it's a Custom service with an MFC server URL detected.
            if ( sName.find("MyFreeCams") == 0 )
            {
                mfcService = true;
            }
            else if (sName == "Custom")
            {
                if (    sURL.find(".myfreecams.com/NxServer") != string::npos
                    ||  sURLCurrent.find(".myfreecams.com/NxServer") != string::npos)
                {
                    mfcService = true;
                }
            }

            if (mfcService)
            {
                if (sKeyCurrent != sKey || sURLCurrent != sURL)
                {
                    size_t updates = 0;
                    // if either the sKey or sURL are set to the string "(null)", then we skip
                    // any updating of them. This is so we can update one of properties without updating
                    // the other, in cases where we have only some of the data available.

                    if (sKey != "(null)")
                    {
                        pSet->objectAdd(string(SERVICE_JSON_STREAM_KEY), sKey);
                        updates++;
                    }

                    if (sURL != "(null)")
                    {
                        pSet->objectAdd(string(SERVICE_JSON_STREAM_URL), sURL);
                        updates++;
                    }

                    if (updates > 0)
                    {
                        string sCurData, sData = js.prettySerialize();
                        stdGetFileContents(sFilename, sCurData);
                        if (sData != sCurData)
                        {
                            if ((retVal = stdSetFileContents(sFilename, sData)) == true)
                            {
                                //blog(100, "[wrote profile config] %s; %zu => %zu bytes", sFilename.c_str(), sCurData.size(), sData.size());
                            }
                            else _MESG("FAILED TO SET %s with data: %s", sFilename.c_str(), sData.c_str());
                        }
                    }
                    else retVal = true;
                }
            }
            else _MESG("SKIPPED non-mfc service profile %s", sName.c_str());
        }
        else _MESG("Failed to find settings");
    }
    else _TRACE("Failed to load %s", sFilename.c_str());

    return retVal;
}


bool CObsServicesJson::refreshProfileSettings(string& sKey, string& sURL)
{
    string sFilename( CObsUtil::AppendPath(CObsUtil::getProfilePath(), SERVICE_JSON_FILE) );
    bool retVal = false;

    ifstream str(sFilename);
    if (str.is_open())
    {
        njson j;
        str >> j;

        if (j.find(SERVICE_JSON_SETTING) != j.end())
        {
            njson jSet = j[SERVICE_JSON_SETTING];

            string sName;
            if (jSet.find(SERVICE_JSON_SERVICE) != jSet.end())
            {
                sName = jSet[SERVICE_JSON_SERVICE].get<string>();

                // only save if we are the current service.
                if (sName == MFC_SERVICES_JSON_NAME_RTMP_VALUE || sName == MFC_SERVICES_JSON_NAME_WEBRTC_VALUE)
                {
                    sKey = jSet[SERVICE_JSON_STREAM_KEY].get<string>();
                    sURL = jSet[SERVICE_JSON_STREAM_URL].get<string>();
                    retVal = true;
                }
            }
        }
    }

    return retVal;
}
