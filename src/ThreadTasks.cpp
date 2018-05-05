//
// This file is part of the aMule Project.
//
// Copyright (c) 2006-2011 Mikkel Schubert ( xaignar@amule.org / http:://www.amule.org )
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


#include <wx/app.h>			// Needed for wxTheApp

#include "ThreadTasks.h"		// Interface declarations
#include "PartFile.h"			// Needed for CPartFile
#include "Logger.h"			// Needed for Add(Debug)LogLine{C,N}
#include <common/Format.h>		// Needed for CFormat
#include "amule.h"			// Needed for theApp
#include "KnownFileList.h"		// Needed for theApp->knownfiles
#include "Preferences.h"		// Needed for thePrefs
#include "PlatformSpecific.h"		// Needed for CanFSHandleSpecialChars
#include <memory>

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

//! This hash represents the value for an empty MD4 hashing
const byte g_emptyMD4Hash[16] = {
        0x31, 0xD6, 0xCF, 0xE0, 0xD1, 0x6A, 0xE9, 0x31,
        0xB7, 0x3C, 0x59, 0xD7, 0xE0, 0xC0, 0x89, 0xC0
};


////////////////////////////////////////////////////////////
// CHashingTask

CHashingTask::CHashingTask(const CPath& path, const CPath& filename, const CPartFile* part)
// GetPrintable is used to improve the readability of the log.
        : CThreadTask(wxT("Hashing"), path.JoinPaths(filename).GetPrintable(), (part ? ETP_High : ETP_Normal)),
          m_path(path),
          m_filename(filename),
          m_toHash((EHashes)(EH_MD4 | EH_AICH)),
          m_owner(part)
{
        // We can only create the AICH hashset if the file is a knownfile or
        // if the partfile is complete, since the MD4 hashset is checked first,
        // so that the AICH hashset only gets assigned if the MD4 hashset
        // matches what we expected. Due to the rareity of post-completion
        // corruptions, this gives us a nice speedup in most cases.
        if (part && !part->GetGapList().empty()) {
                m_toHash = EH_MD4;
        }
        AddDebugLogLineN(logHasher, CFormat(wxT("Hashing thread created for %s")) % m_filename);
}


CHashingTask::CHashingTask(const CKnownFile* toAICHHash)
// GetPrintable is used to improve the readability of the log.
        : CThreadTask(wxT("AICH Hashing"), toAICHHash->GetFilePath().JoinPaths(toAICHHash->GetFileName()).GetPrintable(), ETP_Low),
          m_path(toAICHHash->GetFilePath()),
          m_filename(toAICHHash->GetFileName()),
          m_toHash(EH_AICH),
          m_owner(toAICHHash)
{
        AddDebugLogLineN(logHasher,
                         CFormat(wxT("Hashing thread created for %s")) % m_filename);
}

