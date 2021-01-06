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


#include "../PrecompiledHeaders.h"
#include "DicomServer.h"

#include "../Logging.h"
#include "../MultiThreading/RunnableWorkersPool.h"
#include "../OrthancException.h"
#include "../SystemToolbox.h"
#include "../Toolbox.h"
#include "Internals/CommandDispatcher.h"

#include <boost/thread.hpp>

#if ORTHANC_ENABLE_SSL == 1
#  include "Internals/DicomTls.h"
#endif

#if defined(__linux__)
#  include <cstdlib>
#endif


namespace Orthanc
{
  struct DicomServer::PImpl
  {
    boost::thread  thread_;
    T_ASC_Network *network_;
    std::unique_ptr<RunnableWorkersPool>  workers_;

#if ORTHANC_ENABLE_SSL == 1
    std::unique_ptr<DcmTLSTransportLayer> tls_;
#endif
  };


  void DicomServer::ServerThread(DicomServer* server,
                                 bool useDicomTls)
  {
    CLOG(INFO, DICOM) << "DICOM server started";

    while (server->continue_)
    {
      /* receive an association and acknowledge or reject it. If the association was */
      /* acknowledged, offer corresponding services and invoke one or more if required. */
      std::unique_ptr<Internals::CommandDispatcher> dispatcher(
        Internals::AcceptAssociation(*server, server->pimpl_->network_, useDicomTls));

      try
      {
        if (dispatcher.get() != NULL)
        {
          server->pimpl_->workers_->Add(dispatcher.release());
        }
      }
      catch (OrthancException& e)
      {
        CLOG(ERROR, DICOM) << "Exception in the DICOM server thread: " << e.What();
      }
    }

    CLOG(INFO, DICOM) << "DICOM server stopping";
  }


  DicomServer::DicomServer() : 
    pimpl_(new PImpl),
    aet_("ANY-SCP")
  {
    port_ = 104;
    modalities_ = NULL;
    findRequestHandlerFactory_ = NULL;
    moveRequestHandlerFactory_ = NULL;
    getRequestHandlerFactory_ = NULL;
    storeRequestHandlerFactory_ = NULL;
    worklistRequestHandlerFactory_ = NULL;
    storageCommitmentFactory_ = NULL;
    applicationEntityFilter_ = NULL;
    checkCalledAet_ = true;
    associationTimeout_ = 30;
    continue_ = false;
  }

  DicomServer::~DicomServer()
  {
    if (continue_)
    {
      CLOG(ERROR, DICOM) << "INTERNAL ERROR: DicomServer::Stop() should be invoked manually to avoid mess in the destruction order!";
      Stop();
    }
  }

  void DicomServer::SetPortNumber(uint16_t port)
  {
    Stop();
    port_ = port;
  }

  uint16_t DicomServer::GetPortNumber() const
  {
    return port_;
  }

  void DicomServer::SetAssociationTimeout(uint32_t seconds)
  {
    CLOG(INFO, DICOM) << "Setting timeout for DICOM connections if Orthanc acts as SCP (server): " 
                      << seconds << " seconds (0 = no timeout)";

    Stop();
    associationTimeout_ = seconds;
  }

  uint32_t DicomServer::GetAssociationTimeout() const
  {
    return associationTimeout_;
  }


  void DicomServer::SetCalledApplicationEntityTitleCheck(bool check)
  {
    Stop();
    checkCalledAet_ = check;
  }

  bool DicomServer::HasCalledApplicationEntityTitleCheck() const
  {
    return checkCalledAet_;
  }

  void DicomServer::SetApplicationEntityTitle(const std::string& aet)
  {
    if (aet.size() == 0)
    {
      throw OrthancException(ErrorCode_BadApplicationEntityTitle);
    }

    if (aet.size() > 16)
    {
      throw OrthancException(ErrorCode_BadApplicationEntityTitle);
    }

    for (size_t i = 0; i < aet.size(); i++)
    {
      if (!(aet[i] == '-' ||
            aet[i] == '_' ||
            isdigit(aet[i]) ||
            (aet[i] >= 'A' && aet[i] <= 'Z')))
      {
        CLOG(WARNING, DICOM) << "For best interoperability, only upper case, alphanumeric characters should be present in AET: \"" << aet << "\"";
        break;
      }
    }

    Stop();
    aet_ = aet;
  }

