/*
  fst - An R-package for ultra fast storage and retrieval of datasets.
  Copyright (C) 2017, Mark AJ Klik

  BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the
    distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  You can contact the author at :
  - fst source repository : https://github.com/fstPackage/fst
*/

// Framework libraries
#include "blockStore.h"
#include <compression.h>
#include <compressor.h>

// External libraries
#include "lz4.h"

using namespace std;
using namespace Rcpp;

#define BLOCKSIZE_REAL 2048  // number of doubles in default compression block


SEXP fdsWriteRealVec(ofstream &myfile, SEXP &realVec, unsigned size, unsigned int compression)
{
  double* realP = REAL(realVec);
  unsigned int nrOfRows = LENGTH(realVec);  // vector length

  SEXP res;
  int blockSize = 8 * BLOCKSIZE_REAL;  // block size in bytes

  if (compression == 0)
  {
    return fdsStreamUncompressed(myfile, (char*) realP, nrOfRows, 8, BLOCKSIZE_REAL, NULL);
  }

  if (compression <= 50)  // low compression: linear mix of uncompressed and LZ4_SHUF
  {
    Compressor* compress1 = new DualCompressor(CompAlgo::LZ4_SHUF8, CompAlgo::LZ4, 0, 2 * compression);
    StreamCompressor* streamCompressor = new StreamLinearCompressor(compress1, 2 * compression);
    streamCompressor->CompressBufferSize(blockSize);
    res = fdsStreamcompressed(myfile, (char*) realP, nrOfRows, 8, streamCompressor, BLOCKSIZE_REAL);
    delete compress1;
    delete streamCompressor;
    return res;
  }

  Compressor* compress1 = new DualCompressor(CompAlgo::LZ4_SHUF8, CompAlgo::LZ4, 0, 100);
  Compressor* compress2 = new SingleCompressor(CompAlgo::ZSTD, 20);
  StreamCompressor* streamCompressor = new StreamCompositeCompressor(compress1, compress2, 2 * (compression - 50));
  streamCompressor->CompressBufferSize(blockSize);
  res = fdsStreamcompressed(myfile, (char*) realP, nrOfRows, 8, streamCompressor, BLOCKSIZE_REAL);
  delete compress1;
  delete compress2;
  delete streamCompressor;

  return res;
}


SEXP fdsReadRealVec(ifstream &myfile, SEXP &realVec, unsigned long long blockPos, unsigned startRow, unsigned length, unsigned size)
{
  char* values = (char*) REAL(realVec);  // output vector

  return fdsReadColumn(myfile, values, blockPos, startRow, length, size, 8);
}