bool CHashingTask::Continue()
{
        try {
                CR_BEGIN;
                AddDebugLogLineN(logHasher,
                                 CFormat(wxT("Hashing Entry")) );
                cr.fullPath = m_path.JoinPaths(m_filename);
                AddDebugLogLineN(logHasher,
                                 CFormat(wxT("Hashing Entry %s")) % cr.fullPath);
                if (!cr.file.Open(cr.fullPath, CFile::read)) {
                        AddDebugLogLineC(logHasher,
                                         CFormat(wxT("Warning, failed to open file, skipping: %s")) % cr.fullPath);
                        CR_EXIT;
                }
                
                cr.fileLength = 0;
                try {
                        cr.fileLength = cr.file.GetLength();
                } catch (const CIOFailureException&) {
                        AddDebugLogLineC(logHasher,
                                         CFormat(wxT("Warning, failed to retrieve file-length, skipping: %s")) % cr.fullPath);
                        CR_EXIT;
                }
                
                if (cr.fileLength > MAX_FILE_SIZE) {
                        AddDebugLogLineC(logHasher,
                                         CFormat(wxT("Warning, file is larger than supported size, skipping: %s")) % cr.fullPath);
                        CR_EXIT;
                } else if (cr.fileLength == 0) {
                        if (m_owner) {
                                // It makes no sense to try to hash empty partfiles ...
                                wxFAIL;
                        } else {
                                // Zero-size partfiles should be hashed, but not zero-sized shared-files.
                                AddDebugLogLineC(logHasher,
                                                 CFormat(wxT("Warning, 0-size file, skipping: %s")) % cr.fullPath);
                        }
                        
                        CR_EXIT;
                }
                
                // For thread-safety, results are passed via a temporary file object.
                cr.knownfile.reset(new CKnownFile);
                cr.knownfile->m_filePath = m_path;
                cr.knownfile->SetFileName(m_filename);
                cr.knownfile->SetFileSize(cr.fileLength);
                cr.knownfile->m_lastDateChanged = CPath::GetModificationTime(cr.fullPath);
                cr.knownfile->m_AvailPartFrequency.insert(cr.knownfile->m_AvailPartFrequency.begin(),
                                                          cr.knownfile->GetPartCount(), 0);
                
                if ((m_toHash & EH_MD4) && (m_toHash & EH_AICH)) {
                        cr.knownfile->GetAICHHashset()->FreeHashSet();
                        AddDebugLogLineN( logHasher, CFormat(
                                wxT("Starting to create MD4 and AICH hash for file: %s")) %
                                m_filename );
                } else if ((m_toHash & EH_MD4)) {
                        AddDebugLogLineN( logHasher, CFormat(
                                wxT("Starting to create MD4 hash for file: %s")) % m_filename );
                } else if ((m_toHash & EH_AICH)) {
                        cr.knownfile->GetAICHHashset()->FreeHashSet();
                        AddDebugLogLineN( logHasher, CFormat(
                                wxT("Starting to create AICH hash for file: %s")) % m_filename );
                } else {
                        wxFAIL_COND_MSG(0, (CFormat(wxT("No hashes requested for file, skipping: %s")) % m_filename).GetString());
                        CR_EXIT;
                }
                
                
                // This loops creates the part-hashes, loop-de-loop.
                for (cr.part = 0; cr.part < cr.knownfile->GetPartCount() && !TestDestroy(); cr.part++) {
                        CR_RETURN;
                        if (not CreateNextPartHash(cr.file, cr.part, cr.knownfile.get(), m_toHash)) {
                                AddDebugLogLineC(logHasher,
                                                 CFormat(wxT("Error while hashing file, skipping: %s"))
                                                 % m_filename);
                                
                                CR_EXIT;
                        }
                }
                
                CR_RETURN;
                
                if ((m_toHash & EH_MD4) && !TestDestroy()) {
                        // If the file is < PARTSIZE, then the filehash is that one hash,
                        // otherwise, the filehash is the hash of the parthashes
                        if ( cr.knownfile->m_hashlist.size() == 1 ) {
                                cr.knownfile->m_abyFileHash = cr.knownfile->m_hashlist[0];
                                cr.knownfile->m_hashlist.clear();
                        } else if ( cr.knownfile->m_hashlist.size() ) {
                                CMD4Hash hash;
                                cr.knownfile->CreateHashFromHashlist(cr.knownfile->m_hashlist, &hash);
                                cr.knownfile->m_abyFileHash = hash;
                        } else {
                                // This should not happen!
                                wxFAIL;
                        }
                }
                
                CR_RETURN;

                // Did we create a AICH hashset?
                if ((m_toHash & EH_AICH) && !TestDestroy()) {
                        CAICHHashSet* AICHHashSet = cr.knownfile->GetAICHHashset();
                        
                        AICHHashSet->ReCalculateHash(false);
                        if (AICHHashSet->VerifyHashTree(true) ) {
                                AICHHashSet->SetStatus(AICH_HASHSETCOMPLETE);
                                if (!AICHHashSet->SaveHashSet()) {
                                        AddDebugLogLineC( logHasher,
                                                          CFormat(wxT("Warning, failed to save AICH hashset for file: %s"))
                                                          % m_filename );
                                }
                        }
                }
                
                if ((m_toHash == EH_AICH) && !TestDestroy()) {
                        CHashingEvent * evt = new CHashingEvent(MULE_EVT_AICH_HASHING, cr.knownfile.release(), m_owner);
                        
                        wxQueueEvent(wxTheApp, evt);
                } else if (!TestDestroy()) {
                        CHashingEvent * evt = new CHashingEvent(MULE_EVT_HASHING, cr.knownfile.release(), m_owner);
                        
                        wxQueueEvent(wxTheApp, evt);
                }
                
                CR_END;
                
        } catch (const CSafeIOException& e) {
                AddDebugLogLineC(logHasher, wxT("IO exception while hashing file: ") + e.what());
        }
        CR_EXIT;
}


