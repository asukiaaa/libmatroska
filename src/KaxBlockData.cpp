/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
**
** Copyright (C) 2002-2010 Steve Lhomme.  All rights reserved.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
** 
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
** See http://www.matroska.org/license/lgpl/ for LGPL licensing information.**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
	\file
	\version \$Id: KaxBlockData.cpp 1226 2005-10-13 21:16:43Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#include <cassert>

#include "matroska/KaxBlockData.h"
#include "matroska/KaxContexts.h"
#include "matroska/KaxBlock.h"
#include "matroska/KaxDefines.h"

using namespace LIBEBML_NAMESPACE;

START_LIBMATROSKA_NAMESPACE

const EbmlSemantic ContextList_KaxSlices[1] =
{
	EbmlSemantic(false, false,  EBML_INFO(KaxTimeSlice)),
};

const EbmlSemantic ContextList_KaxTimeSlice[5] =
{
	EbmlSemantic(false, true,  EBML_INFO(KaxSliceLaceNumber)),
	EbmlSemantic(false, true,  EBML_INFO(KaxSliceFrameNumber)),
	EbmlSemantic(false, true,  EBML_INFO(KaxSliceBlockAddID)),
	EbmlSemantic(false, true,  EBML_INFO(KaxSliceDelay)),
	EbmlSemantic(false, true,  EBML_INFO(KaxSliceDuration)),
};

DEFINE_MKX_UINTEGER_DEF (KaxReferencePriority, 0xFA, 1, KaxBlockGroup, "FlagReferenced", 0);
DEFINE_MKX_SINTEGER_CONS(KaxReferenceBlock,    0xFB, 1, KaxBlockGroup, "ReferenceBlock");
DEFINE_MKX_MASTER       (KaxSlices,            0x8E, 1, KaxBlockGroup, "Slices");
DEFINE_MKX_MASTER       (KaxTimeSlice,         0xE8, 1, KaxSlices, "TimeSlice");
DEFINE_MKX_UINTEGER_DEF (KaxSliceLaceNumber,   0xCC, 1, KaxTimeSlice, "SliceLaceNumber", 0);
DEFINE_MKX_UINTEGER_DEF (KaxSliceFrameNumber,  0xCD, 1, KaxTimeSlice, "SliceFrameNumber", 0);
DEFINE_MKX_UINTEGER_DEF (KaxSliceBlockAddID,   0xCB, 1, KaxTimeSlice, "SliceBlockAddID", 0);
DEFINE_MKX_UINTEGER     (KaxSliceDelay,        0xCE, 1, KaxTimeSlice, "SliceDelay");
DEFINE_MKX_UINTEGER_DEF (KaxSliceDuration,     0xCF, 1, KaxTimeSlice, "SliceDuration", 0);
#if MATROSKA_VERSION >= 2
DEFINE_MKX_UINTEGER     (KaxReferenceVirtual,  0xFD, 1, KaxBlockGroup, "ReferenceVirtual");
#endif


const KaxBlockBlob & KaxReferenceBlock::RefBlock() const
{
	assert(RefdBlock != NULL);
	return *RefdBlock;
}

KaxReferenceBlock::KaxReferenceBlock()
:RefdBlock(NULL)
,ParentBlock(NULL)
{
    bTimecodeSet = false;
}

KaxReferenceBlock::KaxReferenceBlock(const KaxReferenceBlock & ElementToClone)
:EbmlSInteger(ElementToClone)
,bTimecodeSet(ElementToClone.bTimecodeSet)
{
}

filepos_t KaxReferenceBlock::UpdateSize(bool bSaveDefault, bool bForceRender)
{
	if (!bTimecodeSet) {
		assert(RefdBlock != NULL);
		assert(ParentBlock != NULL);

		const KaxInternalBlock &block = *RefdBlock;
		*static_cast<EbmlSInteger*>(this) = (int64(block.GlobalTimecode()) - int64(ParentBlock->GlobalTimecode())) / int64(ParentBlock->GlobalTimecodeScale());
	}
	return EbmlSInteger::UpdateSize(bSaveDefault, bForceRender);
}

void KaxReferenceBlock::SetReferencedBlock(const KaxBlockBlob * aRefdBlock)
{
	assert(RefdBlock == NULL);
	assert(aRefdBlock != NULL);
	RefdBlock = aRefdBlock; 
	SetValueIsSet();
}

void KaxReferenceBlock::SetReferencedBlock(const KaxBlockGroup & aRefdBlock)
{
	KaxBlockBlob *block_blob = new KaxBlockBlob(BLOCK_BLOB_NO_SIMPLE);
	block_blob->SetBlockGroup(*const_cast<KaxBlockGroup*>(&aRefdBlock));
	RefdBlock = block_blob; 
	SetValueIsSet();
}

END_LIBMATROSKA_NAMESPACE
