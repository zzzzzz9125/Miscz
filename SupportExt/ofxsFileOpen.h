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

#ifndef openfx_supportext_ofxsFileOpen_h
#define openfx_supportext_ofxsFileOpen_h

#include <string>
#include <cstdio>

namespace OFX {

#ifdef _WIN32
std::wstring utf8_to_utf16 (const std::string& utf8str);
std::string utf16_to_utf8 (const std::wstring& str);
#endif

bool exists_utf8(const char* path_utf8);
int remove_utf8(const char* path_utf8);
std::FILE* fopen_utf8(const char* path, const char* mode);

} // namespace OFX
#endif /* defined(openfx_supportext_ofxsFileOpen_h) */