bool CHashingTask::CreateNextPartHash(CFileAutoClose& file, uint16 part, CKnownFile* owner, EHashes toHash)
{
        wxCHECK_MSG(!file.Eof(), false, wxT("Unexpected EOF in CreateNextPartHash"));

        const uint64 offset = part * PARTSIZE;
        // We'll read at most PARTSIZE bytes per cycle
        const uint64 partLength = owner->GetPartSize(part);

        CMD4Hash hash;
        CMD4Hash* md4Hash = ((toHash & EH_MD4) ? &hash : NULL);
        CAICHHashTree* aichHash = NULL;

        // Setup for AICH hashing
        if (toHash & EH_AICH) {
                aichHash = owner->GetAICHHashset()->m_pHashTree.FindHash(offset, partLength);
        }

        owner->CreateHashFromFile(file, offset, partLength, md4Hash, aichHash);

        if (toHash & EH_MD4) {
                // Store the md4 hash
                owner->m_hashlist.push_back(hash);

                // This is because of the ed2k implementation for parts. A 2 * PARTSIZE
                // file i.e. will have 3 parts (see CKnownFile::SetFileSize for comments).
                // So we have to create the hash for the 0-size data, which will be the default
                // md4 hash for null data: 31D6CFE0D16AE931B73C59D7E0C089C0
                if ((partLength == PARTSIZE) && file.Eof()) {
                        owner->m_hashlist.push_back(CMD4Hash(g_emptyMD4Hash));
                }
        }

        return true;
}


void CHashingTask::OnLastTask()
{
        if (GetType() == wxT("Hashing")) {
                // To prevent rehashing in case of crashes, we
                // explicity save the list of hashed files here.
                theApp->knownfiles->Save();

                // Make sure the AICH-hashes are up to date.
                CThreadScheduler::AddTask(new CAICHSyncTask());
        }
}


////////////////////////////////////////////////////////////
// CAICHSyncTask

CAICHSyncTask::CAICHSyncTask()
        : CThreadTask(wxT("AICH Syncronizing"), wxEmptyString, ETP_Low)
{
}

