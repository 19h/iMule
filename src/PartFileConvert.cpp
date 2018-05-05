//
// This file is part of the imule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include <wx/app.h>

#include "PartFileConvert.h"

#include "amule.h"
#include "DownloadQueue.h"
#include <common/Format.h>
#include "Logger.h"
#include "PartFile.h"
#include "Preferences.h"
#include "SharedFileList.h"
#include <common/FileFunctions.h>
#include <protocol/ed2k/Constants.h>
#include "OtherFunctions.h"
#include "GuiEvents.h"
#include "DataToText.h"

static unsigned s_nextJobId = 0;


struct ConvertJob {
        unsigned	id;
        CPath		folder;
        CPath		filename;
        wxString	filehash;
        int		format;
        ConvStatus	state;
        uint64_t	size;
        uint64_t	spaceneeded;
        uint8		partmettype;
        bool		removeSource;
        ConvertJob()	{ id=s_nextJobId++; size=0; spaceneeded=0; partmettype=PMT_UNKNOWN; removeSource=true; }
};

ConvertInfo::ConvertInfo(ConvertJob *job)
        : id(job->id),
          folder(job->folder), filename(job->filename), filehash(job->filehash),
          state(job->state), size(job->size), spaceneeded(job->spaceneeded)
{}


CPartFileConvert*	CPartFileConvert::s_instance= NULL;
std::list<ConvertJob*>	CPartFileConvert::s_jobs;
ConvertJob*		CPartFileConvert::s_pfconverting = NULL;


int CPartFileConvert::ScanFolderToAdd(const CPath& folder, bool deletesource)
{
        int count = 0;
        CDirIterator finder(folder);

        CPath file = finder.GetFirstFile(CDirIterator::File, wxT("*.part.met"));
        while (file.IsOk()) {
                ConvertToeMule(folder.JoinPaths(file), deletesource);
                file = finder.GetNextFile();
                count++;
        }
        /* Shareaza
        file = finder.GetFirstFile(CDirIterator::File, wxT("*.sd"));
        while (!file.IsEmpty()) {
        	ConvertToeMule(file, deletesource);
        	file = finder.GetNextFile();
        	count++;
        }
        */

        file = finder.GetFirstFile(CDirIterator::Dir, wxT("*.*"));
        while (file.IsOk()) {
                ScanFolderToAdd(folder.JoinPaths(file), deletesource);

                file = finder.GetNextFile();
        }

        return count;
}

void CPartFileConvert::ConvertToeMule(const CPath& file, bool deletesource)
{
        if (!file.FileExists()) {
                return;
        }

        ConvertJob* newjob = new ConvertJob();
        newjob->folder = file;
        newjob->removeSource = deletesource;
        newjob->state = CONV_QUEUE;

        s_jobs.push_back(newjob);

        Notify_ConvertUpdateJobInfo(newjob);

        StartThread();
}

void CPartFileConvert::StartThread()
{
        if (!s_instance) {
                s_instance = new CPartFileConvert();

                AddDebugLogLineN( logPfConvert, wxT("A new thread has been created.") );
                s_instance->Bind(wxEVT_IDLE, &CPartFileConvert::Run, s_instance);
        }
}

void CPartFileConvert::StopThread()
{
        DeleteContents(s_jobs);
        if (s_instance) {
                s_instance->Unbind(wxEVT_IDLE, &CPartFileConvert::Run, s_instance);
                delete s_instance;
                s_instance = NULL;
        } else {
                return;
        }
}

void CPartFileConvert::Run(wxIdleEvent & /*evt*/)
{
        cr_begin(run);
        run.imported = 0;

        while (true) {
                // search next queued job and start it
                s_pfconverting = NULL;
                for (std::list<ConvertJob*>::iterator it = s_jobs.begin(); it != s_jobs.end(); ++it) {
                        if ((*it)->state == CONV_QUEUE) {
                                s_pfconverting = *it;
                                break;
                        }
                }
                
                if (s_pfconverting) {
                        s_pfconverting->state = CONV_INPROGRESS;

                        Notify_ConvertUpdateJobInfo(s_pfconverting);
                        
                        cr_redo(run);
                        s_pfconverting->state = performConvertToeMule(s_pfconverting->folder);
                        
                        if (s_pfconverting->state == CONV_INPROGRESS) {
                                return;
                        }
                        if (s_pfconverting->state == CONV_OK) {
                                ++run.imported;
                        }

                        Notify_ConvertUpdateJobInfo(s_pfconverting);

                        AddLogLineC(CFormat(_("Importing %s: %s")) % s_pfconverting->folder % GetConversionState(s_pfconverting->state));
                } else {
                        break; // nothing more to do now
                }
                // for high cpu usage, requesting a continuous flow of idle events
                //                 evt.RequestMore();
                cr_return(,run);
        }
        
        // clean up
        Notify_ConvertClearInfos();
        
        if (run.imported) {
                theApp->sharedfiles->PublishNextTurn();
        }
        
        AddDebugLogLineN(logPfConvert, wxT("No more jobs on queue, exiting from thread."));
        StopThread();
        
        cr_end(run);
}

