/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#pragma once

/**
 * Besides the "pragma once" above that only protects this file,
 * define a macro to prevent including different versions of
 * "OrthancFramework.h"
 **/
#ifndef __ORTHANC_FRAMEWORK_H
#define __ORTHANC_FRAMEWORK_H

#if !defined(ORTHANC_FRAMEWORK_BUILDING_LIBRARY)
#  error Macro ORTHANC_FRAMEWORK_BUILDING_LIBRARY must be defined
#endif

/**
 * It is implied that if this file is used, we're building the Orthanc
 * framework (not using it as a shared library): We don't use the
 * common "BUILDING_DLL"
 * construction. https://gcc.gnu.org/wiki/Visibility
 **/
#if ORTHANC_FRAMEWORK_BUILDING_LIBRARY == 1
#  if defined(_WIN32) || defined (__CYGWIN__)
#    define ORTHANC_PUBLIC __declspec(dllexport)
#    define ORTHANC_LOCAL
#  else
#    if __GNUC__ >= 4
#      define ORTHANC_PUBLIC __attribute__((visibility ("default")))
#      define ORTHANC_LOCAL  __attribute__((visibility ("hidden")))
#    else
#      define ORTHANC_PUBLIC
#      define ORTHANC_LOCAL
#      pragma warning Unknown dynamic link import/export semantics
#    endif
#  endif
#else
#  define ORTHANC_PUBLIC
#  define ORTHANC_LOCAL
#endif

#endif /* __ORTHANC_FRAMEWORK_H */
