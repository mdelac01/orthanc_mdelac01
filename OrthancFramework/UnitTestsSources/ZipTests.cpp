/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2021 Osimis S.A., Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/


#if ORTHANC_UNIT_TESTS_LINK_FRAMEWORK == 1
// Must be the first to be sure to use the Orthanc framework shared library
#  include <OrthancFramework.h>
#endif

#include <gtest/gtest.h>

#include "../Sources/Compression/HierarchicalZipWriter.h"
#include "../Sources/Compression/ZipReader.h"
#include "../Sources/OrthancException.h"
#include "../Sources/TemporaryFile.h"
#include "../Sources/Toolbox.h"


using namespace Orthanc;

TEST(ZipWriter, Basic)
{
  Orthanc::ZipWriter w;
  w.SetOutputPath("UnitTestsResults/hello.zip");
  w.Open();
  w.OpenFile("world/hello");
  w.Write("Hello world");
}


TEST(ZipWriter, Basic64)
{
  Orthanc::ZipWriter w;
  w.SetOutputPath("UnitTestsResults/hello64.zip");
  w.SetZip64(true);
  w.Open();
  w.OpenFile("world/hello");
  w.Write("Hello world");
}


TEST(ZipWriter, Exceptions)
{
  Orthanc::ZipWriter w;
  ASSERT_THROW(w.Open(), Orthanc::OrthancException);
  w.SetOutputPath("UnitTestsResults/hello3.zip");
  w.Open();
  ASSERT_THROW(w.Write("hello world"), Orthanc::OrthancException);
}


TEST(ZipWriter, Append)
{
  {
    Orthanc::ZipWriter w;
    w.SetAppendToExisting(false);
    w.SetOutputPath("UnitTestsResults/append.zip");
    w.Open();
    w.OpenFile("world/hello");
    w.Write("Hello world 1");
  }

  {
    Orthanc::ZipWriter w;
    w.SetAppendToExisting(true);
    w.SetOutputPath("UnitTestsResults/append.zip");
    w.Open();
    w.OpenFile("world/appended");
    w.Write("Hello world 2");
  }
}





namespace Orthanc
{
  // The namespace is necessary
  // http://code.google.com/p/googletest/wiki/AdvancedGuide#Private_Class_Members

  TEST(HierarchicalZipWriter, Index)
  {
    HierarchicalZipWriter::Index i;
    ASSERT_EQ("hello", i.OpenFile("hello"));
    ASSERT_EQ("hello-2", i.OpenFile("hello"));
    ASSERT_EQ("coucou", i.OpenFile("coucou"));
    ASSERT_EQ("hello-3", i.OpenFile("hello"));

    i.OpenDirectory("coucou");

    ASSERT_EQ("coucou-2/world", i.OpenFile("world"));
    ASSERT_EQ("coucou-2/world-2", i.OpenFile("world"));

    i.OpenDirectory("world");
  
    ASSERT_EQ("coucou-2/world-3/hello", i.OpenFile("hello"));
    ASSERT_EQ("coucou-2/world-3/hello-2", i.OpenFile("hello"));

    i.CloseDirectory();

    ASSERT_EQ("coucou-2/world-4", i.OpenFile("world"));

    i.CloseDirectory();

    ASSERT_EQ("coucou-3", i.OpenFile("coucou"));

    ASSERT_THROW(i.CloseDirectory(), OrthancException);
  }


  TEST(HierarchicalZipWriter, Filenames)
  {
    ASSERT_EQ("trE hell", HierarchicalZipWriter::Index::KeepAlphanumeric("    ??trE hell??  "));

    // The "^" character is considered as a space in DICOM
    ASSERT_EQ("Hel lo world", HierarchicalZipWriter::Index::KeepAlphanumeric("    Hel^^  ^\r\n\t^^lo  \t  <world>  "));
  }
}


TEST(HierarchicalZipWriter, Basic)
{
  static const std::string SPACES = "                             ";

  HierarchicalZipWriter w("UnitTestsResults/hello2.zip");

  w.SetCompressionLevel(0);

  // Inside "/"
  w.OpenFile("hello");
  w.Write(SPACES + "hello\n");
  w.OpenFile("hello");
  w.Write(SPACES + "hello-2\n");
  w.OpenDirectory("hello");

  // Inside "/hello-3"
  w.OpenFile("hello");
  w.Write(SPACES + "hello\n");
  w.OpenDirectory("hello");

  w.SetCompressionLevel(9);

  // Inside "/hello-3/hello-2"
  w.OpenFile("hello");
  w.Write(SPACES + "hello\n");
  w.OpenFile("hello");
  w.Write(SPACES + "hello-2\n");
  w.CloseDirectory();

  // Inside "/hello-3"
  w.OpenFile("hello");
  w.Write(SPACES + "hello-3\n");

  /**

     TO CHECK THE CONTENT OF THE "hello2.zip" FILE:

     # unzip -v hello2.zip 

     => There must be 6 files. The first 3 files must have a negative
     compression ratio.

  **/
}


TEST(ZipReader, Basic)
{
  TemporaryFile f;
  
  {
    Orthanc::ZipWriter w;
    w.SetOutputPath(f.GetPath().c_str());
    w.Open();
    w.OpenFile("world/hello");
    w.Write("Hello world");
  }

  ASSERT_TRUE(ZipReader::IsZipFile(f.GetPath()));

  std::unique_ptr<ZipReader> reader(ZipReader::CreateFromFile(f.GetPath()));

  ASSERT_EQ(1u, reader->GetFilesCount());
  
  std::string filename, content;
  ASSERT_TRUE(reader->ReadNextFile(filename, content));
  ASSERT_EQ("world/hello", filename);
  ASSERT_EQ("Hello world", content);
  ASSERT_FALSE(reader->ReadNextFile(filename, content));
}

