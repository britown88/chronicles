#pragma once

// Sean's Cool Format

#include "defs.h"

typedef struct SCFHeader SCFHeader;

enum SCFType_ {
   SCFType_NULL = 0,   
   SCFType_INT,
   SCFType_FLOAT,
   SCFType_STRING,
   SCFType_BYTES,
   SCFType_SUBLIST
};
typedef byte SCFType;

struct SCFReader {
   SCFHeader* header = nullptr;
   SCFType* typeList = nullptr;
   void* pos = nullptr;
};

SCFReader scfView(void const* scf);
bool scfReaderNull(SCFReader const& view);
bool scfReaderAtEnd(SCFReader const& view);

SCFType scfReaderPeek(SCFReader const& view);
u32 scfReaderRemaining(SCFReader const& view);
void scfReaderSkip(SCFReader& view);

SCFReader scfReadList(SCFReader& view);
i64 const* scfReadInt(SCFReader& view);
f64 const* scfReadFloat(SCFReader& view);
StringView scfReadString(SCFReader& view);
byte const* scfReadBytes(SCFReader& view, u64* sizeOut);

typedef struct SCFWriter SCFWriter;

SCFWriter* scfWriterCreate();
void scfWriterDestroy(SCFWriter* writer);

void scfWriteListBegin(SCFWriter* writer);
void scfWriteListEnd(SCFWriter* writer);
void scfWriteInt(SCFWriter* writer, i64 i);
void scfWriteFloat(SCFWriter* writer, f64 f);
void scfWriteString(SCFWriter* writer, StringView string);
void scfWriteBytes(SCFWriter* writer, void const* data, u64 size);

void* scfWriteToBuffer(SCFWriter* writer, u64* sizeOut);

void DEBUG_imShowWriterStats(SCFWriter *writer);