bool CAICHSyncTask::Continue()
{
        try {
                CR_BEGIN;
                ConvertToKnown2ToKnown264();
                
                AddDebugLogLineN( logAICHThread, wxT("Syncronization thread started.") );
                
                // We collect all masterhashs which we find in the known2.met and store them in a list
                cr.fullpath = CPath(theApp->ConfigDir + KNOWN2_MET_FILENAME);
                
                if (!cr.fullpath.FileExists()) {
                        // File does not exist. Try to create it to see if it can be created at all (and don't start hashing otherwise).
                        if (!cr.file.Open(cr.fullpath, CFile::write)) {
                                AddDebugLogLineC( logAICHThread, wxT("Error, failed to open 'known2_64.met' file!") );
                                CR_EXIT;
                        }
                        try {
                                cr.file.WriteUInt8(KNOWN2_MET_VERSION);
                        } catch (const CIOFailureException& e) {
                                AddDebugLogLineC(logAICHThread, wxT("IO failure while creating hashlist (Aborting): ") + e.what());
                                CR_EXIT;
                        }
                } else {
                        if (!cr.file.Open(cr.fullpath, CFile::read)) {
                                AddDebugLogLineC( logAICHThread, wxT("Error, failed to open 'known2_64.met' file!") );
                                CR_EXIT;
                        }
                        
                        cr.nLastVerifiedPos = 0;
                        if (cr.file.ReadUInt8() != KNOWN2_MET_VERSION) {
                                throw CEOFException(wxT("Invalid met-file header found, removing file."));
                        }
                        
                        cr.nExistingSize = cr.file.GetLength();
                        while (cr.file.GetPosition() < cr.nExistingSize) {
                                {
                                        // Read the next hash
                                        cr.hashlist.push_back( CAICHHash( &cr.file ) );
                                        
                                        uint32 nHashCount = cr.file.ReadUInt32();
                                        if (cr.file.GetPosition() + nHashCount * CAICHHash::GetHashSize() > cr.nExistingSize) {
                                                throw CEOFException(wxT("Hashlist ends past end of file."));
                                        }
                                        
                                        // skip the rest of this hashset
                                        cr.nLastVerifiedPos = cr.file.Seek( nHashCount * HASHSIZE, wxFromCurrent );
                                }
                                CR_RETURN;
                        }
                        AddDebugLogLineN( logAICHThread, wxT("Masterhashes of known files have been loaded.") );
                }
                CR_END;
        } catch (const CEOFException&) {
                AddDebugLogLineC(logAICHThread, wxT("Hashlist corrupted, truncating file."));
                cr.file.Close();
                cr.file.Reopen(CFile::read_write);
                cr.file.SetLength(cr.nLastVerifiedPos);
        } catch (const CIOFailureException& e) {
                AddDebugLogLineC(logAICHThread, wxT("IO failure while reading hashlist (Aborting): ") + e.what());
                CR_EXIT;
        }
        
        // Now we check that all files which are in the sharedfilelist have a
        // corresponding hash in our list. Those how don't are queued for hashing.
        theApp->sharedfiles->CheckAICHHashes(cr.hashlist);
        CR_EXIT;
}


bool CAICHSyncTask::ConvertToKnown2ToKnown264()
{
        // converting known2.met to known2_64.met to support large files
        // changing hashcount from uint16 to uint32

        const CPath oldfullpath = CPath(theApp->ConfigDir + OLD_KNOWN2_MET_FILENAME);
        const CPath newfullpath = CPath(theApp->ConfigDir + KNOWN2_MET_FILENAME);

        if (newfullpath.FileExists() || !oldfullpath.FileExists()) {
                // In this case, there is nothing that we need to do.
                return false;
        }

        CFile oldfile;
        CFile newfile;

        if (!oldfile.Open(oldfullpath, CFile::read)) {
                AddDebugLogLineC(logAICHThread, wxT("Failed to open 'known2.met' file."));

                // else -> known2.met also doesn't exists, so nothing to convert
                return false;
        }


        if (!newfile.Open(newfullpath, CFile::write_excl)) {
                AddDebugLogLineC(logAICHThread, wxT("Failed to create 'known2_64.met' file."));

                return false;
        }

        AddLogLineN(CFormat(_("Converting old AICH hashsets in '%s' to 64b in '%s'."))
                    % OLD_KNOWN2_MET_FILENAME % KNOWN2_MET_FILENAME);

        try {
                newfile.WriteUInt8(KNOWN2_MET_VERSION);

                while (newfile.GetPosition() < oldfile.GetLength()) {
                        CAICHHash aichHash(&oldfile);
                        uint32 nHashCount = oldfile.ReadUInt64();

                        std::unique_ptr<byte[]> buffer(new byte[nHashCount * CAICHHash::GetHashSize()]);

                        oldfile.Read(buffer.get(), nHashCount * CAICHHash::GetHashSize());
                        newfile.Write(aichHash.GetRawHash(), CAICHHash::GetHashSize());
                        newfile.WriteUInt32(nHashCount);
                        newfile.Write(buffer.get(), nHashCount * CAICHHash::GetHashSize());
                }
                newfile.Flush();
        } catch (const CEOFException& e) {
                AddDebugLogLineC(logAICHThread, wxT("Error reading old 'known2.met' file.") + e.what());
                return false;
        } catch (const CIOFailureException& e) {
                AddDebugLogLineC(logAICHThread, wxT("IO error while converting 'known2.met' file: ") + e.what());
                return false;
        }

        // FIXME LARGE FILES (uncomment)
        //DeleteFile(oldfullpath);

        return true;
}



