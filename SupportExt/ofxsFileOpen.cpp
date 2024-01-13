/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of openfx-supportext <https://github.com/NatronGitHub/openfx-supportext>,
 * (C) 2018-2021 The Natron Developers
 * (C) 2013-2018 INRIA
 *
 * openfx-supportext is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * openfx-supportext is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openfx-supportext.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

/*
 * OFX utiliy functions to open a file safely with UTF-8 encoded strings.
 */

#include "ofxsFileOpen.h"

#if defined(_WIN32)
#include <windows.h>
#include <fcntl.h>
#include <sys/stat.h>
#endif

#include <fstream>

using std::string;
using std::wstring;


namespace OFX {
#ifdef _WIN32
std::wstring
utf8_to_utf16 (const string& str)
{
    wstring native;

    native.resize( MultiByteToWideChar (CP_UTF8, 0, str.data(), (int)str.length(), NULL, 0) );
    MultiByteToWideChar ( CP_UTF8, 0, str.data(), (int)str.length(), &native[0], (int)native.size() );

    return native;
}

string
utf16_to_utf8 (const wstring& str)
{
    string utf8;

    utf8.resize( WideCharToMultiByte (CP_UTF8, 0, str.data(), (int)str.length(), NULL, 0, NULL, NULL) );
    WideCharToMultiByte (CP_UTF8, 0, str.data(), (int)str.length(), &utf8[0], (int)utf8.size(), NULL, NULL);

    return utf8;
}

#endif

bool
exists_utf8(const char* path_utf8)
{
#ifdef _WIN32
    // on Windows fopen does not accept UTF-8 paths, so we convert to wide char
    wstring wpath = utf8_to_utf16 (path_utf8);

    return ::_waccess ( wpath.c_str(), 0 ) != -1;
#else

    // on Unix platforms passing in UTF-8 works
    return !!std::ifstream(path_utf8);
#endif
}

// returns ​0​ upon success or non-zero value on error
int
remove_utf8(const char* path_utf8)
{
#ifdef _WIN32
    // on Windows fopen does not accept UTF-8 paths, so we convert to wide char
    wstring wpath = utf8_to_utf16 (path_utf8);

    return ::_wremove ( wpath.c_str() );
#else

    // on Unix platforms passing in UTF-8 works
    return std::remove (path_utf8);
#endif
}

std::FILE*
fopen_utf8(const char* path_utf8,
           const char* mode)
{
#ifdef _WIN32
    // on Windows fopen does not accept UTF-8 paths, so we convert to wide char
    wstring wpath = utf8_to_utf16 (path_utf8);
    wstring wmode = utf8_to_utf16 (mode);

    return ::_wfopen ( wpath.c_str(), wmode.c_str() );
#else

    // on Unix platforms passing in UTF-8 works
    return std::fopen (path_utf8, mode);
#endif
}
} // namespace OFX
