/*
 * (C) Copyright 1996- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include "eckit/utils/AECCompressor.h"
#include "libaec.h"

#include "eckit/exception/Exceptions.h"
#include "eckit/io/Buffer.h"
#include "eckit/io/ResizableBuffer.h"


// compression parameters heve been tuned for maximum compression ratio with GRIB2 3D fields.
// Performance on GRIB 1 sfc fields is bad with any parameter values
// https://sourceware.org/bzip2/manual/manual.html#bzcompress-init
// bits_per_sample   range [1..32]  Storage size from sample size. If a sample requires less bits than the storage size
// provides, then you have to make sure that unused bits are not set.
//                                  Libaec does not check this for performance reasons and will produce undefined output
//                                  if unused bits are set.
//                                   1 -  8 bits 	1 byte
//                                   9 - 16 bits 	2 bytes
//                                  17 - 24 bits 	3 bytes (only if AEC_DATA_3BYTE is set)
//                                  25 - 32 bits 	4 bytes (if AEC_DATA_3BYTE is set)
//                                  17 - 32 bits 	4 bytes (if AEC_DATA_3BYTE is not set)
// block_size       range [8..64]   Smaller blocks allow the compression to adapt more rapidly to changing source
// statistics. Larger blocks create less overhead but can be less efficient if source statistics change across the
// block. rsi                              Sets the reference sample interval. A large RSI will improve performance and
// efficiency. It will also increase memory requirements since internal buffering is based on RSI size.
//                                  A smaller RSI may be desirable in situations where each RSI will be packetized and
//                                  possible error propagation has to be minimized.
#define AEC_bits_per_sample 16
#define AEC_block_size 64
#define AEC_rsi 129
#define AEC_flags AEC_DATA_PREPROCESS | AEC_DATA_MSB


namespace eckit {

//----------------------------------------------------------------------------------------------------------------------

AECCompressor::AECCompressor() {}

AECCompressor::~AECCompressor() {}

size_t AECCompressor::minInputSize(const size_t inputSize, const aec_stream& strm) {
    int blockSizeBytes = strm.bits_per_sample * strm.block_size / 8;
    int minSize        = (inputSize / blockSizeBytes);
    if (inputSize % blockSizeBytes > 0)
        minSize++;

    return minSize * blockSizeBytes;
}

size_t AECCompressor::compress(const eckit::Buffer& inTmp, ResizableBuffer& out) const {
    std::ostringstream msg;

    struct aec_stream strm;

    strm.bits_per_sample = AEC_bits_per_sample;
    strm.block_size      = AEC_block_size;
    strm.rsi             = AEC_rsi;
    strm.flags           = AEC_flags;

    Buffer in(inTmp, minInputSize(inTmp.size(), strm));

    unsigned int maxcompressed = (size_t)(1.2 * in.size());
    if (out.size() < maxcompressed)
        out.resize(maxcompressed);
    unsigned int bufferSize = out.size();

    strm.next_in  = (unsigned char*)in.data();
    strm.avail_in = in.size();

    strm.next_out  = (unsigned char*)out.data();
    strm.avail_out = bufferSize;

    int ret = aec_encode_init(&strm);
    if (ret != AEC_OK) {
        msg << "returned " << ret;
        throw FailedLibraryCall("AEC", "aec_encode_init", msg.str(), Here());
    }

    // Perform encoding in one call and flush output.
    // you must be sure that the output buffer is large enough for all compressed output
    ret = aec_encode(&strm, AEC_FLUSH);
    if (ret != AEC_OK) {
        msg << "returned " << ret;
        throw FailedLibraryCall("AEC", "aec_encode", msg.str(), Here());
    }
    size_t outSize = strm.total_out;

    // free all resources used by encoder
    ret = aec_encode_end(&strm);
    if (ret == AEC_OK)
        return outSize;

    msg << "returned " << ret;
    throw FailedLibraryCall("AEC", "aec_encode_end", msg.str(), Here());
}

size_t AECCompressor::uncompress(const eckit::Buffer& in, ResizableBuffer& out) const {
    std::ostringstream msg;

    // AEC assumes you have transmitted the original size separately
    // We assume here that out is correctly sized

    struct aec_stream strm;

    strm.bits_per_sample = AEC_bits_per_sample;
    strm.block_size      = AEC_block_size;
    strm.rsi             = AEC_rsi;
    strm.flags           = AEC_flags;

    strm.next_in  = (unsigned char*)in.data();
    strm.avail_in = in.size();

    Buffer outTmp(minInputSize(out.size(), strm));

    strm.next_out  = (unsigned char*)outTmp.data();
    strm.avail_out = outTmp.size();

    int ret = aec_decode_init(&strm);
    if (ret != AEC_OK) {
        msg << "returned " << ret;
        throw FailedLibraryCall("AEC", "aec_decode_init", msg.str(), Here());
    }

    ret = aec_decode(&strm, AEC_FLUSH);
    if (ret != AEC_OK) {
        msg << "returned " << ret;
        throw FailedLibraryCall("AEC", "aec_decode", msg.str(), Here());
    }
    size_t outSize = strm.total_out;

    // free all resources used by decoder
    ret = aec_decode_end(&strm);
    if (ret == AEC_OK) {
        ::memcpy(out, outTmp, out.size());
        return out.size();
    }
    msg << "returned " << ret;
    throw FailedLibraryCall("AEC", "aec_decode_end", msg.str(), Here());
}

CompressorBuilder<AECCompressor> aec("aec");

//----------------------------------------------------------------------------------------------------------------------

}  // namespace eckit