////////////////////////////////////////////////////////////
// CCompletionTask

CCompletionTask::CCompletionTask(const CPartFile* file)
// GetPrintable is used to improve the readability of the log.
        : CThreadTask(wxT("Completing"), file->GetFullName().GetPrintable(), ETP_High),
          m_filename(file->GetFileName()),
          m_metPath(file->GetFullName()),
          m_category(file->GetCategory()),
          m_owner(file),
          m_error(false)
{
        wxASSERT(m_filename.IsOk());
        wxASSERT(m_metPath.IsOk());
        wxASSERT(m_owner);
}


bool CCompletionTask::CloneFile()
{
        cr_begin(cr);
        if (!cr.in.Open(cr.partfilename, CFile::read)) {
                AddDebugLogLineC(logPartFile, CFormat(wxT("WARNING: Could not open original '%s' for moving into %s")) % cr.partfilename % cr.newName);
                m_error = true;
                cr_exit(cr);
        }
        if (!cr.out.Open(cr.newName, CFile::write)) {
                AddDebugLogLineC(logPartFile, CFormat(wxT("WARNING: Could not open '%s' for moving %s")) % cr.newName % cr.partfilename);
                m_error = true;
                cr.in.Close();
                cr_exit(cr);
        }
        
        while (cr.in.GetAvailable())
        {
                try {
                        AddDebugLogLineN(logPartFile, CFormat(wxT("CCompletionTask::CloneFile cloning %s to %s, remaining %uo")) % cr.partfilename % cr.newName % cr.in.GetAvailable());
                        std::array<byte, PARTSIZE> part;
                        uint64_t totransfer = min(PARTSIZE,cr.in.GetAvailable());
                        cr.in.Read(part.data(), totransfer);
                        cr.out.Write(part.data(), totransfer);
                } catch (CIOFailureException & e) {
                        AddDebugLogLineC(logPartFile, wxT("IO failure while cloning partfile (Aborting): ") + e.what());
                        cr.in.Close();
                        cr.out.Close();
                        cr_exit(cr);
                }
                cr_return(true, cr);
        }
        
        cr.in.Close();
        cr.out.Close();
        cr_end(cr);
        return false;
}

bool CCompletionTask::Continue()
{
        CR_BEGIN;
        {        
                CPath targetPath;
                AddDebugLogLineN(logPartFile, CFormat(wxT("CCompletionTask::Continue")));
                
                {
                        targetPath = theApp->glob_prefs->GetCategory(m_category)->path;
                        if (!targetPath.DirExists()) {
                                targetPath = thePrefs::GetIncomingDir();
                        }
                }
                
                CPath dstName = m_filename.Cleanup(true, !PlatformSpecific::CanFSHandleSpecialChars(targetPath));
                
                // Avoid empty filenames ...
                if (!dstName.IsOk()) {
                        dstName = CPath(wxT("Unknown"));
                }
                
                if (m_filename != dstName) {
                        AddLogLineC(CFormat(_("WARNING: The filename '%s' is invalid and has been renamed to '%s'.")) % m_filename % dstName);
                }

                // Avoid saving to an already existing filename
                cr.newName = targetPath.JoinPaths(dstName);
                for (unsigned count = 0; cr.newName.FileExists(); ++count) {
                        wxString postfix = CFormat(wxT("(%u)")) % count;
                        
                        cr.newName = targetPath.JoinPaths(dstName.AddPostfix(postfix));
                }
                
                if (cr.newName != targetPath.JoinPaths(dstName)) {
                        AddLogLineC(CFormat(_("WARNING: The file '%s' already exists, new file renamed to '%s'.")) % dstName % cr.newName.GetFullName());
                }
        }
        // Move will handle dirs on the same partition, otherwise copy is needed.
        cr.partfilename = m_metPath.RemoveExt();
        if (!CPath::RenameFile(cr.partfilename, cr.newName)) {
                CR_CALL(CloneFile());
                //if (!CPath::CloneFile(cr.partfilename, cr.newName, true)) {
                if (m_error)
                {
                        m_error = true;
                        CR_EXIT;
                }
                
                if (!CPath::RemoveFile(cr.partfilename)) {
                        AddDebugLogLineC(logPartFile, CFormat(wxT("WARNING: Could not remove original '%s' after creating backup")) % cr.partfilename);
                }
        } else {
                AddDebugLogLineN(logPartFile, CFormat(wxT("file '%s' renamed to '%s'.")) % cr.partfilename % cr.newName);
        }
        {
                // Removes the various other data-files
                const wxChar* otherMetExt[] = { wxT(""), PARTMET_BAK_EXT, wxT(".seeds"), NULL };
                for (size_t i = 0; otherMetExt[i]; ++i) {
                        CPath toRemove = m_metPath.AppendExt(otherMetExt[i]);
                        
                        if (toRemove.FileExists()) {
                                if (!CPath::RemoveFile(toRemove)) {
                                        AddDebugLogLineC(logPartFile, CFormat(wxT("WARNING: Failed to delete %s")) % toRemove);
                                }
                        }
                }

                m_newName = cr.newName;
        }
        CR_END;
        CR_EXIT;
}


