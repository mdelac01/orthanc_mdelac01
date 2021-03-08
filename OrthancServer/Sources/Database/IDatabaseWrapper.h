/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2021 Osimis S.A., Belgium
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

#include "../../../OrthancFramework/Sources/DicomFormat/DicomMap.h"
#include "../../../OrthancFramework/Sources/FileStorage/FileInfo.h"
#include "../../../OrthancFramework/Sources/FileStorage/IStorageArea.h"
#include "../../../OrthancFramework/Sources/SQLite/ITransaction.h"

#include "../ExportedResource.h"
#include "IDatabaseListener.h"

#include <list>
#include <boost/noncopyable.hpp>
#include <set>

namespace Orthanc
{
  class DatabaseConstraint;
  class ResourcesContent;

  
  class IDatabaseWrapper : public boost::noncopyable
  {
  public:
    class ITransaction : public boost::noncopyable
    {
    public:
      virtual ~ITransaction()
      {
      }

      virtual void Rollback() = 0;

      virtual void Commit(int64_t fileSizeDelta) = 0;
    };


    struct CreateInstanceResult
    {
      bool     isNewPatient_;
      bool     isNewStudy_;
      bool     isNewSeries_;
      int64_t  patientId_;
      int64_t  studyId_;
      int64_t  seriesId_;
    };

    virtual ~IDatabaseWrapper()
    {
    }

    virtual void Open() = 0;

    virtual void Close() = 0;

    virtual void AddAttachment(int64_t id,
                               const FileInfo& attachment) = 0;

    virtual void ClearChanges() = 0;

    virtual void ClearExportedResources() = 0;

    virtual void DeleteAttachment(int64_t id,
                                  FileContentType attachment) = 0;

    virtual void DeleteMetadata(int64_t id,
                                MetadataType type) = 0;

    virtual void DeleteResource(int64_t id) = 0;

    virtual void FlushToDisk() = 0;

    virtual bool HasFlushToDisk() const = 0;

    virtual void GetAllMetadata(std::map<MetadataType, std::string>& target,
                                int64_t id) = 0;

    virtual void GetAllPublicIds(std::list<std::string>& target,
                                 ResourceType resourceType) = 0;

    virtual void GetAllPublicIds(std::list<std::string>& target,
                                 ResourceType resourceType,
                                 size_t since,
                                 size_t limit) = 0;

    virtual void GetChanges(std::list<ServerIndexChange>& target /*out*/,
                            bool& done /*out*/,
                            int64_t since,
                            uint32_t maxResults) = 0;

    virtual void GetChildrenInternalId(std::list<int64_t>& target,
                                       int64_t id) = 0;

    virtual void GetChildrenPublicId(std::list<std::string>& target,
                                     int64_t id) = 0;

    virtual void GetExportedResources(std::list<ExportedResource>& target /*out*/,
                                      bool& done /*out*/,
                                      int64_t since,
                                      uint32_t maxResults) = 0;

    virtual void GetLastChange(std::list<ServerIndexChange>& target /*out*/) = 0;

    virtual void GetLastExportedResource(std::list<ExportedResource>& target /*out*/) = 0;

    virtual void GetMainDicomTags(DicomMap& map,
                                  int64_t id) = 0;

    virtual std::string GetPublicId(int64_t resourceId) = 0;

    virtual uint64_t GetResourceCount(ResourceType resourceType) = 0;

    virtual ResourceType GetResourceType(int64_t resourceId) = 0;

    virtual uint64_t GetTotalCompressedSize() = 0;
    
    virtual uint64_t GetTotalUncompressedSize() = 0;

    virtual bool IsExistingResource(int64_t internalId) = 0;

    virtual bool IsProtectedPatient(int64_t internalId) = 0;

    virtual void ListAvailableAttachments(std::set<FileContentType>& target,
                                          int64_t id) = 0;

    virtual void LogChange(int64_t internalId,
                           const ServerIndexChange& change) = 0;

    virtual void LogExportedResource(const ExportedResource& resource) = 0;
    
    virtual bool LookupAttachment(FileInfo& attachment,
                                  int64_t id,
                                  FileContentType contentType) = 0;

    virtual bool LookupGlobalProperty(std::string& target,
                                      GlobalProperty property) = 0;

    virtual bool LookupMetadata(std::string& target,
                                int64_t id,
                                MetadataType type) = 0;

    virtual bool LookupParent(int64_t& parentId,
                              int64_t resourceId) = 0;

    virtual bool LookupResource(int64_t& id,
                                ResourceType& type,
                                const std::string& publicId) = 0;

    virtual bool SelectPatientToRecycle(int64_t& internalId) = 0;

    virtual bool SelectPatientToRecycle(int64_t& internalId,
                                        int64_t patientIdToAvoid) = 0;

    virtual void SetGlobalProperty(GlobalProperty property,
                                   const std::string& value) = 0;

    virtual void ClearMainDicomTags(int64_t id) = 0;

    virtual void SetMetadata(int64_t id,
                             MetadataType type,
                             const std::string& value) = 0;

    virtual void SetProtectedPatient(int64_t internalId, 
                                     bool isProtected) = 0;

    virtual ITransaction* StartTransaction(TransactionType type) = 0;

    virtual void SetListener(IDatabaseListener& listener) = 0;

    virtual unsigned int GetDatabaseVersion() = 0;

    virtual void Upgrade(unsigned int targetVersion,
                         IStorageArea& storageArea) = 0;


    /**
     * Primitives introduced in Orthanc 1.5.2
     **/
    
    virtual bool IsDiskSizeAbove(uint64_t threshold) = 0;
    
    virtual void ApplyLookupResources(std::list<std::string>& resourcesId,
                                      std::list<std::string>* instancesId, // Can be NULL if not needed
                                      const std::vector<DatabaseConstraint>& lookup,
                                      ResourceType queryLevel,
                                      size_t limit) = 0;

    // Returns "true" iff. the instance is new and has been inserted
    // into the database. If "false" is returned, the content of
    // "result" is undefined, but "instanceId" must be properly
    // set. This method must also tag the parent patient as the most
    // recent in the patient recycling order if it is not protected
    // (so as to fix issue #58).
    virtual bool CreateInstance(CreateInstanceResult& result, /* out */
                                int64_t& instanceId,          /* out */
                                const std::string& patient,
                                const std::string& study,
                                const std::string& series,
                                const std::string& instance) = 0;

    // It is guaranteed that the resources to be modified have no main
    // DICOM tags, and no DICOM identifiers associated with
    // them. However, some metadata might be already existing, and
    // have to be overwritten.
    virtual void SetResourcesContent(const ResourcesContent& content) = 0;

    virtual void GetChildrenMetadata(std::list<std::string>& target,
                                     int64_t resourceId,
                                     MetadataType metadata) = 0;

    virtual int64_t GetLastChangeIndex() = 0;


    /**
     * Primitives introduced in Orthanc 1.5.4
     **/

    virtual bool LookupResourceAndParent(int64_t& id,
                                         ResourceType& type,
                                         std::string& parentPublicId,
                                         const std::string& publicId) = 0;
  };
}