  const std::string& DicomServer::GetApplicationEntityTitle() const
  {
    return aet_;
  }

  void DicomServer::SetRemoteModalities(IRemoteModalities& modalities)
  {
    Stop();
    modalities_ = &modalities;
  }
  
  DicomServer::IRemoteModalities& DicomServer::GetRemoteModalities() const
  {
    if (modalities_ == NULL)
    {
      throw OrthancException(ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      return *modalities_;
    }
  }
    
  void DicomServer::SetFindRequestHandlerFactory(IFindRequestHandlerFactory& factory)
  {
    Stop();
    findRequestHandlerFactory_ = &factory;
  }

  bool DicomServer::HasFindRequestHandlerFactory() const
  {
    return (findRequestHandlerFactory_ != NULL);
  }

  IFindRequestHandlerFactory& DicomServer::GetFindRequestHandlerFactory() const
  {
    if (HasFindRequestHandlerFactory())
    {
      return *findRequestHandlerFactory_;
    }
    else
    {
      throw OrthancException(ErrorCode_NoCFindHandler);
    }
  }

  void DicomServer::SetMoveRequestHandlerFactory(IMoveRequestHandlerFactory& factory)
  {
    Stop();
    moveRequestHandlerFactory_ = &factory;
  }

  bool DicomServer::HasMoveRequestHandlerFactory() const
  {
    return (moveRequestHandlerFactory_ != NULL);
  }

  IMoveRequestHandlerFactory& DicomServer::GetMoveRequestHandlerFactory() const
  {
    if (HasMoveRequestHandlerFactory())
    {
      return *moveRequestHandlerFactory_;
    }
    else
    {
      throw OrthancException(ErrorCode_NoCMoveHandler);
    }
  }

  void DicomServer::SetGetRequestHandlerFactory(IGetRequestHandlerFactory& factory)
  {
    Stop();
    getRequestHandlerFactory_ = &factory;
  }

  bool DicomServer::HasGetRequestHandlerFactory() const
  {
    return (getRequestHandlerFactory_ != NULL);
  }

  IGetRequestHandlerFactory& DicomServer::GetGetRequestHandlerFactory() const
  {
    if (HasGetRequestHandlerFactory())
    {
      return *getRequestHandlerFactory_;
    }
    else
    {
      throw OrthancException(ErrorCode_NoCGetHandler);
    }
  }

  void DicomServer::SetStoreRequestHandlerFactory(IStoreRequestHandlerFactory& factory)
  {
    Stop();
    storeRequestHandlerFactory_ = &factory;
  }

  bool DicomServer::HasStoreRequestHandlerFactory() const
  {
    return (storeRequestHandlerFactory_ != NULL);
  }

  IStoreRequestHandlerFactory& DicomServer::GetStoreRequestHandlerFactory() const
  {
    if (HasStoreRequestHandlerFactory())
    {
      return *storeRequestHandlerFactory_;
    }
    else
    {
      throw OrthancException(ErrorCode_NoCStoreHandler);
    }
  }

  void DicomServer::SetWorklistRequestHandlerFactory(IWorklistRequestHandlerFactory& factory)
  {
    Stop();
    worklistRequestHandlerFactory_ = &factory;
  }

  bool DicomServer::HasWorklistRequestHandlerFactory() const
  {
    return (worklistRequestHandlerFactory_ != NULL);
  }

  IWorklistRequestHandlerFactory& DicomServer::GetWorklistRequestHandlerFactory() const
  {
    if (HasWorklistRequestHandlerFactory())
    {
      return *worklistRequestHandlerFactory_;
    }
    else
    {
      throw OrthancException(ErrorCode_NoWorklistHandler);
    }
  }

  void DicomServer::SetStorageCommitmentRequestHandlerFactory(IStorageCommitmentRequestHandlerFactory& factory)
  {
    Stop();
    storageCommitmentFactory_ = &factory;
  }

  bool DicomServer::HasStorageCommitmentRequestHandlerFactory() const
  {
    return (storageCommitmentFactory_ != NULL);
  }

  IStorageCommitmentRequestHandlerFactory& DicomServer::GetStorageCommitmentRequestHandlerFactory() const
  {
    if (HasStorageCommitmentRequestHandlerFactory())
    {
      return *storageCommitmentFactory_;
    }
    else
    {
      throw OrthancException(ErrorCode_NoStorageCommitmentHandler);
    }
  }

  void DicomServer::SetApplicationEntityFilter(IApplicationEntityFilter& factory)
  {
    Stop();
    applicationEntityFilter_ = &factory;
  }

  bool DicomServer::HasApplicationEntityFilter() const
  {
    return (applicationEntityFilter_ != NULL);
  }

  IApplicationEntityFilter& DicomServer::GetApplicationEntityFilter() const
  {
    if (HasApplicationEntityFilter())
    {
      return *applicationEntityFilter_;
    }
    else
    {
      throw OrthancException(ErrorCode_NoApplicationEntityFilter);
    }
  }


  void DicomServer::Start()
  {
    if (modalities_ == NULL)
    {
      throw OrthancException(ErrorCode_BadSequenceOfCalls,
                             "No list of modalities was provided to the DICOM server");
    }
    
    Stop();

    /* initialize network, i.e. create an instance of T_ASC_Network*. */
    OFCondition cond = ASC_initializeNetwork
      (NET_ACCEPTOR, OFstatic_cast(int, port_), /*opt_acse_timeout*/ 30, &pimpl_->network_);
    if (cond.bad())
    {
      throw OrthancException(ErrorCode_DicomPortInUse,
                             " (port = " + boost::lexical_cast<std::string>(port_) +
                             ") cannot create network: " + std::string(cond.text()));
    }

    bool useDicomTls = false;    // TODO - Read from configuration option

#if ORTHANC_ENABLE_SSL == 1
    assert(pimpl_->tls_.get() == NULL);

    if (useDicomTls)
    {
      try
      {
        // TODO - Configuration options
        pimpl_->tls_.reset(Internals::InitializeDicomTls(pimpl_->network_, NET_ACCEPTOR,
                                                         "/tmp/j/Server.key", "/tmp/j/Server.crt", "/tmp/j/Client.crt"));
      }
      catch (OrthancException&)
      {
        ASC_dropNetwork(&pimpl_->network_);
        throw;
      }
    }
#endif

    if (useDicomTls)
    {
      CLOG(INFO, DICOM) << "Orthanc SCP will use DICOM TLS";
    }
    else
    {
      CLOG(INFO, DICOM) << "Orthanc SCP will *not* use DICOM TLS";
    }

    continue_ = true;
    pimpl_->workers_.reset(new RunnableWorkersPool(4));   // Use 4 workers - TODO as a parameter?
    pimpl_->thread_ = boost::thread(ServerThread, this, useDicomTls);
  }


  void DicomServer::Stop()
  {
    if (continue_)
    {
      continue_ = false;

      if (pimpl_->thread_.joinable())
      {
        pimpl_->thread_.join();
      }

      pimpl_->workers_.reset(NULL);

#if ORTHANC_ENABLE_SSL == 1
      pimpl_->tls_.reset(NULL);  // Transport layer must be destroyed before the association itself
#endif

      /* drop the network, i.e. free memory of T_ASC_Network* structure. This call */
      /* is the counterpart of ASC_initializeNetwork(...) which was called above. */
      OFCondition cond = ASC_dropNetwork(&pimpl_->network_);
      if (cond.bad())
      {
        CLOG(ERROR, DICOM) << "Error while dropping the network: " << cond.text();
      }
    }
  }


  bool DicomServer::IsMyAETitle(const std::string& aet) const
  {
    if (modalities_ == NULL)
    {
      throw OrthancException(ErrorCode_BadSequenceOfCalls);
    }
    
    if (!HasCalledApplicationEntityTitleCheck())
    {
      // OK, no check on the AET.
      return true;
    }
    else
    {
      return modalities_->IsSameAETitle(aet, GetApplicationEntityTitle());
    }
  }
}
