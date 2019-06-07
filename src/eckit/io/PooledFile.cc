/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include <cstdio>
#include <string>
#include <thread>

#include "eckit/io/PooledFile.h"
#include "eckit/filesystem/PathName.h"
#include "eckit/exception/Exceptions.h"

#include "eckit/exception/Exceptions.h"

namespace eckit {

class PoolFileEntry;

thread_local std::map<PathName, PoolFileEntry*> pool_;

struct PoolFileEntryStatus {

    off_t position_;
    bool opened_;

    PoolFileEntryStatus() :
        position_(0),
        opened_(false) {
    }
};

class PoolFileEntry {
public:
    std::string name_;
    FILE* file_;
    size_t count_;
    std::map<const PooledFile*, PoolFileEntryStatus > statuses_;

    long nbOpens_ = 0;
    long nbReads_ = 0;

public:

    PoolFileEntry(const std::string& name):
        name_(name),
        file_(nullptr),
        count_(0)
    {
    }

    void doClose() {
        if(file_) {
            Log::info() << "Closing from file " << name_ << std::endl;
            if(::fclose(file_) != 0) {
                throw PooledFileError(name_, "Failed to close", Here());
            }
            file_ = nullptr;
        }
    }

    void add(const PooledFile* file) {
        ASSERT(statuses_.find(file) == statuses_.end());
        statuses_[file] = PoolFileEntryStatus();
    }

    void remove(const PooledFile* file) {
        auto s = statuses_.find(file);
        ASSERT(s != statuses_.end());
        if(s->second.opened_) { // To check if we want that semantic
            throw PooledFileError(name_, "Pooled file not closed", Here());
        }

        statuses_.erase(s);

        if(statuses_.size() == 0) {
            doClose();
            pool_.erase(name_);
            // No code after !!!
        }
    }

    void open(const PooledFile* file) {
        auto s = statuses_.find(file);
        ASSERT(s != statuses_.end());
        ASSERT(!s->second.opened_);

        if(!file_) {
            Log::info() << "Opening file " << name_ << std::endl;
            nbOpens_++;
            file_ = ::fopen(name_.c_str(), "r");
        }

        s->second.opened_ = true;
        s->second.position_ = 0;
    }

    void close(const PooledFile* file)  {
        auto s = statuses_.find(file);
        ASSERT(s != statuses_.end());

        ASSERT(s->second.opened_);
        s->second.opened_ = false;
    }

    long read(const PooledFile* file, void *buffer, long len) {
        auto s = statuses_.find(file);
        ASSERT(s != statuses_.end());
        ASSERT(s->second.opened_);

        if(::fseeko(file_, s->second.position_, SEEK_SET)<0) {
            throw PooledFileError(name_, "Failed to seek", Here());
        }

        Log::info() << "Reading @ position " << s->second.position_ << " file : " << name_ << std::endl;

        long n = ::fread(buffer, 1, len, file_);
        s->second.position_ = ::ftello(file_);

        nbReads_++;

        return n;
    }

    long seek(const PooledFile* file, off_t position) {
        auto s = statuses_.find(file);
        ASSERT(s != statuses_.end());
        ASSERT(s->second.opened_);

        if(::fseeko(file_, position, SEEK_SET)<0) {
            return -1;
        }

        s->second.position_ = ::ftello(file_);

        return s->second.position_;
    }
};


PooledFile::PooledFile(const PathName& name):
    name_(name),
    entry_(nullptr)
{
    auto j = pool_.find(name);
    if(j == pool_.end()) {
        pool_[name] = new PoolFileEntry(name);
        j = pool_.find(name);
    }

    entry_ = (*j).second;
    entry_->add(this);
}

PooledFile::~PooledFile() {
    ASSERT(entry_);
    entry_->remove(this);
}

void PooledFile::open() {
    ASSERT(entry_);
    entry_->open(this);
}

void PooledFile::close() {
    ASSERT(entry_);
    entry_->close(this);
}

off_t PooledFile::seek(off_t offset) {
    ASSERT(entry_);
    return entry_->seek(this, offset);
}

off_t PooledFile::rewind() {
    return seek(0);
}

long PooledFile::nbOpens() const {
    ASSERT(entry_);
    return entry_->nbOpens_;
}

long PooledFile::nbReads() const {
    ASSERT(entry_);
    return entry_->nbReads_;
}

long PooledFile::read(void *buffer, long len) {
    ASSERT(entry_);
    return entry_->read(this, buffer, len);
}

PooledFileError::PooledFileError(const std::string& file, const std::string& msg, const CodeLocation& loc) :
    FileError(msg + " : error on pooled file " + file, loc) {}

} // namespace eckit