void CCompletionTask::OnExit()
{
        // Notify the app that the completion has finished for this file.
        AddDebugLogLineN(logPartFile, CFormat(wxT("Notifying wxTheApp about completion of %s")) % m_newName);
        CCompletionEvent * evt = new CCompletionEvent(m_error, m_owner, m_newName);
        wxQueueEvent(wxTheApp, evt);
        CThreadTask::OnExit();
}



////////////////////////////////////////////////////////////
// CAllocateFileTask

#ifdef HAVE_FALLOCATE
#	ifndef _GNU_SOURCE
#		define _GNU_SOURCE
#	endif
#	ifdef HAVE_FCNTL_H
#		include <fcntl.h>
#	endif
#	include <linux/falloc.h>
#elif defined HAVE_SYS_FALLOCATE
#	include <sys/syscall.h>
#	include <sys/types.h>
#	include <unistd.h>
#elif defined HAVE_POSIX_FALLOCATE
#	define _XOPEN_SOURCE 600
#	include <stdlib.h>
#	ifdef HAVE_FCNTL_H
#		include <fcntl.h>
#	endif
#endif
#include <stdlib.h>
#include <errno.h>

CAllocateFileTask::CAllocateFileTask(CPartFile *file, bool pause)
// GetPrintable is used to improve the readability of the log.
        : CThreadTask(wxT("Allocating"), file->GetFullName().RemoveExt().GetPrintable(), ETP_High),
          m_file(file), m_pause(pause), m_result(ENOSYS)
{
        wxASSERT(file != NULL);
}