static struct ConvertCtx {
        wxString filepartindex, buffer;
        unsigned fileindex;
        
        CPath folder   ;
        CDirIterator * finder;
        CPath partfile ;
        CPath newfilename;
        CPartFile* file ;
        CPath oldfile ;
        CPath oldFile ;
        char *ba;
        CFile inputfile;
        unsigned maxindex;
        unsigned partfilecount ;
        CPath filePath ;
        float stepperpart;
        sint64 freespace;
        unsigned curindex ;
        CPath filename ;
        uint64 toReadWrite;
        uint32 chunkstart ;
        bool ret;
        cr_context(ConvertCtx);
        void reinit() {
                if (ba) delete[] ba;
                if (file) file->Delete();
                if (finder) delete finder;
                __s = 0;
        };
} cvt;

ConvStatus CPartFileConvert::performConvertToeMule(const CPath& fileName)
{
        try {                        // just count
                cr_begin(cvt);
                cvt.ba = NULL;
                cvt.folder    = fileName.GetPath();
                cvt.partfile  = fileName.GetFullName();
                cvt.finder = new CDirIterator(cvt.folder);
                
                Notify_ConvertUpdateProgressFull(0, _("Reading temp folder"), s_pfconverting->folder.GetPrintable());
                
                cvt.filepartindex = cvt.partfile.RemoveAllExt().GetRaw();
                
                Notify_ConvertUpdateProgress(4, _("Retrieving basic information from download info file"));
                cr_return(CONV_INPROGRESS,cvt);
                
                cvt.file = new CPartFile();
                s_pfconverting->partmettype = cvt.file->LoadPartFile(cvt.folder, cvt.partfile, false, true);
                
                switch (s_pfconverting->partmettype) {
                        case PMT_UNKNOWN:
                        case PMT_BADFORMAT:
                                cvt.reinit();
                                return CONV_BADFORMAT;
                }
                
                cvt.oldfile = cvt.folder.JoinPaths(cvt.partfile.RemoveExt());
                s_pfconverting->size = cvt.file->GetFileSize();
                s_pfconverting->filename = cvt.file->GetFileName();
                s_pfconverting->filehash = cvt.file->GetFileHash().Encode();
                
                Notify_ConvertUpdateJobInfo(s_pfconverting);
                cr_return(CONV_INPROGRESS,cvt);
                
                if (theApp->downloadqueue->GetFileByID(cvt.file->GetFileHash())) {
                        cvt.reinit();                                
                        return CONV_ALREADYEXISTS;
                }
                
                if (s_pfconverting->partmettype == PMT_SPLITTED) {
                        cvt.ba = new char [PARTSIZE];
                        
                        cvt.maxindex = 0;
                        cvt.partfilecount = 0;
                        cvt.filePath = cvt.finder->GetFirstFile(CDirIterator::File, cvt.filepartindex + wxT(".*.part"));
                        while (cvt.filePath.IsOk()) {
                                long l;
                                ++cvt.partfilecount;
                                cvt.filePath.GetFullName().RemoveExt().GetExt().ToLong(&l);
                                cvt.fileindex = (unsigned)l;
                                cvt.filePath = cvt.finder->GetNextFile();
                                if (cvt.fileindex > cvt.maxindex) cvt.maxindex = cvt.fileindex;
                        }
                        if (cvt.partfilecount > 0) {
                                cvt.stepperpart = (80.0f / cvt.partfilecount);
                                if (cvt.maxindex * PARTSIZE <= s_pfconverting->size) {
                                        s_pfconverting->spaceneeded = cvt.maxindex * PARTSIZE;
                                } else {
                                        s_pfconverting->spaceneeded = s_pfconverting->size;
                                }
                        } else {
                                cvt.stepperpart = 80.0f;
                                s_pfconverting->spaceneeded = 0;
                        }
                        
                        Notify_ConvertUpdateJobInfo(s_pfconverting);
                        cr_return(CONV_INPROGRESS,cvt);
                        
                        cvt.freespace = CPath::GetFreeSpaceAt(thePrefs::GetTempDir());
                        if (cvt.freespace != wxInvalidOffset) {
                                if (static_cast<uint64>(cvt.freespace) < cvt.maxindex * PARTSIZE) {
                                        cvt.reinit();
                                        return CONV_OUTOFDISKSPACE;
                                }
                        }
                        
                        // create new partmetfile, and remember the new name
                        cvt.file->CreatePartFile();
                        cvt.newfilename = cvt.file->GetFullName();
                        
                        Notify_ConvertUpdateProgress(8, _("Creating destination file"));
                        cr_return(CONV_INPROGRESS,cvt);
                        
                        cvt.file->m_hpartfile.SetLength( s_pfconverting->spaceneeded );
                        
                        cvt.curindex = 0;
                        cvt.filename = cvt.finder->GetFirstFile(CDirIterator::File, cvt.filepartindex + wxT(".*.part"));
                        while (cvt.filename.IsOk()) {
                                // stats
                                ++cvt.curindex;
                                cvt.buffer = CFormat(_("Loading data from old download file (%u of %u)")) % cvt.curindex % cvt.partfilecount;
                                
                                Notify_ConvertUpdateProgress(10 + (cvt.curindex * cvt.stepperpart), cvt.buffer);
                                cr_return(CONV_INPROGRESS,cvt);
                                
                                {
                                        long l;
                                        cvt.filename.GetFullName().RemoveExt().GetExt().ToLong(&l);
                                        cvt.fileindex = (unsigned)l;
                                }
                                if (cvt.fileindex == 0) {
                                        cvt.filename = cvt.finder->GetNextFile();
                                        continue;
                                }
                                
                                cvt.chunkstart = (cvt.fileindex - 1) * PARTSIZE;
                                
                                // open, read data of the part-part-file into buffer, close file
                                cvt.inputfile.Open(cvt.filename, CFile::read);
                                cvt.toReadWrite = std::min<uint64>(PARTSIZE, cvt.inputfile.GetLength());
                                cvt.inputfile.Read(cvt.ba, cvt.toReadWrite);
                                cvt.inputfile.Close();
                                
                                cvt.buffer = CFormat(_("Saving data block into new single download file (%u of %u)")) % cvt.curindex % cvt.partfilecount;
                                
                                Notify_ConvertUpdateProgress(10 + (cvt.curindex * cvt.stepperpart), cvt.buffer);
                                cr_return(CONV_INPROGRESS,cvt);
                                
                                // write the buffered data
                                cvt.file->m_hpartfile.WriteAt(cvt.ba, cvt.chunkstart, cvt.toReadWrite);
                                
                                cvt.filename = cvt.finder->GetNextFile();
                                cr_return(CONV_INPROGRESS,cvt);
                        }
                        delete[] cvt.ba;
                        
                        cvt.file->m_hpartfile.Close();
                }
                // import an external common format partdownload
                else { //if (pfconverting->partmettype==PMT_DEFAULTOLD || pfconverting->partmettype==PMT_NEWOLD || Shareaza  )
                        if (!s_pfconverting->removeSource) {
                                s_pfconverting->spaceneeded = cvt.oldfile.GetFileSize();
                        }
                        
                        Notify_ConvertUpdateJobInfo(s_pfconverting);
                        cr_return(CONV_INPROGRESS,cvt);
                        
                        cvt.freespace = CPath::GetFreeSpaceAt(thePrefs::GetTempDir());
                        if (cvt.freespace == wxInvalidOffset) {
                                cvt.reinit();
                                return CONV_IOERROR;
                        } else if (cvt.freespace < (signed) s_pfconverting->spaceneeded) {
                                cvt.reinit();
                                return CONV_OUTOFDISKSPACE;
                        }
                        
                        cvt.file->CreatePartFile();
                        cvt.newfilename = cvt.file->GetFullName();
                        
                        cvt.file->m_hpartfile.Close();
                        
                        cvt.ret = false;
                        
                        Notify_ConvertUpdateProgress(92, _("Copy"));
                        cr_return(CONV_INPROGRESS,cvt);
                        
                        CPath::RemoveFile(cvt.newfilename.RemoveExt());
                        if (!cvt.oldfile.FileExists()) {
                                // data file does not exist. well, then create a 0 byte big one
                                CFile datafile;
                                cvt.ret = datafile.Create(cvt.newfilename.RemoveExt());
                        } else if (s_pfconverting->removeSource) {
                                cvt.ret = CPath::RenameFile(cvt.oldfile, cvt.newfilename.RemoveExt());
                        } else {
                                cvt.ret = CPath::CloneFile(cvt.oldfile, cvt.newfilename.RemoveExt(), false);
                        }
                        if (!cvt.ret) {
                                cvt.reinit();
                                return CONV_FAILED;
                        }
                        
                }
                
                Notify_ConvertUpdateProgress(94, _("Retrieving source downloadfile information"));
                cr_return(CONV_INPROGRESS, cvt);
                CPath::RemoveFile(cvt.newfilename);
                if (s_pfconverting->removeSource) {
                        CPath::RenameFile(cvt.folder.JoinPaths(cvt.partfile), cvt.newfilename);
                } else {
                        CPath::CloneFile(cvt.folder.JoinPaths(cvt.partfile), cvt.newfilename, false);
                }
                
                cvt.file->m_hashlist.clear();
                
                
                if (!cvt.file->LoadPartFile(thePrefs::GetTempDir(), cvt.file->GetPartMetFileName(), false)) {
                        cvt.reinit();
                        return CONV_BADFORMAT;
                }
                
                if (s_pfconverting->partmettype == PMT_NEWOLD || s_pfconverting->partmettype == PMT_SPLITTED ) {
                        cvt.file->SetCompletedSize(cvt.file->transferred);
                        cvt.file->m_iGainDueToCompression = 0;
                        cvt.file->m_iLostDueToCorruption = 0;
                }
                
                Notify_ConvertUpdateProgress(100, _("Adding download and saving new partfile"));
                cr_return(CONV_INPROGRESS, cvt);
                theApp->downloadqueue->AddDownload(cvt.file, thePrefs::AddNewFilesPaused(), 0);
                cvt.file->SavePartFile();
                
                if (cvt.file->GetStatus(true) == PS_READY) {
                        theApp->sharedfiles->SafeAddKFile(cvt.file); // part files are always shared files
                }
                
                if (s_pfconverting->removeSource) {
                        cvt.oldFile = cvt.finder->GetFirstFile(CDirIterator::File, cvt.filepartindex + wxT(".*"));
                        while (cvt.oldFile.IsOk()) {
                                CPath::RemoveFile(cvt.folder.JoinPaths(cvt.oldFile));
                                cr_return(CONV_INPROGRESS,cvt);
                                cvt.oldFile = cvt.finder->GetNextFile();
                        }
                        
                        if (s_pfconverting->partmettype == PMT_SPLITTED) {
                                CPath::RemoveDir(cvt.folder);
                        }
                }
                delete cvt.finder;
                cr_end(cvt);
                return CONV_OK;
        } catch (const CSafeIOException& e) {
                AddDebugLogLineC(logPfConvert, wxT("IO error while converting partfiles: ") + e.what());
                cvt.reinit();
                return CONV_IOERROR;
        }
}

// Notification handlers

void CPartFileConvert::RemoveJob(unsigned id)
{
        for (std::list<ConvertJob*>::iterator it = s_jobs.begin(); it != s_jobs.end(); ++it) {
                if ((*it)->id == id && (*it)->state != CONV_INPROGRESS) {
                        ConvertJob *job = *it;
                        s_jobs.erase(it);
                        Notify_ConvertRemoveJobInfo(id);
                        delete job;
                        break;
                }
        }
}

void CPartFileConvert::RetryJob(unsigned id)
{
        for (std::list<ConvertJob*>::iterator it = s_jobs.begin(); it != s_jobs.end(); ++it) {
                if ((*it)->id == id && (*it)->state != CONV_INPROGRESS && (*it)->state != CONV_OK) {
                        (*it)->state = CONV_QUEUE;
                        Notify_ConvertUpdateJobInfo(*it);
                        StartThread();
                        break;
                }
        }
}

void CPartFileConvert::ReaddAllJobs()
{
        for (std::list<ConvertJob*>::iterator it = s_jobs.begin(); it != s_jobs.end(); ++it) {
                Notify_ConvertUpdateJobInfo(*it);
        }
}