bool CAllocateFileTask::Continue()
{
        CR_BEGIN;
        if (m_file->GetFileSize() == 0) {
                m_result = 0;
                CR_EXIT;
        }

        cr.minFree = thePrefs::IsCheckDiskspaceEnabled() ? thePrefs::GetMinFreeDiskSpace() : 0;
        cr.freeSpace = CPath::GetFreeSpaceAt(thePrefs::GetTempDir());

        // Don't even try to allocate, if there's no space to complete the operation.
        if (cr.freeSpace != wxInvalidOffset) {
                if ((uint64_t)cr.freeSpace < m_file->GetFileSize() + cr.minFree) {
                        m_result = ENOSPC;
                        CR_EXIT;
                }
        }

        cr.file.Open(m_file->GetFullName().RemoveExt(), CFile::read_write);

#ifdef __WXMSW__
        try {
                // File is already created as non-sparse, so we only need to set the length.
                // This will fail to allocate the file e.g. under wine on linux/ext3,
                // but works with NTFS and FAT32.
                cr.file.Seek(m_file->GetFileSize() - 1, wxFromStart);
                cr.file.WriteUInt8(0);
                cr.file.Close();
                m_result = 0;
        } catch (const CSafeIOException&) {
                m_result = errno;
        }
#else
        // Use kernel level routines if possible
#  ifdef HAVE_FALLOCATE
        m_result = fallocate(cr.file.fd(), 0, 0, m_file->GetFileSize());
#  elif defined HAVE_SYS_FALLOCATE
        m_result = syscall(SYS_fallocate, cr.file.fd(), 0, (loff_t)0, (loff_t)m_file->GetFileSize());
        if (m_result == -1) {
                m_result = errno;
        }
#  elif defined HAVE_POSIX_FALLOCATE
        // otherwise use glibc implementation, if available
        m_result = posix_fallocate(cr.file.fd(), 0, m_file->GetFileSize());
#  endif

        if (m_result != 0 && m_result != ENOSPC) {
                // If everything else fails, use slow-and-dirty method of allocating the file: write the whole file with zeroes.
#  define BLOCK_SIZE	1048576		/* Write 1 MB blocks */
                cr.zero = calloc(1, BLOCK_SIZE);
                if (cr.zero != NULL) {
                        m_result = 0;
                        for (cr.size = m_file->GetFileSize(); cr.size >= BLOCK_SIZE; cr.size -= BLOCK_SIZE) {
                                try {
                                        cr.file.Write(cr.zero, BLOCK_SIZE);
                                } catch (const CSafeIOException&) {
                                        m_result = errno;
                                        cr.size = 0;
                                }
                                CR_RETURN;
                        }
                        if (cr.size > 0) {
                                try {
                                        cr.file.Write(cr.zero, cr.size);
                                } catch (const CSafeIOException&) {
                                        m_result = errno;
                                }
                        }
                        cr.file.Close();
                        free(cr.zero);
                } else {
                        m_result = ENOMEM;
                }
        }

#endif
        if (cr.file.IsOpened()) {
                cr.file.Close();
        }
        CR_END;
        CR_EXIT;
}

void CAllocateFileTask::OnExit()
{
        // Notify the app that the preallocation has finished for this file.
        CAllocFinishedEvent * evt =new CAllocFinishedEvent(m_file, m_pause, m_result);
        wxQueueEvent(wxTheApp, evt);
        CThreadTask::OnExit();
}



////////////////////////////////////////////////////////////
// CHashingEvent

DEFINE_LOCAL_EVENT_TYPE(MULE_EVT_HASHING)
DEFINE_LOCAL_EVENT_TYPE(MULE_EVT_AICH_HASHING)

CHashingEvent::CHashingEvent(wxEventType type, CKnownFile* result, const CKnownFile* owner)
        : wxEvent(-1, type),
          m_owner(owner),
          m_result(result)
{
}


wxEvent* CHashingEvent::Clone() const
{
        return new CHashingEvent(GetEventType(), m_result, m_owner);
}


const CKnownFile* CHashingEvent::GetOwner() const
{
        return m_owner;
}


CKnownFile* CHashingEvent::GetResult() const
{
        return m_result;
}




////////////////////////////////////////////////////////////
// CCompletionEvent

DEFINE_LOCAL_EVENT_TYPE(MULE_EVT_FILE_COMPLETED)


CCompletionEvent::CCompletionEvent(bool errorOccured, const CPartFile* owner, const CPath& fullPath)
        : wxEvent(-1, MULE_EVT_FILE_COMPLETED),
          m_fullPath(fullPath),
          m_owner(owner),
          m_error(errorOccured)
{
}


wxEvent* CCompletionEvent::Clone() const
{
        return new CCompletionEvent(m_error, m_owner, m_fullPath);
}


bool CCompletionEvent::ErrorOccured() const
{
        return m_error;
}


const CPartFile* CCompletionEvent::GetOwner() const
{
        return m_owner;
}


const CPath& CCompletionEvent::GetFullPath() const
{
        return m_fullPath;
}


////////////////////////////////////////////////////////////
// CAllocFinishedEvent

DEFINE_LOCAL_EVENT_TYPE(MULE_EVT_ALLOC_FINISHED)

wxEvent *CAllocFinishedEvent::Clone() const
{
        return new CAllocFinishedEvent(m_file, m_pause, m_result);
}

// File_checked_for_headers